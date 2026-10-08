#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "Universe/UniverseData.h"

class IntelParser
{
public:
    // Returns the distinct systems named in Message that are also in Candidates. Restricting to the candidates keeps
    // short or common-word system names from matching ordinary chat outside the area being watched.
    static std::vector<int> FindSystems(const UniverseData& Data, const std::string& Message, const std::unordered_set<int>& Candidates);

private:
    static constexpr size_t MAX_NAME_WORDS = 4;

    static bool IsNameCharacter(const unsigned char Character);
    static std::vector<std::string> Tokenize(const std::string& Message);
};
