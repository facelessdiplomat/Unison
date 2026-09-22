#pragma once

#include <unison/core/asset_id.hpp>
#include <unison/core/contract.hpp>
#include <unison/sim/padding_free.hpp>

#include <entt/core/type_info.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>

namespace unison::sim
{

/// What a type must be to serve as an asset: plain data free of padding, because the asset tables
/// are hashed into the number clients compare before they agree that they are playing the same game.
template <typename Asset>
struct AssetTraits
{
    static constexpr bool isValid = std::is_trivially_copyable_v<Asset> && PaddingFree<Asset>;
};

/// One asset as the hash sees it: which name it answers to and the bytes it is made of.
struct AssetEntry
{
    AssetId id = AssetId{};
    const std::byte* bytes = nullptr;
    std::size_t size = 0;
};

/// The design data a match is built from, keyed by the hash of its name and grouped by type. It is
/// filled before a session starts and frozen once, after which it only answers questions: an asset
/// that arrives late, an identifier already taken, or a lookup that finds nothing all break a
/// contract rather than being papered over, because a 32-bit name hash can collide.
class AssetRegistry
{
public:
    template <typename Asset>
    void add(AssetId id, const Asset& asset)
    {
        static_assert(AssetTraits<Asset>::isValid, "an asset must be plain data free of padding");

        UNISON_VERIFY(!frozen);
        UNISON_VERIFY(!holds(id));

        if (frozen || holds(id))
        {
            return;
        }

        tableFor<Asset>().add(id, asset);
    }

    template <typename Asset>
    [[nodiscard]] const Asset& get(AssetId id) const
    {
        static_assert(AssetTraits<Asset>::isValid, "an asset must be plain data free of padding");

        UNISON_VERIFY(frozen);

        const Table* table = findTable(entt::type_hash<Asset>::value());
        UNISON_VERIFY(table != nullptr);

        static const Asset missing{};

        return table == nullptr ? missing : static_cast<const TypedTable<Asset>*>(table)->get(id);
    }

    void freeze();

    [[nodiscard]] bool isFrozen() const;

    [[nodiscard]] bool holds(AssetId id) const;

    void collectAssets(std::vector<AssetEntry>& entries) const;

private:
    class Table
    {
    public:
        Table() = default;
        virtual ~Table() = default;

        Table(const Table&) = delete;
        Table& operator=(const Table&) = delete;
        Table(Table&&) = delete;
        Table& operator=(Table&&) = delete;

        [[nodiscard]] virtual entt::id_type typeId() const = 0;

        [[nodiscard]] virtual bool holds(AssetId id) const = 0;

        virtual void collect(std::vector<AssetEntry>& entries) const = 0;
    };

    template <typename Asset>
    class TypedTable final : public Table
    {
    public:
        void add(AssetId id, const Asset& asset)
        {
            ids.push_back(id);
            assets.push_back(asset);
        }

        [[nodiscard]] const Asset& get(AssetId id) const
        {
            for (std::size_t index = 0; index < ids.size(); ++index)
            {
                if (ids[index] == id)
                {
                    return assets[index];
                }
            }

            UNISON_VERIFY(false);

            static const Asset missing{};

            return missing;
        }

        [[nodiscard]] entt::id_type typeId() const override
        {
            return entt::type_hash<Asset>::value();
        }

        [[nodiscard]] bool holds(AssetId id) const override
        {
            for (const AssetId taken : ids)
            {
                if (taken == id)
                {
                    return true;
                }
            }

            return false;
        }

        void collect(std::vector<AssetEntry>& entries) const override
        {
            for (std::size_t index = 0; index < ids.size(); ++index)
            {
                entries.push_back(
                    AssetEntry{ids[index], reinterpret_cast<const std::byte*>(&assets[index]), sizeof(Asset)});
            }
        }

    private:
        std::vector<AssetId> ids;
        std::vector<Asset> assets;
    };

    template <typename Asset>
    TypedTable<Asset>& tableFor()
    {
        const entt::id_type wanted = entt::type_hash<Asset>::value();

        for (const std::unique_ptr<Table>& table : tables)
        {
            if (table->typeId() == wanted)
            {
                return *static_cast<TypedTable<Asset>*>(table.get());
            }
        }

        tables.push_back(std::make_unique<TypedTable<Asset>>());

        return *static_cast<TypedTable<Asset>*>(tables.back().get());
    }

    [[nodiscard]] const Table* findTable(entt::id_type typeId) const;

    std::vector<std::unique_ptr<Table>> tables;
    bool frozen = false;
};

}
