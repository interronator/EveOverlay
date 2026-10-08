#include "Universe/IntelParser.h"

#include <algorithm>
#include <cctype>

#include "Config/TextUtil.h"

std::vector<int> IntelParser::FindSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates, const bool IgnoreClear)
{
    std::vector<int> Found;
    if (IgnoreClear == true && IsStatusRequest(Message) == true)
    {
        return Found;
    }

    std::unordered_set<int> Seen;
    for (const SystemMatch& Match : ScanSystems(Data, Message, Candidates))
    {
        if (IgnoreClear == true && Match.ReportedClear == true)
        {
            continue;
        }

        if (Seen.insert(Match.System).second == true)
        {
            Found.push_back(Match.System);
        }
    }

    return Found;
}

std::vector<int> IntelParser::FindClearedSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates)
{
    std::vector<int> Cleared;
    if (IsStatusRequest(Message) == true)
    {
        return Cleared;
    }

    const std::vector<SystemMatch> Matches = ScanSystems(Data, Message, Candidates);
    std::unordered_set<int> Hostile;
    for (const SystemMatch& Match : Matches)
    {
        if (Match.ReportedClear == false)
        {
            Hostile.insert(Match.System);
        }
    }

    for (const SystemMatch& Match : Matches)
    {
        if (Match.ReportedClear == false || Hostile.contains(Match.System) == true)
        {
            continue;
        }

        if (std::find(Cleared.begin(), Cleared.end(), Match.System) == Cleared.end())
        {
            Cleared.push_back(Match.System);
        }
    }

    return Cleared;
}

std::vector<IntelParser::SystemMatch> IntelParser::ScanSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates)
{
    std::vector<SystemMatch> Matches;
    const std::vector<std::string> Words = Tokenize(Message);

    size_t Start = 0;
    while (Start < Words.size())
    {
        size_t MatchedLength = 0;
        int MatchedSystem = UniverseData::NOT_FOUND;
        std::string Phrase;
        for (size_t Length = 1; Length <= MAX_NAME_WORDS && Start + Length <= Words.size(); Length++)
        {
            Phrase += Length == 1 ? Words[Start] : " " + Words[Start + Length - 1];
            const int System = Data.Find(Phrase);
            if (System == UniverseData::NOT_FOUND || Candidates.contains(System) == false)
            {
                continue;
            }

            MatchedLength = Length;
            MatchedSystem = System;
        }

        if (MatchedLength == 0)
        {
            Start++;
            continue;
        }

        SystemMatch Match;
        Match.System = MatchedSystem;
        Match.ReportedClear = IsClearMarker(Words, Start + MatchedLength);
        Matches.push_back(Match);
        Start += MatchedLength;
    }

    return Matches;
}

bool IntelParser::IsHarmlessReport(const ShipCatalog& Ships, const std::string& Message)
{
    const std::vector<std::string> Words = Tokenize(Message);
    bool NamedHarmless = false;

    size_t Start = 0;
    while (Start < Words.size())
    {
        size_t MatchedLength = 0;
        bool MatchedHarmless = false;
        std::string Phrase;
        for (size_t Length = 1; Length <= MAX_NAME_WORDS && Start + Length <= Words.size(); Length++)
        {
            Phrase += Length == 1 ? Words[Start] : " " + Words[Start + Length - 1];
            const ShipType* const Ship = Ships.FindByName(Phrase);
            if (Ship != nullptr && Ship->Category == "Ship")
            {
                MatchedLength = Length;
                MatchedHarmless = IsHarmlessGroup(Ship->Group);
            }
        }

        if (MatchedLength == 0 && IsHarmlessAlias(Words[Start]) == true)
        {
            MatchedLength = 1;
            MatchedHarmless = true;
        }

        if (MatchedLength == 0)
        {
            Start++;
            continue;
        }

        if (MatchedHarmless == false)
        {
            return false;
        }

        NamedHarmless = true;
        Start += MatchedLength;
    }

    return NamedHarmless;
}

