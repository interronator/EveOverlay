#pragma once

#include <filesystem>

class AppPaths
{
public:
    static std::filesystem::path GetExecutableDirectory();
    static std::filesystem::path GetConfigurationFilePath();
    static std::filesystem::path GetUniverseDataPath();
    static std::filesystem::path GetShipDataPath();
    static std::filesystem::path GetUniverseMapSettingsPath();
    static std::filesystem::path GetLogDirectory();
};
