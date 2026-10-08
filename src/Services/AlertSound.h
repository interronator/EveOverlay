#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <Windows.h>

// Plays one sound file at a time through MCI, which handles wav, mp3 and wma and, unlike PlaySound, has a volume control
class AlertSound
{
public:
    AlertSound() = default;
    AlertSound(const AlertSound&) = delete;
    AlertSound& operator=(const AlertSound&) = delete;

    ~AlertSound();

    // The bundled armor warning next to the exe, or a Windows alarm if that file has been removed
    static std::filesystem::path GetDefaultSoundPath();

    // Every sound file in the sounds folder next to the exe, sorted by file name
    static std::vector<std::filesystem::path> GetBundledSounds();

    // Setting is empty for the default sound, a bare file name for a bundled one, or a full path for a custom file. Bundled
    // sounds are cut to one second. A missing file falls back to the default sound, and then to the system beep.
    void Play(const std::wstring& Setting, const int VolumePercent);
    void Close();

private:
    static constexpr const wchar_t* ALIAS = L"EveOverlayAlert";
    static constexpr int BUNDLED_SOUND_LENGTH_MS = 1000;

    bool IsOpen = false;
};
