#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Universe/ShipCatalog.h"
#include "Universe/UniverseData.h"

class IntelParser
{
public:
    // Returns the distinct systems named in Message that are also in Candidates. Restricting to the candidates keeps
    // short or common-word system names from matching ordinary chat outside the area being watched. With IgnoreClear, a system
    // followed by "clr", "clear" or "nv" is skipped, and a message that asks a question or for a status is ignored entirely.
    static std::vector<int> FindSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates, const bool IgnoreClear = false);

    // Returns the distinct systems that Message reports as clear ("Jita clr", "Jita no visual"). Status requests and questions
    // report nothing. A system that Message also reports as hostile is left out, since the message then does not clear it for sure.
    static std::vector<int> FindClearedSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates);

    // True when Message names at least one ship and every ship it names is harmless: a shuttle, pod, rookie ship, hauler, miner
    // and so on. A single combat ship, or any other ship the catalog does not class as harmless, makes the report a real threat.
    static bool IsHarmlessReport(const ShipCatalog& Ships, const std::string& Message);

    // Splits "bubble, gate camp;cyno" into lowercase entries without blanks or duplicates
    static std::vector<std::string> ParseKeywordList(const std::string& Text);

    // True when any keyword (a word or a phrase) appears in Message as whole words, ignoring case
    static bool ContainsKeyword(const std::string& Message, const std::vector<std::string>& Keywords);

private:
    static constexpr size_t MAX_NAME_WORDS = 4;

    static bool IsHarmlessGroup(const std::string& Group);
    static bool IsHarmlessAlias(const std::string& Word);
    static bool IsThreatWord(const std::string& Word);

    struct SystemMatch
    {
        int System = UniverseData::NOT_FOUND;
        bool ReportedClear = false;
    };

    static std::vector<SystemMatch> ScanSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates);
    static bool IsNameCharacter(const unsigned char Character);
    static bool IsStatusRequest(const std::string& Message);
    static bool IsClearMarker(const std::vector<std::string>& Words, const size_t Position);
    static std::vector<std::string> Tokenize(const std::string& Message);
};
