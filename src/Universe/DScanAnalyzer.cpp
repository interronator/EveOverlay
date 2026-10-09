#include "Universe/DScanAnalyzer.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>

#include "Config/TextUtil.h"

const DScanAnalyzer::GroupRule DScanAnalyzer::GROUP_RULES[] = {
    {"Titan", ShipClass::Capital, "Titan", 100},
    {"Supercarrier", ShipClass::Capital, "Supercarrier", 95},
    {"Lancer Dreadnought", ShipClass::Capital, "Capital ship", 90},
    {"Dreadnought", ShipClass::Capital, "Capital ship", 90},
    {"Carrier", ShipClass::Capital, "Capital ship", 85},
    {"Command Carrier", ShipClass::Capital, "Capital ship", 85},
    {"Force Auxiliary", ShipClass::Capital, "Capital logistics", 85},
    {"Capital Industrial Ship", ShipClass::Capital, nullptr, 0},
    {"Black Ops", ShipClass::Combat, "Can bridge and light a covert cyno", 80},
    {"Force Recon Ship", ShipClass::Recon, "Can light a cyno, electronic warfare", 70},
    {"Covert Ops", ShipClass::Recon, "Can light a covert cyno", 60},
    {"Heavy Interdiction Cruiser", ShipClass::Combat, "Can launch a heavy bubble", 65},
    {"Interdictor", ShipClass::Combat, "Can launch bubbles", 60},
    {"Command Ship", ShipClass::Combat, "Fleet boosts", 50},
    {"Command Destroyer", ShipClass::Combat, "Fleet boosts, micro jump field", 50},
    {"Stealth Bomber", ShipClass::Combat, "Bombs and torpedoes", 45},
    {"Combat Recon Ship", ShipClass::Recon, "Electronic warfare", 40},
    {"Electronic Attack Ship", ShipClass::Recon, "Electronic warfare", 40},
    {"Logistics", ShipClass::Logistics, "Remote repairs", 30},
    {"Logistics Frigate", ShipClass::Logistics, "Remote repairs", 30},
    {"Marauder", ShipClass::Combat, nullptr, 0},
    {"Strategic Cruiser", ShipClass::Combat, nullptr, 0},
    {"Tactical Destroyer", ShipClass::Combat, nullptr, 0},
    {"Assault Frigate", ShipClass::Combat, nullptr, 0},
    {"Heavy Assault Cruiser", ShipClass::Combat, nullptr, 0},
    {"Interceptor", ShipClass::Combat, nullptr, 0},
    {"Attack Battlecruiser", ShipClass::Combat, nullptr, 0},
    {"Combat Battlecruiser", ShipClass::Combat, nullptr, 0},
    {"Flag Cruiser", ShipClass::Combat, nullptr, 0},
    {"Corvette", ShipClass::Combat, nullptr, 0},
    {"Frigate", ShipClass::Combat, nullptr, 0},
    {"Destroyer", ShipClass::Combat, nullptr, 0},
    {"Cruiser", ShipClass::Combat, nullptr, 0},
    {"Battlecruiser", ShipClass::Combat, nullptr, 0},
    {"Battleship", ShipClass::Combat, nullptr, 0},
    {"Industrial Command Ship", ShipClass::Industrial, nullptr, 0},
    {"Hauler", ShipClass::Industrial, nullptr, 0},
    {"Deep Space Transport", ShipClass::Industrial, nullptr, 0},
    {"Blockade Runner", ShipClass::Industrial, nullptr, 0},
    {"Freighter", ShipClass::Industrial, nullptr, 0},
    {"Jump Freighter", ShipClass::Industrial, nullptr, 0},
    {"Mining Barge", ShipClass::Industrial, nullptr, 0},
    {"Exhumer", ShipClass::Industrial, nullptr, 0},
    {"Expedition Frigate", ShipClass::Industrial, nullptr, 0},
    {"Prototype Exploration Ship", ShipClass::Industrial, nullptr, 0},
};

