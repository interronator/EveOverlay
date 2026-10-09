#pragma once

#include <string>
#include <vector>

#include "Universe/ShipCatalog.h"

enum class ShipClass
{
    Capital,
    Combat,
    Recon,
    Logistics,
    Industrial,
    OtherShip,
    Drone,
    Npc,
    Structure,
    Unknown
};

// A kind of ship worth a second look, with how many were seen and how close the nearest one is
struct DScanFlag
{
    std::string TypeName;
    std::string Group;
    std::string Reason;
    int Count = 0;
    int Severity = 0;

    // Negative when the scan did not give a distance
    double NearestMeters = -1.0;
};

struct DScanClassCount
{
    ShipClass Class = ShipClass::Unknown;
    int Count = 0;
};

// How many of one kind of ship were on scan, such as 3 Hurricane, with the group it belongs to
struct DScanShipCount
{
    std::string TypeName;
    std::string Group;
    int Count = 0;
};

struct DScanResult
{
    int Lines = 0;
    int Recognized = 0;
    bool LooksLikeScan = false;
    std::vector<DScanClassCount> Classes;

    // Every kind of ship on scan with how many, the most common first; drones, NPCs and structures are left out
    std::vector<DScanShipCount> Ships;

    // Most worrying first
    std::vector<DScanFlag> Flags;
};

// Turns the text copied from the directional scan window into a short summary. Each line is "type id, name, type name, distance"
// separated by tabs; the type id is used to tell what it is, so the game language does not matter.
class DScanAnalyzer
{
public:
    static DScanResult Analyze(const ShipCatalog& Catalog, const std::string& Text);

    static ShipClass Classify(const ShipType& Type);
    static const char* GetClassLabel(const ShipClass Class);

    // One line per kind of ship, most common first, such as "2x Hurricane"; empty when no ships were on scan
    static std::string FormatShipList(const DScanResult& Result);

    // False when the group is nothing special; Severity is higher for the more worrying ones
    static bool TryGetDanger(const std::string& Group, std::string* const Reason, int* const Severity);

    // "2.3 AU", "1,234 km" or "500 m"; false for "-" and anything else
    static bool TryParseDistance(const std::string& Text, double* const Meters);
    static std::string FormatDistance(const double Meters);

private:
    struct GroupRule
    {
        const char* Group;
        ShipClass Class;
        const char* Danger;
        int Severity;
    };

    static const GroupRule GROUP_RULES[];
    static const GroupRule* FindRule(const std::string& Group);
    static bool IsAllDigits(const std::string& Text);

    static constexpr double METERS_PER_KILOMETER = 1000.0;
    static constexpr double METERS_PER_AU = 149597870700.0;
    static constexpr size_t FIELD_ID = 0;
    static constexpr size_t FIELD_TYPE_NAME = 2;
    static constexpr size_t FIELD_DISTANCE = 3;
    static constexpr size_t MINIMUM_FIELDS = 3;
};
