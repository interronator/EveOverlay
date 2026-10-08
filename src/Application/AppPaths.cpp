#include "Application/AppPaths.h"

#include <string>

#include <Windows.h>

std::filesystem::path AppPaths::GetExecutableDirectory()
{
    std::wstring ModulePath(MAX_PATH, L'\0');
    while (true)
    {
        const DWORD Length = ::GetModuleFileNameW(nullptr, ModulePath.data(), static_cast<DWORD>(ModulePath.size()));
        if (Length < ModulePath.size())
        {
            ModulePath.resize(Length);
            break;
        }

        ModulePath.resize(ModulePath.size() * 2);
    }

    return std::filesystem::path(ModulePath).parent_path();
}

std::filesystem::path AppPaths::GetConfigurationFilePath()
{
    return GetExecutableDirectory() / L"Eve Overlay.json";
}

std::filesystem::path AppPaths::GetUniverseDataPath()
{
    return GetExecutableDirectory() / L"universeData" / L"systems.csv";
}

std::filesystem::path AppPaths::GetUniverseMapSettingsPath()
{
    return GetExecutableDirectory() / L"UniverseMap.ini";
}

std::filesystem::path AppPaths::GetLogDirectory()
{
    return GetExecutableDirectory() / L"logs";
}