const DScanAnalyzer::GroupRule* DScanAnalyzer::FindRule(const std::string& Group)
{
    for (const GroupRule& Rule : GROUP_RULES)
    {
        if (Group == Rule.Group)
        {
            return &Rule;
        }
    }

    return nullptr;
}

bool DScanAnalyzer::IsAllDigits(const std::string& Text)
{
    return Text.empty() == false && std::all_of(Text.begin(), Text.end(), [](const char Character)
    {
        return Character >= '0' && Character <= '9';
    });
}

ShipClass DScanAnalyzer::Classify(const ShipType& Type)
{
    if (Type.Category == "Ship")
    {
        const GroupRule* const Rule = FindRule(Type.Group);
        return Rule == nullptr ? ShipClass::OtherShip : Rule->Class;
    }

    if (Type.Category == "Drone" || Type.Category == "Fighter")
    {
        return ShipClass::Drone;
    }

    if (Type.Category == "Entity")
    {
        return ShipClass::Npc;
    }

    return ShipClass::Structure;
}

const char* DScanAnalyzer::GetClassLabel(const ShipClass Class)
{
    switch (Class)
    {
    case ShipClass::Capital:
        return "Capitals";
    case ShipClass::Combat:
        return "Combat ships";
    case ShipClass::Recon:
        return "Recon and covert";
    case ShipClass::Logistics:
        return "Logistics";
    case ShipClass::Industrial:
        return "Industrial";
    case ShipClass::OtherShip:
        return "Other ships";
    case ShipClass::Drone:
        return "Drones and fighters";
    case ShipClass::Npc:
        return "NPCs";
    case ShipClass::Structure:
        return "Structures";
    case ShipClass::Unknown:
        break;
    }

    return "Other objects";
}

std::string DScanAnalyzer::FormatShipList(const DScanResult& Result)
{
    std::string List;
    for (const DScanShipCount& Ship : Result.Ships)
    {
        if (List.empty() == false)
        {
            List += "\n";
        }

        List += std::to_string(Ship.Count) + "x " + Ship.TypeName;
    }

    return List;
}

bool DScanAnalyzer::TryGetDanger(const std::string& Group, std::string* const Reason, int* const Severity)
{
    const GroupRule* const Rule = FindRule(Group);
    if (Rule == nullptr || Rule->Danger == nullptr)
    {
        return false;
    }

    *Reason = Rule->Danger;
    *Severity = Rule->Severity;
    return true;
}

bool DScanAnalyzer::TryParseDistance(const std::string& Text, double* const Meters)
{
    std::string Trimmed = TextUtil::Trim(Text);
    // The game may separate the unit with a no-break space
    const std::string NoBreakSpace = "\xC2\xA0";
    for (size_t Position = Trimmed.find(NoBreakSpace); Position != std::string::npos; Position = Trimmed.find(NoBreakSpace))
    {
        Trimmed.replace(Position, NoBreakSpace.size(), " ");
    }

    const size_t UnitStart = Trimmed.find_last_of(' ');
    if (UnitStart == std::string::npos)
    {
        return false;
    }

    const std::string Unit = TextUtil::ToLower(Trimmed.substr(UnitStart + 1));
    std::string Number = TextUtil::Trim(Trimmed.substr(0, UnitStart));
    double Scale = 0.0;
    if (Unit == "au")
    {
        Scale = METERS_PER_AU;
        std::replace(Number.begin(), Number.end(), ',', '.');
    }
    else if (Unit == "km" || Unit == "m")
    {
        Scale = Unit == "km" ? METERS_PER_KILOMETER : 1.0;
        Number.erase(std::remove_if(Number.begin(), Number.end(), [](const char Character)
        {
            return Character == ',' || Character == '.' || Character == ' ';
        }), Number.end());
    }
    else
    {
        return false;
    }

    if (Number.empty() == true)
    {
        return false;
    }

    char* End = nullptr;
    const double Value = std::strtod(Number.c_str(), &End);
    if (End == Number.c_str() || *End != '\0')
    {
        return false;
    }

    *Meters = Value * Scale;
    return true;
}

