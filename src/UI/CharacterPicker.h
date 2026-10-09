#pragma once

#include <string>
#include <vector>

// A dropdown of characters where each one has a small button to take it off the list
class CharacterPicker
{
public:
    // True when the selection changed. A character removed with its button is put in Removed; the list itself is left for the caller to update.
    static bool Draw(const char* const Id, const char* const NoneLabel, const float Width, std::string& Selected, const std::vector<std::string>& Characters, std::string& Removed);
};
