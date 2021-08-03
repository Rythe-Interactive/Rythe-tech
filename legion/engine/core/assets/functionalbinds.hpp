#pragma once
#include <core/assets/assetcache.hpp>

namespace legion::core::assets
{
    template<typename AssetType>
    common::result<asset<AssetType>> load(const fs::view& file);
    template<typename AssetType>
    common::result<asset<AssetType>> load(const fs::view& file, const import_settings<AssetType>& settings);
    template<typename AssetType>
    common::result<asset<AssetType>> load(const std::string& name, const fs::view& file);
    template<typename AssetType>
    common::result<asset<AssetType>> load(const std::string& name, const fs::view& file, const import_settings<AssetType>& settings);
    template<typename AssetType>
    common::result<asset<AssetType>> load(id_type nameHash, const std::string& name, const fs::view& file);
    template<typename AssetType>
    common::result<asset<AssetType>> load(id_type nameHash, const std::string& name, const fs::view& file, const import_settings<AssetType>& settings);
    template<typename AssetType>
    async::async_operation<common::result<asset<AssetType>>> loadAsync(const fs::view& file);
    template<typename AssetType>
    async::async_operation<common::result<asset<AssetType>>> loadAsync(const fs::view& file, const import_settings<AssetType>& settings);
    template<typename AssetType>
    async::async_operation<common::result<asset<AssetType>>> loadAsync(const std::string& name, const fs::view& file);
    template<typename AssetType>
    async::async_operation<common::result<asset<AssetType>>> loadAsync(const std::string& name, const fs::view& file, const import_settings<AssetType>& settings);
    template<typename AssetType>
    async::async_operation<common::result<asset<AssetType>>> loadAsync(id_type nameHash, const std::string& name, const fs::view& file);
    template<typename AssetType>
    async::async_operation<common::result<asset<AssetType>>> loadAsync(id_type nameHash, const std::string& name, const fs::view& file, const import_settings<AssetType>& settings);

    template<typename AssetType>
    asset<AssetType> create(const std::string& name);
    template<typename AssetType>
    asset<AssetType> create(const std::string& name, AssetType&& src);
    template<typename AssetType>
    asset<AssetType> create(const std::string& name, const AssetType& src);
    template<typename AssetType, typename... Arguments>
    asset<AssetType> create(const std::string& name, Arguments&&... args);
    template<typename AssetType>
    asset<AssetType> create(id_type nameHash, const std::string& name);
    template<typename AssetType>
    asset<AssetType> create(id_type nameHash, const std::string& name, AssetType&& src);
    template<typename AssetType>
    asset<AssetType> create(id_type nameHash, const std::string& name, const AssetType& src);
    template<typename AssetType, typename... Arguments>
    asset<AssetType> create(id_type nameHash, const std::string& name, Arguments&&... args);

    template<typename AssetType>
    asset<AssetType> exists(const std::string& name);
    template<typename AssetType>
    asset<AssetType> exists(id_type nameHash);

    template<typename AssetType>
    asset<AssetType> get(const std::string& name);
    template<typename AssetType>
    asset<AssetType> get(id_type nameHash);

    template<typename AssetType>
    void destroy(const std::string& name);
    template<typename AssetType>
    void destroy(id_type nameHash);
}

#include <core/assets/functionalbinds.inl>