bool IntelParser::IsHarmlessGroup(const std::string& Group)
{
    static const std::unordered_set<std::string> HARMLESS_GROUPS = {
        "Shuttle", "Capsule", "Corvette", "Hauler", "Freighter", "Jump Freighter", "Deep Space Transport", "Blockade Runner",
        "Mining Barge", "Exhumer", "Industrial Command Ship", "Capital Industrial Ship", "Special Edition Yachts"};
    return HARMLESS_GROUPS.contains(Group);
}

bool IntelParser::IsHarmlessAlias(const std::string& Word)
{
    static const std::unordered_set<std::string> HARMLESS_WORDS = {"shuttle", "shuttles", "pod", "pods", "capsule", "capsules", "hauler", "haulers", "miner", "miners", "freighter", "freighters", "rookie"};
    return HARMLESS_WORDS.contains(TextUtil::ToLower(Word));
}

std::vector<std::string> IntelParser::ParseKeywordList(const std::string& Text)
{
    std::string Separated = Text;
    for (char& Character : Separated)
    {
        if (Character == ';' || Character == '\n')
        {
            Character = ',';
        }
    }

    std::vector<std::string> Keywords;
    for (const std::string& Part : TextUtil::Split(Separated, ','))
    {
        const std::string Keyword = TextUtil::ToLower(TextUtil::Trim(Part));
        if (Keyword.empty() == true)
        {
            continue;
        }

        if (std::find(Keywords.begin(), Keywords.end(), Keyword) == Keywords.end())
        {
            Keywords.push_back(Keyword);
        }
    }

    return Keywords;
}

bool IntelParser::ContainsKeyword(const std::string& Message, const std::vector<std::string>& Keywords)
{
    const std::string Lowered = TextUtil::ToLower(Message);
    for (const std::string& Keyword : Keywords)
    {
        size_t Position = Lowered.find(Keyword);
        while (Position != std::string::npos)
        {
            const size_t After = Position + Keyword.size();
            const bool StartsWord = Position == 0 || IsNameCharacter(static_cast<unsigned char>(Lowered[Position - 1])) == false;
            const bool EndsWord = After >= Lowered.size() || IsNameCharacter(static_cast<unsigned char>(Lowered[After])) == false;
            if (StartsWord == true && EndsWord == true)
            {
                return true;
            }

            Position = Lowered.find(Keyword, Position + 1);
        }
    }

    return false;
}

bool IntelParser::IsStatusRequest(const std::string& Message)
{
    const std::string Trimmed = TextUtil::Trim(Message);
    if (Trimmed.empty() == false && Trimmed.back() == '?')
    {
        return true;
    }

    for (const std::string& Word : Tokenize(TextUtil::ToLower(Trimmed)))
    {
        if (Word == "status")
        {
            return true;
        }
    }

    return false;
}

bool IntelParser::IsClearMarker(const std::vector<std::string>& Words, const size_t Position)
{
    if (Position >= Words.size())
    {
        return false;
    }

    const std::string Word = TextUtil::ToLower(Words[Position]);
    if (Word == "clr" || Word == "clear" || Word == "cleared" || Word == "nv")
    {
        return true;
    }

    return Word == "no" && Position + 1 < Words.size() && TextUtil::ToLower(Words[Position + 1]) == "visual";
}

bool IntelParser::IsNameCharacter(const unsigned char Character)
{
    return Character >= 0x80 || std::isalnum(Character) != 0 || Character == '-';
}

std::vector<std::string> IntelParser::Tokenize(const std::string& Message)
{
    std::vector<std::string> Words;
    std::string Current;
    for (const char Character : Message)
    {
        if (IsNameCharacter(static_cast<unsigned char>(Character)) == true)
        {
            Current += Character;
            continue;
        }

        if (Current.empty() == false)
        {
            Words.push_back(Current);
            Current.clear();
        }
    }

    if (Current.empty() == false)
    {
        Words.push_back(Current);
    }

    return Words;
}
