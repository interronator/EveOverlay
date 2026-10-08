#pragma once

#include <filesystem>
#include <map>
#include <string>

#include <nlohmann/json.hpp>

#include "Config/Geometry.h"
#include "Config/ThumbnailConfiguration.h"

class ConfigurationStorage
{
public:
    using Json = nlohmann::ordered_json;

    ConfigurationStorage(ThumbnailConfiguration& ConfigurationReference, const std::filesystem::path& FilePath);

    // Properties are applied in file order because two of the setters clear collections as a side effect
    void Load();
    bool Save() const;
    Json BuildJson() const;

private:
    struct BoolProperty
    {
        const char* Key;
        bool ThumbnailConfiguration::* Member;
    };

    struct IntProperty
    {
        const char* Key;
        int ThumbnailConfiguration::* Member;
    };

    struct SizeProperty
    {
        const char* Key;
        Size ThumbnailConfiguration::* Member;
    };

    static const BoolProperty BOOL_PROPERTIES[];
    static const IntProperty INT_PROPERTIES[];
    static const SizeProperty SIZE_PROPERTIES[];

    static bool TryReadInt(const Json& Value, int* const Result);
    static bool TryReadPoint(const Json& Value, Point* const Result);
    static Json BuildIdentifierMap(const std::map<long long, std::string>& Source);
    static void LoadIdentifierMap(const Json& Value, std::map<long long, std::string>& Target);

    void ApplyProperty(const std::string& Key, const Json& Value);
    void LoadLayoutPresets(const Json& Value);
    void LoadPerClientLayout(const Json& Value);
    void LoadFlatLayout(const Json& Value);
    void LoadClientLayouts(const Json& Value);
    void LoadClientHotkeys(const Json& Value);
    void LoadDisabledThumbnails(const Json& Value);
    void LoadSavedChannels(const Json& Value);
    void LoadPriorityClients(const Json& Value);

    ThumbnailConfiguration& Configuration;
    std::filesystem::path ConfigurationFilePath;
};
