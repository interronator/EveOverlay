#include "Universe/UniverseData.h"

#include <cstdlib>
#include <fstream>

#include "Config/TextUtil.h"

bool UniverseData::Load(const std::filesystem::path& CsvPath)
{
    std::ifstream Stream(CsvPath, std::ios::binary);
    if (Stream.is_open() == false)
    {
        return false;
    }

    Systems.clear();
    NameToIndex.clear();

    std::vector<std::vector<std::string>> GateNames;
    std::string Line;
    bool IsHeader = true;
    while (std::getline(Stream, Line).fail() == false)
    {
        if (IsHeader == true)
        {
            IsHeader = false;
            continue;
        }

        const std::vector<std::string> Fields = TextUtil::Split(StripByteOrderMark(Line), ',');
        if (Fields.size() < FIELD_COUNT)
        {
            continue;
        }

        SolarSystem System;
        System.Name = Fields[FIELD_NAME];
        System.Region = Fields[FIELD_REGION];
        System.SovHolder = Fields[FIELD_SOV];
        System.Security = std::strtod(Fields[FIELD_SECURITY].c_str(), nullptr);

        NameToIndex[TextUtil::ToLower(System.Name)] = static_cast<int>(Systems.size());
        GateNames.push_back(Fields[FIELD_GATES].empty() == true ? std::vector<std::string>() : TextUtil::Split(Fields[FIELD_GATES], ';'));
        Systems.push_back(std::move(System));
    }

    for (size_t Index = 0; Index < Systems.size(); Index++)
    {
        for (const std::string& GateName : GateNames[Index])
        {
            const int Neighbour = Find(GateName);
            if (Neighbour == NOT_FOUND)
            {
                continue;
            }

            AddLink(static_cast<int>(Index), Neighbour);
        }
    }

    return Systems.empty() == false;
}

int UniverseData::Find(const std::string& Name) const
{
    const std::unordered_map<std::string, int>::const_iterator Match = NameToIndex.find(TextUtil::ToLower(TextUtil::Trim(Name)));
    if (Match == NameToIndex.end())
    {
        return NOT_FOUND;
    }

    return Match->second;
}

const SolarSystem& UniverseData::Get(const int Index) const
{
    return Systems[static_cast<size_t>(Index)];
}

size_t UniverseData::Count() const
{
    return Systems.size();
}

std::string UniverseData::StripByteOrderMark(const std::string& Text)
{
    if (Text.compare(0, 3, "\xEF\xBB\xBF") == 0)
    {
        return Text.substr(3);
    }

    return Text;
}

void UniverseData::AddLink(const int From, const int To)
{
    if (From == To)
    {
        return;
    }

    LinkOneWay(From, To);
    LinkOneWay(To, From);
}

void UniverseData::LinkOneWay(const int From, const int To)
{
    std::vector<int>& Neighbours = Systems[static_cast<size_t>(From)].Neighbours;
    for (const int Existing : Neighbours)
    {
        if (Existing == To)
        {
            return;
        }
    }

    Neighbours.push_back(To);
}
