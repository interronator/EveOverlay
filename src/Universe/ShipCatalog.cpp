#include "Universe/ShipCatalog.h"

#include <fstream>
#include <utility>

#include "Config/TextUtil.h"

bool ShipCatalog::Load(const std::filesystem::path& TsvPath)
{
    std::ifstream Stream(TsvPath, std::ios::binary);
    if (Stream.is_open() == false)
    {
        return false;
    }

    Types.clear();
    ById.clear();
    ByLowerName.clear();

    std::string Line;
    while (std::getline(Stream, Line))
    {
        const std::vector<std::string> Fields = TextUtil::Split(TextUtil::Trim(Line), '\t');
        int Id = 0;
        if (Fields.size() < FIELD_COUNT || TextUtil::TryParseInt(Fields[FIELD_ID], &Id) == false)
        {
            continue;
        }

        ShipType Type;
        Type.Id = Id;
        Type.Name = Fields[FIELD_NAME];
        Type.Group = Fields[FIELD_GROUP];
        Type.Category = Fields[FIELD_CATEGORY];

        ById[Id] = Types.size();
        ByLowerName.emplace(TextUtil::ToLower(Type.Name), Types.size());
        Types.push_back(std::move(Type));
    }

    return Types.empty() == false;
}

const ShipType* ShipCatalog::FindById(const int Id) const
{
    const std::unordered_map<int, size_t>::const_iterator Found = ById.find(Id);
    return Found == ById.end() ? nullptr : &Types[Found->second];
}

const ShipType* ShipCatalog::FindByName(const std::string& Name) const
{
    const std::unordered_map<std::string, size_t>::const_iterator Found = ByLowerName.find(TextUtil::ToLower(TextUtil::Trim(Name)));
    return Found == ByLowerName.end() ? nullptr : &Types[Found->second];
}

size_t ShipCatalog::Count() const
{
    return Types.size();
}
