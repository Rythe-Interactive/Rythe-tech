#pragma once
#include <core/types/types.hpp>
#include <core/containers/hashed_sparse_set.hpp>

namespace legion::core
{
    namespace ecs
    {
        struct component_base;

        template<typename component_type>
        struct component;

        struct entity;
    }

    namespace serialization
    {
        template<typename T>
        struct prototype;

        using component_prototype_base = prototype<ecs::component_base>;

        template<typename component_type>
        using component_prototype = prototype<ecs::component<component_type>>;
    }
}

#if !defined(DOXY_EXCLUDE)
namespace std
{
    template<> struct hash<legion::core::ecs::entity>
    {
        constexpr std::size_t operator()(legion::core::ecs::entity const& handle) const noexcept;
    };
}
#endif


namespace legion::core::ecs
{
    using entity_set = hashed_sparse_set<entity>;

    static constexpr id_type world_entity_id = 1;

    /**@struct entity
     * @brief Handle to an entity.
     */
    struct entity
    {
    public:
        id_type id = invalid_id;

        bool operator ==(std::nullptr_t) const;
        bool operator !=(std::nullptr_t) const;

        operator const id_type& () const noexcept;
        operator id_type& () noexcept;

        void set_parent(id_type parent);
        void set_parent(entity parent);

        entity get_parent() const;

        void add_child(id_type child);
        void add_child(entity child);

        void remove_child(id_type child);
        void remove_child(entity child);

        entity_set& children();
        const entity_set& children() const;

        entity_set::iterator begin();
        entity_set::const_iterator begin() const;
        entity_set::const_iterator cbegin() const;

        entity_set::iterator end();
        entity_set::const_iterator end() const;
        entity_set::const_iterator cend() const;

        void destroy(bool recurse = true);

        template<typename component_type>
        component<component_type> add_component();

        template<typename component_type>
        component<component_type> add_component(const serialization::component_prototype<component_type>& prot);

        template<typename component_type>
        component<component_type> add_component(serialization::component_prototype<component_type>&& prot);

        template<typename component_type>
        bool has_component() const;

        template<typename component_type>
        component<component_type> get_component();

        template<typename component_type>
        const component<component_type> get_component() const;

        template<typename component_type>
        void remove_component();
    };

    static constexpr entity world = entity{ world_entity_id };
}


#if !defined(DOXY_EXCLUDE)
namespace std
{
    constexpr size_t hash<legion::core::ecs::entity>::operator()(legion::core::ecs::entity const& handle) const noexcept
    {
        return handle.id;
    }
}
#endif