#pragma once

#include <filesystem>
#include <string>
#include <vector>

// A settings row for choosing an alert sound: a dropdown of the bundled sounds plus a custom file option and a test button
class SoundPicker
{
public:
    // Path is empty for the default sound, a bare file name for a bundled one, or a full path for a custom file. For an optional
    // sound an empty path means "same as the alert sound". Returns true when the chosen sound changed; TestPressed is set when
    // the test button was clicked.
    bool Draw(const char* const Label, const char* const Id, std::string& Path, const bool IsOptional, bool& TestPressed);

private:
    // "Default Warning" from defaultWarning.wav
    static std::string DisplayName(const std::filesystem::path& Sound);

    static std::string GetLabel(const std::string& Path, const bool IsOptional);
    static bool IsSelected(const std::string& Path, const std::filesystem::path& Bundled);
    static bool Browse(std::string& Path);

    std::vector<std::filesystem::path> BundledSounds;
};
