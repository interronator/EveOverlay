#include "Universe/IntelParser.h"

#include <cctype>

std::vector<int> IntelParser::FindSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates)
{
    std::vector<std::string> Words = Tokenize(Message);
    std::vector<int> Found;
    std::unordered_set<int> Seen;

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

        if (Seen.insert(MatchedSystem).second == true)
        {
            Found.push_back(MatchedSystem);
        }

        Start += MatchedLength;
    }

    return Found;
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