std::string DScanAnalyzer::FormatDistance(const double Meters)
{
    char Text[32] = {};
    if (Meters < 0.0)
    {
        return "?";
    }

    if (Meters >= METERS_PER_AU * 0.1)
    {
        std::snprintf(Text, sizeof(Text), "%.1f AU", Meters / METERS_PER_AU);
    }
    else if (Meters >= METERS_PER_KILOMETER)
    {
        std::snprintf(Text, sizeof(Text), "%.0f km", Meters / METERS_PER_KILOMETER);
    }
    else
    {
        std::snprintf(Text, sizeof(Text), "%.0f m", Meters);
    }

    return Text;
}

DScanResult DScanAnalyzer::Analyze(const ShipCatalog& Catalog, const std::string& Text)
{
    DScanResult Result;
    std::map<ShipClass, int> ClassTotals;
    std::map<std::string, DScanShipCount> ShipsByType;
    std::map<std::string, DScanFlag> FlagsByType;
    int ScanShapedLines = 0;

    for (const std::string& RawLine : TextUtil::Split(Text, '\n'))
    {
        const std::string Line = TextUtil::Trim(RawLine);
        if (Line.empty() == true)
        {
            continue;
        }

        Result.Lines++;
        const std::vector<std::string> Fields = TextUtil::Split(Line, '\t');
        if (Fields.size() < MINIMUM_FIELDS || IsAllDigits(TextUtil::Trim(Fields[FIELD_ID])) == false)
        {
            continue;
        }

        ScanShapedLines++;
        int Id = 0;
        const ShipType* Type = TextUtil::TryParseInt(TextUtil::Trim(Fields[FIELD_ID]), &Id) == true ? Catalog.FindById(Id) : nullptr;
        if (Type == nullptr)
        {
            Type = Catalog.FindByName(Fields[FIELD_TYPE_NAME]);
        }

        if (Type == nullptr)
        {
            ClassTotals[ShipClass::Unknown]++;
            continue;
        }

        Result.Recognized++;
        ClassTotals[Classify(*Type)]++;
        if (Type->Category == "Ship")
        {
            DScanShipCount& Ship = ShipsByType[Type->Name];
            Ship.TypeName = Type->Name;
            Ship.Group = Type->Group;
            Ship.Count++;
        }

        std::string Reason;
        int Severity = 0;
        if (TryGetDanger(Type->Group, &Reason, &Severity) == false)
        {
            continue;
        }

        DScanFlag& Flag = FlagsByType[Type->Name];
        Flag.TypeName = Type->Name;
        Flag.Group = Type->Group;
        Flag.Reason = Reason;
        Flag.Severity = Severity;
        Flag.Count++;

        double Meters = 0.0;
        if (Fields.size() > FIELD_DISTANCE && TryParseDistance(Fields[FIELD_DISTANCE], &Meters) == true)
        {
            Flag.NearestMeters = Flag.NearestMeters < 0.0 ? Meters : std::min(Flag.NearestMeters, Meters);
        }
    }

    Result.LooksLikeScan = Result.Lines > 0 && ScanShapedLines * 10 >= Result.Lines * 6 && Result.Recognized > 0;

    for (const std::pair<const ShipClass, int>& Entry : ClassTotals)
    {
        Result.Classes.push_back(DScanClassCount{Entry.first, Entry.second});
    }

    for (const std::pair<const std::string, DScanShipCount>& Entry : ShipsByType)
    {
        Result.Ships.push_back(Entry.second);
    }

    std::sort(Result.Ships.begin(), Result.Ships.end(), [](const DScanShipCount& Left, const DScanShipCount& Right)
    {
        if (Left.Count != Right.Count)
        {
            return Left.Count > Right.Count;
        }

        return Left.TypeName < Right.TypeName;
    });

    for (const std::pair<const std::string, DScanFlag>& Entry : FlagsByType)
    {
        Result.Flags.push_back(Entry.second);
    }

    std::sort(Result.Flags.begin(), Result.Flags.end(), [](const DScanFlag& Left, const DScanFlag& Right)
    {
        if (Left.Severity != Right.Severity)
        {
            return Left.Severity > Right.Severity;
        }

        return Left.Count > Right.Count;
    });

    return Result;
}
