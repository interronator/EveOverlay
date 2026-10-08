#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct ShipType
{
    int Id = 0;
    std::string Name;
    std::string Group;
    std::string Category;
};

// Ships, drones, structures and NPCs by type id and name, read from universeData\types.tsv (built by tools\Build-ShipData.ps1)
class ShipCatalog
{
public:
    bool Load(const std::filesystem::path& TsvPath);

    const ShipType* FindById(const int Id) const;
    const ShipType* FindByName(const std::string& Name) const;
    size_t Count() const;

private:
    static constexpr size_t FIELD_ID = 0;
    static constexpr size_t FIELD_NAME = 1;
    static constexpr size_t FIELD_GROUP = 2;
    static constexpr size_t FIELD_CATEGORY = 3;
    static constexpr size_t FIELD_COUNT = 4;

    std::vector<ShipType> Types;
    std::unordered_map<int, size_t> ById;
    std::unordered_map<std::string, size_t> ByLowerName;
};
