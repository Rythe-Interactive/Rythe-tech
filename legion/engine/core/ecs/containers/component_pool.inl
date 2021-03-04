#include <core/ecs/registry.hpp>
#include "component_pool.hpp"
#pragma once

namespace legion::core::ecs
{
    template<typename component_type>
    void* component_pool<component_type>::this_ptr = Registry::tryEmplaceFamily<component_type>();
    
    template<typename component_type>
    sparse_map<id_type, component_type> component_pool<component_type>::m_components;

    template<typename component_type>
    inline void* component_pool<component_type>::create_component(id_type target)
    {
        FilterRegistry::markComponentAdd<component_type>(entity{ target });
        return &(m_components.emplace(target).first.value());
    }

    template<typename component_type>
    inline void* component_pool<component_type>::create_component(id_type target, const serialization::component_prototype_base& prototype)
    {
        FilterRegistry::markComponentAdd<component_type>(entity{ target });
        return &(m_components.emplace(target,
            from_reflector(static_cast<const serialization::component_prototype<component_type>&>(prototype))
        ).first.value());
    }

    template<typename component_type>
    inline void* component_pool<component_type>::create_component(id_type target, serialization::component_prototype_base&& prototype)
    {
        FilterRegistry::markComponentAdd<component_type>(entity{ target });
        return &(m_components.emplace(target,
            from_reflector(static_cast<serialization::component_prototype<component_type>&&>(prototype))
        ).first.value());
    }

    template<typename component_type>
    inline bool component_pool<component_type>::contains(id_type target) const
    {
        return m_components.contains(target);
    }

    template<typename component_type>
    inline void component_pool<component_type>::destroy_component(id_type target)
    {
        m_components.erase(target);
        FilterRegistry::markComponentErase<component_type>(entity{ target });
    }

    template<typename component_type>
    inline void* component_pool<component_type>::get_component(id_type target) const
    {
        return &(m_components.at(target));
    }

    template<typename component_type>
    inline component_type& component_pool<component_type>::create_component_direct(id_type target)
    {
        FilterRegistry::markComponentAdd<component_type>(entity{ target });
        return m_components.emplace(target).first.value();
    }

    template<typename component_type>
    inline component_type& component_pool<component_type>::create_component_direct(id_type target, const serialization::component_prototype_base& prototype)
    {
        FilterRegistry::markComponentAdd<component_type>(entity{ target });
        return m_components.emplace(target,
            from_reflector(static_cast<const serialization::component_prototype<component_type>&>(prototype))
        ).first.value();
    }

    template<typename component_type>
    inline component_type& component_pool<component_type>::create_component_direct(id_type target, serialization::component_prototype_base&& prototype)
    {
        FilterRegistry::markComponentAdd<component_type>(entity{ target });
        return m_components.emplace(target,
            from_reflector(static_cast<serialization::component_prototype<component_type>&&>(prototype))
        ).first.value();
    }

    template<typename component_type>
    inline bool component_pool<component_type>::contains_direct(id_type target)
    {
        return m_components.contains(target);
    }

    template<typename component_type>
    inline void component_pool<component_type>::destroy_component_direct(id_type target)
    {
        m_components.erase(target);
        FilterRegistry::markComponentErase<component_type>(entity{ target });
    }

    template<typename component_type>
    inline component_type& component_pool<component_type>::get_component_direct(id_type target)
    {
        return m_components.at(target);
    }

}