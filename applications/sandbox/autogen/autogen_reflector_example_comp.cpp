#pragma once
#include "autogen_reflector_example_comp.hpp"
namespace legion::core
{
    L_NODISCARD reflector make_reflector(example_comp& obj)
    {
        reflector refl;
        refl.typeId = typeHash<example_comp>();
        refl.typeName = "example_comp";
        refl.members = std::vector<member_reference>
        {
            member_reference
            {
                "value",
                primitive_reference {typeHash<int>(), &obj.value}
            }
        }
        ;refl.data = std::addressof(obj);
        return refl;
    }
    L_NODISCARD reflector make_reflector(const example_comp& obj)
    {
        ptr_type address = reinterpret_cast<ptr_type>(std::addressof(obj));
        reflector refl;
        refl.typeId = typeHash<example_comp>();
        refl.typeName = "example_comp";
        refl.members = std::vector<member_reference>
        {
            member_reference
            {
                "value",
                primitive_reference {typeHash<int>(), &obj.value}
            }
        }
        ;refl.data = reinterpret_cast<void*>(address);
        return refl;
    }
}
