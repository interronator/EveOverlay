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
        // Several types can share a name (an NPC "Vanguard" and the real one); a ship wins over anything else
        const std::string LowerName = TextUtil::ToLower(Type.Name);
        const std::unordered_map<std::string, size_t>::iterator Existing = ByLowerName.find(LowerName);
        if (Existing == ByLowerName.end())
        {
            ByLowerName.emplace(LowerName, Types.size());
        }
        else if (Types[Existing->second].Category != "Ship" && Type.Category == "Ship")
        {
            Existing->second = Types.size();
        }

        Types.push_back(std::move(Type));
    }

    return Types.empty() == false;
}

const ShipCatalog& ShipCatalog::GetShared(const std::filesystem::path& TsvPath)
{
    static ShipCatalog Shared;
    static bool Tried = false;
    if (Tried == false)
    {
        Tried = true;
        Shared.Load(TsvPath);
    }

    return Shared;
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
