#pragma once

#include <string>

struct UniverseMapOptions
{
    std::string System = "Jita";
    int Jumps = 3;
    std::string IntelChannel = "Intel";
    bool AlwaysOnTop = true;
    bool SmartMap = false;
    int Size = 600;
    int Rotation = 0;
    bool SoundEnabled = true;
    int SoundVolume = 70;
    int AlertSeconds = 300;
    std::string SoundPath;
    bool IgnoreClear = true;
    bool ScaleVolumeByDistance = true;
    bool FollowLocation = false;
    std::string LocationCharacter;
    bool UseJumpBridges = true;
    std::string Keywords;
    std::string KeywordSoundPath;
};
