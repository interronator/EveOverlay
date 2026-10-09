#include "Config/ConfigurationStorage.h"

#include <charconv>
#include <fstream>
#include <iterator>
#include <limits>

#include "Application/Logger.h"
#include "Config/TextUtil.h"

const ConfigurationStorage::BoolProperty ConfigurationStorage::BOOL_PROPERTIES[] = {
    {"MinimizeToTray", &ThumbnailConfiguration::MinimizeToTray},
    {"MainWindowMaximized", &ThumbnailConfiguration::MainWindowMaximized},
    {"MainWindowAlwaysOnTop", &ThumbnailConfiguration::MainWindowAlwaysOnTop},
    {"LightTheme", &ThumbnailConfiguration::LightTheme},
    {"ShowDevelopingTabs", &ThumbnailConfiguration::ShowDevelopingTabs},
    {"CompatibilityMode", &ThumbnailConfiguration::EnableCompatibilityMode},
    {"HideActiveClientThumbnail", &ThumbnailConfiguration::HideActiveClientThumbnail},
    {"MinimizeInactiveClients", &ThumbnailConfiguration::MinimizeInactiveClients},
    {"ShowThumbnailsAlwaysOnTop", &ThumbnailConfiguration::ShowThumbnailsAlwaysOnTop},
    {"HideThumbnailsOnLostFocus", &ThumbnailConfiguration::HideThumbnailsOnLostFocus},
    {"EnableThumbnailSnap", &ThumbnailConfiguration::EnableThumbnailSnap},
    {"MoveAllThumbnails", &ThumbnailConfiguration::MoveAllThumbnails},
    {"LockThumbnails", &ThumbnailConfiguration::LockThumbnails},
    {"ThumbnailOrganizerEnabled", &ThumbnailConfiguration::OrganizerEnabled},
    {"OrganizerSmartStart", &ThumbnailConfiguration::OrganizerSmartStart},
    {"EnableThumbnailZoom", &ThumbnailConfiguration::ThumbnailZoomEnabled},
    {"ShowThumbnailOverlays", &ThumbnailConfiguration::ShowThumbnailOverlays},
    {"ShowThumbnailFrames", &ThumbnailConfiguration::ShowThumbnailFrames},
    {"EnableActiveClientHighlight", &ThumbnailConfiguration::EnableActiveClientHighlight},
    {"UniverseMapAlwaysOnTop", &ThumbnailConfiguration::UniverseMapAlwaysOnTop},
    {"UniverseMapVisible", &ThumbnailConfiguration::UniverseMapVisible},
    {"UniverseSmartMap", &ThumbnailConfiguration::UniverseSmartMap},
    {"UniverseAlertSoundEnabled", &ThumbnailConfiguration::UniverseAlertSoundEnabled},
    {"UniverseIgnoreClear", &ThumbnailConfiguration::UniverseIgnoreClear},
    {"UniverseScaleVolumeByDistance", &ThumbnailConfiguration::UniverseScaleVolumeByDistance},
    {"UniverseFollowLocation", &ThumbnailConfiguration::UniverseFollowLocation},
    {"AttackAlertsEnabled", &ThumbnailConfiguration::AttackAlertsEnabled},
    {"AttackAlertOnDamage", &ThumbnailConfiguration::AttackAlertOnDamage},
    {"AttackAlertOnWarpDisruption", &ThumbnailConfiguration::AttackAlertOnWarpDisruption},
    {"AttackAlertSoundEnabled", &ThumbnailConfiguration::AttackAlertSoundEnabled},
    {"DScanAutoRead", &ThumbnailConfiguration::DScanAutoRead},
    {"DScanMarksScanAge", &ThumbnailConfiguration::DScanMarksScanAge},
    {"DScanCopyShipList", &ThumbnailConfiguration::DScanCopyShipList},
    {"TimerWindowEnabled", &ThumbnailConfiguration::TimerWindowEnabled},
    {"TimerShowScanAge", &ThumbnailConfiguration::TimerShowScanAge},
    {"TimerSoundEnabled", &ThumbnailConfiguration::TimerSoundEnabled},
};

const ConfigurationStorage::StringProperty ConfigurationStorage::STRING_PROPERTIES[] = {
    {"MainCharacter", &ThumbnailConfiguration::MainCharacter},
    {"UniverseLocationCharacter", &ThumbnailConfiguration::UniverseLocationCharacter},
    {"UniverseSystem", &ThumbnailConfiguration::UniverseSystem},
    {"UniverseIntelChannel", &ThumbnailConfiguration::UniverseIntelChannel},
    {"UniverseAlertSoundPath", &ThumbnailConfiguration::UniverseAlertSoundPath},
    {"UniverseKeywords", &ThumbnailConfiguration::UniverseKeywords},
    {"UniverseKeywordSoundPath", &ThumbnailConfiguration::UniverseKeywordSoundPath},
    {"AttackAlertSoundPath", &ThumbnailConfiguration::AttackAlertSoundPath},
    {"TogglePreviewsHotkey", &ThumbnailConfiguration::TogglePreviewsHotkey},
    {"MinimizeAllHotkey", &ThumbnailConfiguration::MinimizeAllHotkey},
    {"ScanHotkey", &ThumbnailConfiguration::ScanHotkey},
    {"QuickTimerHotkey", &ThumbnailConfiguration::QuickTimerHotkey},
    {"TimerSoundPath", &ThumbnailConfiguration::TimerSoundPath},
    {"SyncerUserFilePath", &ThumbnailConfiguration::SyncerUserFilePath},
    {"SyncerCharacterFilePath", &ThumbnailConfiguration::SyncerCharacterFilePath},
};

const ConfigurationStorage::IntProperty ConfigurationStorage::INT_PROPERTIES[] = {
    {"ThumbnailRefreshPeriod", &ThumbnailConfiguration::ThumbnailRefreshPeriod},
    {"MainWindowWidth", &ThumbnailConfiguration::MainWindowWidth},
    {"MainWindowHeight", &ThumbnailConfiguration::MainWindowHeight},
    {"HideThumbnailsDelay", &ThumbnailConfiguration::HideThumbnailsDelay},
    {"ThumbnailZoomFactor", &ThumbnailConfiguration::ThumbnailZoomFactor},
    {"ActiveClientHighlightThickness", &ThumbnailConfiguration::ActiveClientHighlightThickness},
    {"UniverseJumps", &ThumbnailConfiguration::UniverseJumps},
    {"UniverseMapSize", &ThumbnailConfiguration::UniverseMapSize},
    {"UniverseMapRotation", &ThumbnailConfiguration::UniverseMapRotation},
    {"UniverseAlertVolume", &ThumbnailConfiguration::UniverseAlertVolume},
    {"UniverseAlertTimeout", &ThumbnailConfiguration::UniverseAlertTimeout},
    {"AttackAlertVolume", &ThumbnailConfiguration::AttackAlertVolume},
    {"AttackAlertSeconds", &ThumbnailConfiguration::AttackAlertSeconds},
    {"DScanShowSeconds", &ThumbnailConfiguration::DScanShowSeconds},
    {"QuickTimerSeconds", &ThumbnailConfiguration::QuickTimerSeconds},
    {"TimerVolume", &ThumbnailConfiguration::TimerVolume},
};

const ConfigurationStorage::SizeProperty ConfigurationStorage::SIZE_PROPERTIES[] = {
    {"ThumbnailSize", &ThumbnailConfiguration::ThumbnailSize},
    {"ThumbnailMaximumSize", &ThumbnailConfiguration::ThumbnailMaximumSize},
    {"ThumbnailMinimumSize", &ThumbnailConfiguration::ThumbnailMinimumSize},
};

ConfigurationStorage::ConfigurationStorage(ThumbnailConfiguration& ConfigurationReference, const std::filesystem::path& FilePath)
    : Configuration(ConfigurationReference)
    , ConfigurationFilePath(FilePath)
{
}

void ConfigurationStorage::Load()
{
    std::ifstream Stream(ConfigurationFilePath, std::ios::binary);
    if (Stream.is_open() == false)
    {
        return;
    }

    std::string RawData((std::istreambuf_iterator<char>(Stream)), std::istreambuf_iterator<char>());
    if (RawData.starts_with("\xEF\xBB\xBF") == true)
    {
        RawData.erase(0, 3);
    }

    const Json Root = Json::parse(RawData, nullptr, false);
    if (Root.is_object() == false)
    {
        // Keep the unreadable file, otherwise the next save replaces it with defaults
        std::filesystem::path BackupPath = ConfigurationFilePath;
        BackupPath += L".bak";
        std::error_code Error;
        std::filesystem::copy_file(ConfigurationFilePath, BackupPath, std::filesystem::copy_options::overwrite_existing, Error);
        Logger::Error("The configuration file is not valid JSON, defaults are used; the original was copied to " + Logger::DescribePath(BackupPath));
        return;
    }

    for (Json::const_iterator Property = Root.begin(); Property != Root.end(); ++Property)
    {
        ApplyProperty(Property.key(), Property.value());
    }

    // Keys load alphabetically, so the layouts can arrive after the flag that disables them
    Configuration.SetPerClientThumbnailLayoutsEnabled(Configuration.IsPerClientThumbnailLayoutsEnabled());

    Configuration.ApplyRestrictions();
}

bool ConfigurationStorage::Save() const
{
    const std::string RawData = BuildJson().dump(2, ' ', false, Json::error_handler_t::replace);

    std::filesystem::path TemporaryPath = ConfigurationFilePath;
    TemporaryPath += L".tmp";

    {
        std::ofstream Stream(TemporaryPath, std::ios::binary | std::ios::trunc);
        if (Stream.is_open() == false)
        {
            Logger::Error("Could not open " + Logger::DescribePath(TemporaryPath) + " to save the configuration");
            return false;
        }

        Stream << RawData;
        Stream.flush();
        if (Stream.good() == false)
        {
            Logger::Error("Could not write the configuration to " + Logger::DescribePath(TemporaryPath));
            return false;
        }
    }

    std::error_code Error;
    std::filesystem::rename(TemporaryPath, ConfigurationFilePath, Error);
    if (Error.value() != 0)
    {
        Logger::Error("Could not replace " + Logger::DescribePath(ConfigurationFilePath) + ": " + Error.message());
        std::filesystem::remove(TemporaryPath, Error);
        return false;
    }

    return true;
}

ConfigurationStorage::Json ConfigurationStorage::BuildJson() const
{
    Json Root = Json::object();

    Root["MinimizeToTray"] = Configuration.MinimizeToTray;
    Root["MainWindowWidth"] = Configuration.MainWindowWidth;
    Root["MainWindowHeight"] = Configuration.MainWindowHeight;
    Root["MainWindowMaximized"] = Configuration.MainWindowMaximized;
    Root["MainWindowAlwaysOnTop"] = Configuration.MainWindowAlwaysOnTop;
    Root["LightTheme"] = Configuration.LightTheme;
    Root["ShowDevelopingTabs"] = Configuration.ShowDevelopingTabs;
    Root["ThumbnailRefreshPeriod"] = Configuration.ThumbnailRefreshPeriod;
    Root["CompatibilityMode"] = Configuration.EnableCompatibilityMode;
    Root["ThumbnailsOpacity"] = Configuration.ThumbnailOpacity;
    Root["EnableClientLayoutTracking"] = Configuration.ClientLayoutTrackingEnabled;
    Root["HideActiveClientThumbnail"] = Configuration.HideActiveClientThumbnail;
    Root["MinimizeInactiveClients"] = Configuration.MinimizeInactiveClients;
    Root["ShowThumbnailsAlwaysOnTop"] = Configuration.ShowThumbnailsAlwaysOnTop;
    Root["EnablePerClientThumbnailLayouts"] = Configuration.PerClientThumbnailLayoutsEnabled;
    Root["HideThumbnailsOnLostFocus"] = Configuration.HideThumbnailsOnLostFocus;
    Root["HideThumbnailsDelay"] = Configuration.HideThumbnailsDelay;
    Root["ThumbnailSize"] = Configuration.ThumbnailSize.ToString();
    Root["ThumbnailMaximumSize"] = Configuration.ThumbnailMaximumSize.ToString();
    Root["ThumbnailMinimumSize"] = Configuration.ThumbnailMinimumSize.ToString();
    Root["EnableThumbnailSnap"] = Configuration.EnableThumbnailSnap;
    Root["MoveAllThumbnails"] = Configuration.MoveAllThumbnails;
    Root["LockThumbnails"] = Configuration.LockThumbnails;
    Root["ThumbnailOrganizerEnabled"] = Configuration.OrganizerEnabled;
    Root["OrganizerSmartStart"] = Configuration.OrganizerSmartStart;
    Root["EnableThumbnailZoom"] = Configuration.ThumbnailZoomEnabled;
    Root["ThumbnailZoomFactor"] = Configuration.ThumbnailZoomFactor;
    Root["ThumbnailZoomAnchor"] = static_cast<int>(Configuration.ThumbnailZoomAnchor);
    Root["ShowThumbnailOverlays"] = Configuration.ShowThumbnailOverlays;
    Root["ShowThumbnailFrames"] = Configuration.ShowThumbnailFrames;
    Root["EnableActiveClientHighlight"] = Configuration.EnableActiveClientHighlight;
    Root["ActiveClientHighlightColor"] = Configuration.ActiveClientHighlightColor.ToString();
    Root["ActiveClientHighlightThickness"] = Configuration.ActiveClientHighlightThickness;
    for (const StringProperty& Property : STRING_PROPERTIES)
    {
        Root[Property.Key] = Configuration.*Property.Member;
    }

    Root["UniverseJumps"] = Configuration.UniverseJumps;
    Root["UniverseMapAlwaysOnTop"] = Configuration.UniverseMapAlwaysOnTop;
    Root["UniverseMapSize"] = Configuration.UniverseMapSize;
    Root["UniverseMapRotation"] = Configuration.UniverseMapRotation;
    Root["UniverseMapVisible"] = Configuration.UniverseMapVisible;
    Root["UniverseSmartMap"] = Configuration.UniverseSmartMap;
    Root["UniverseAlertSoundEnabled"] = Configuration.UniverseAlertSoundEnabled;
    Root["UniverseAlertVolume"] = Configuration.UniverseAlertVolume;
    Root["UniverseAlertTimeout"] = Configuration.UniverseAlertTimeout;
    Root["UniverseIgnoreClear"] = Configuration.UniverseIgnoreClear;
    Root["UniverseScaleVolumeByDistance"] = Configuration.UniverseScaleVolumeByDistance;
    Root["UniverseFollowLocation"] = Configuration.UniverseFollowLocation;
    Root["AttackAlertsEnabled"] = Configuration.AttackAlertsEnabled;
    Root["AttackAlertOnDamage"] = Configuration.AttackAlertOnDamage;
    Root["AttackAlertOnWarpDisruption"] = Configuration.AttackAlertOnWarpDisruption;
    Root["AttackAlertSoundEnabled"] = Configuration.AttackAlertSoundEnabled;
    Root["AttackAlertVolume"] = Configuration.AttackAlertVolume;
    Root["AttackAlertSeconds"] = Configuration.AttackAlertSeconds;
    Root["DScanAutoRead"] = Configuration.DScanAutoRead;
    Root["DScanShowSeconds"] = Configuration.DScanShowSeconds;
    Root["DScanMarksScanAge"] = Configuration.DScanMarksScanAge;
    Root["DScanCopyShipList"] = Configuration.DScanCopyShipList;
    Root["TimerWindowEnabled"] = Configuration.TimerWindowEnabled;
    Root["TimerShowScanAge"] = Configuration.TimerShowScanAge;
    Root["TimerSoundEnabled"] = Configuration.TimerSoundEnabled;
    Root["QuickTimerSeconds"] = Configuration.QuickTimerSeconds;
    Root["TimerVolume"] = Configuration.TimerVolume;

    Json PerClientLayout = Json::object();
    for (const std::pair<const std::wstring, std::map<std::wstring, Point>>& Client : Configuration.PerClientLayout)
    {
        Json Layout = Json::object();
        for (const std::pair<const std::wstring, Point>& Thumbnail : Client.second)
        {
            Layout[TextUtil::ToUtf8(Thumbnail.first)] = Thumbnail.second.ToString();
        }

        PerClientLayout[TextUtil::ToUtf8(Client.first)] = Layout;
    }

    Root["CharacterNames"] = BuildIdentifierMap(Configuration.CharacterNames);
    Root["AccountNicknames"] = BuildIdentifierMap(Configuration.AccountNicknames);

    Root["PerClientLayout"] = PerClientLayout;

    Json FlatLayout = Json::object();
    for (const std::pair<const std::wstring, Point>& Thumbnail : Configuration.FlatLayout)
    {
        FlatLayout[TextUtil::ToUtf8(Thumbnail.first)] = Thumbnail.second.ToString();
    }

    Root["FlatLayout"] = FlatLayout;

    Json ClientLayouts = Json::object();
    for (const std::pair<const std::wstring, ClientLayout>& Client : Configuration.ClientLayouts)
    {
        Json Layout = Json::object();
        Layout["X"] = Client.second.X;
        Layout["Y"] = Client.second.Y;
        Layout["Width"] = Client.second.Width;
        Layout["Height"] = Client.second.Height;
        Layout["IsMaximized"] = Client.second.IsMaximized;
        ClientLayouts[TextUtil::ToUtf8(Client.first)] = Layout;
    }

    Root["ClientLayout"] = ClientLayouts;

    Json ClientHotkeys = Json::object();
    for (const std::pair<const std::wstring, std::string>& Client : Configuration.ClientHotkeys)
    {
        ClientHotkeys[TextUtil::ToUtf8(Client.first)] = Client.second;
    }

    Root["ClientHotkey"] = ClientHotkeys;

    Json DisabledThumbnails = Json::object();
    for (const std::pair<const std::wstring, bool>& Client : Configuration.DisabledThumbnails)
    {
        DisabledThumbnails[TextUtil::ToUtf8(Client.first)] = Client.second;
    }

    Root["DisableThumbnail"] = DisabledThumbnails;

    Json PriorityClients = Json::array();
    for (const std::wstring& Client : Configuration.PriorityClients)
    {
        PriorityClients.push_back(TextUtil::ToUtf8(Client));
    }

    Root["PriorityClients"] = PriorityClients;

    Json SavedChannels = Json::array();
    for (const std::string& Channel : Configuration.UniverseSavedChannels)
    {
        SavedChannels.push_back(Channel);
    }

    Root["UniverseSavedChannels"] = SavedChannels;

    Root["KnownCharacters"] = BuildStringList(Configuration.KnownCharacters);
    Root["HiddenCharacters"] = BuildStringList(Configuration.HiddenCharacters);
    Root["OrganizerSlots"] = BuildStringList(Configuration.OrganizerSlots);

    Json CycleGroups = Json::array();
    for (const CycleGroup& Group : Configuration.CycleGroups)
    {
        Json Entry = Json::object();
        Entry["Name"] = Group.Name;
        Json Members = Json::array();
        for (const std::wstring& Member : Group.Members)
        {
            Members.push_back(TextUtil::ToUtf8(Member));
        }

        Entry["Members"] = Members;
        Entry["Next"] = Group.Next.ToString();
        Entry["Previous"] = Group.Previous.ToString();
        CycleGroups.push_back(Entry);
    }

    Root["CycleGroups"] = CycleGroups;

    Json LayoutPresets = Json::array();
    for (const LayoutPreset& Preset : Configuration.LayoutPresets)
    {
        Json Entry = Json::object();
        Entry["Name"] = Preset.Name;
        Entry["ClientCount"] = Preset.ClientCount;
        Entry["Columns"] = Preset.Arrangement.Shape.Columns;
        Entry["Rows"] = Preset.Arrangement.Shape.Rows;
        Entry["PartialRowFirst"] = Preset.Arrangement.Shape.PartialRowFirst;
        Entry["Gap"] = Preset.Arrangement.Gap;
        Entry["Origin"] = Preset.Arrangement.Origin.ToString();
        Entry["Slots"] = BuildStringList(Preset.Arrangement.SlotCharacters);
        LayoutPresets.push_back(Entry);
    }

    Root["LayoutPresets"] = LayoutPresets;

    return Root;
}

bool ConfigurationStorage::TryReadInt(const Json& Value, int* const Result)
{
    if (Value.is_number_integer() == false)
    {
        return false;
    }

    if (Value.is_number_unsigned() == true)
    {
        if (Value.get<unsigned long long>() > static_cast<unsigned long long>(std::numeric_limits<int>::max()))
        {
            return false;
        }
    }
    else if (Value.get<long long>() < std::numeric_limits<int>::min())
    {
        return false;
    }

    *Result = Value.get<int>();
    return true;
}

bool ConfigurationStorage::TryReadPoint(const Json& Value, Point* const Result)
{
    if (Value.is_string() == false)
    {
        return false;
    }

    return Point::TryParse(Value.get<std::string>(), Result);
}

void ConfigurationStorage::ApplyProperty(const std::string& Key, const Json& Value)
{
    for (const BoolProperty& Property : BOOL_PROPERTIES)
    {
        if (Key != Property.Key)
        {
            continue;
        }

        if (Value.is_boolean() == true)
        {
            Configuration.*Property.Member = Value.get<bool>();
        }

        return;
    }

    for (const IntProperty& Property : INT_PROPERTIES)
    {
        if (Key != Property.Key)
        {
            continue;
        }

        int Number = 0;
        if (TryReadInt(Value, &Number) == true)
        {
            Configuration.*Property.Member = Number;
        }

        return;
    }

    for (const SizeProperty& Property : SIZE_PROPERTIES)
    {
        if (Key != Property.Key)
        {
            continue;
        }

        Size Parsed;
        if (Value.is_string() == true && Size::TryParse(Value.get<std::string>(), &Parsed) == true)
        {
            Configuration.*Property.Member = Parsed;
        }

        return;
    }

    for (const StringProperty& Property : STRING_PROPERTIES)
    {
        if (Key != Property.Key)
        {
            continue;
        }

        if (Value.is_string() == true)
        {
            Configuration.*Property.Member = Value.get<std::string>();
        }

        return;
    }

    if (Key == "ThumbnailsOpacity")
    {
        if (Value.is_number() == true)
        {
            Configuration.ThumbnailOpacity = Value.get<double>();
        }

        return;
    }

    if (Key == "EnableClientLayoutTracking")
    {
        if (Value.is_boolean() == true)
        {
            Configuration.SetClientLayoutTrackingEnabled(Value.get<bool>());
        }

        return;
    }

    if (Key == "EnablePerClientThumbnailLayouts")
    {
        if (Value.is_boolean() == true)
        {
            Configuration.SetPerClientThumbnailLayoutsEnabled(Value.get<bool>());
        }

        return;
    }

    if (Key == "ThumbnailZoomAnchor")
    {
        int Number = 0;
        if (TryReadInt(Value, &Number) == true && Number >= static_cast<int>(ZoomAnchor::NW) && Number <= static_cast<int>(ZoomAnchor::SE))
        {
            Configuration.ThumbnailZoomAnchor = static_cast<ZoomAnchor>(Number);
        }

        return;
    }

    if (Key == "ActiveClientHighlightColor")
    {
        Color Parsed;
        if (Value.is_string() == true && Color::TryParse(Value.get<std::string>(), &Parsed) == true)
        {
            Configuration.ActiveClientHighlightColor = Parsed;
        }

        return;
    }

    if (Key == "CharacterNames")
    {
        LoadIdentifierMap(Value, Configuration.CharacterNames);
        return;
    }

    if (Key == "AccountNicknames")
    {
        LoadIdentifierMap(Value, Configuration.AccountNicknames);
        return;
    }

    if (Key == "PerClientLayout")
    {
        LoadPerClientLayout(Value);
        return;
    }

    if (Key == "FlatLayout")
    {
        LoadFlatLayout(Value);
        return;
    }

    if (Key == "ClientLayout")
    {
        LoadClientLayouts(Value);
        return;
    }

    if (Key == "ClientHotkey")
    {
        LoadClientHotkeys(Value);
        return;
    }

    if (Key == "DisableThumbnail")
    {
        LoadDisabledThumbnails(Value);
        return;
    }

    if (Key == "UniverseSavedChannels")
    {
        LoadSavedChannels(Value);
        return;
    }

    if (Key == "KnownCharacters")
    {
        LoadStringList(Value, Configuration.KnownCharacters);
        return;
    }

    if (Key == "HiddenCharacters")
    {
        LoadStringList(Value, Configuration.HiddenCharacters);
        return;
    }

    if (Key == "OrganizerSlots")
    {
        LoadStringList(Value, Configuration.OrganizerSlots);
        return;
    }

    if (Key == "PriorityClients")
    {
        LoadPriorityClients(Value);
        return;
    }

    if (Key == "CycleGroups")
    {
        LoadCycleGroups(Value);
        return;
    }

    if (Key == "LayoutPresets")
    {
        LoadLayoutPresets(Value);
    }
}

void ConfigurationStorage::LoadCycleGroups(const Json& Value)
{
    Configuration.CycleGroups.clear();
    if (Value.is_array() == false)
    {
        return;
    }

    for (const Json& Entry : Value)
    {
        if (Entry.is_object() == false)
        {
            continue;
        }

        CycleGroup Group;
        const Json Name = Entry.value("Name", Json());
        Group.Name = Name.is_string() == true ? Name.get<std::string>() : std::string();

        const Json Members = Entry.value("Members", Json());
        if (Members.is_array() == true)
        {
            for (const Json& Member : Members)
            {
                if (Member.is_string() == true)
                {
                    Group.Members.push_back(TextUtil::FromUtf8(Member.get<std::string>()));
                }
            }
        }

        const Json Next = Entry.value("Next", Json());
        const Json Previous = Entry.value("Previous", Json());
        Group.Next = Next.is_string() == true ? Hotkey::Parse(Next.get<std::string>()) : Hotkey();
        Group.Previous = Previous.is_string() == true ? Hotkey::Parse(Previous.get<std::string>()) : Hotkey();
        Configuration.CycleGroups.push_back(std::move(Group));
    }
}

void ConfigurationStorage::LoadLayoutPresets(const Json& Value)
{
    if (Value.is_array() == false)
    {
        return;
    }

    for (const Json& Entry : Value)
    {
        if (Entry.is_object() == false)
        {
            continue;
        }

        const Json Name = Entry.value("Name", Json());
        LayoutPreset Preset;
        if (Name.is_string() == false || TryReadInt(Entry.value("ClientCount", Json()), &Preset.ClientCount) == false
            || TryReadInt(Entry.value("Columns", Json()), &Preset.Arrangement.Shape.Columns) == false
            || TryReadInt(Entry.value("Rows", Json()), &Preset.Arrangement.Shape.Rows) == false
            || TryReadInt(Entry.value("Gap", Json()), &Preset.Arrangement.Gap) == false
            || TryReadPoint(Entry.value("Origin", Json()), &Preset.Arrangement.Origin) == false)
        {
            continue;
        }

        Preset.Name = Name.get<std::string>();

        const Json PartialFirst = Entry.value("PartialRowFirst", Json());
        Preset.Arrangement.Shape.PartialRowFirst = PartialFirst.is_boolean() == true && PartialFirst.get<bool>();
        LoadStringList(Entry.value("Slots", Json()), Preset.Arrangement.SlotCharacters);
        Configuration.LayoutPresets.push_back(Preset);
    }
}

ConfigurationStorage::Json ConfigurationStorage::BuildIdentifierMap(const std::map<long long, std::string>& Source)
{
    Json Result = Json::object();
    for (const std::pair<const long long, std::string>& Entry : Source)
    {
        Result[std::to_string(Entry.first)] = Entry.second;
    }

    return Result;
}

void ConfigurationStorage::LoadIdentifierMap(const Json& Value, std::map<long long, std::string>& Target)
{
    if (Value.is_object() == false)
    {
        return;
    }

    for (Json::const_iterator Entry = Value.begin(); Entry != Value.end(); ++Entry)
    {
        long long Identifier = 0;
        const std::string& Key = Entry.key();
        const std::from_chars_result ParseResult = std::from_chars(Key.data(), Key.data() + Key.size(), Identifier);
        if (ParseResult.ec != std::errc() || ParseResult.ptr != Key.data() + Key.size() || Entry.value().is_string() == false)
        {
            continue;
        }

        Target[Identifier] = Entry.value().get<std::string>();
    }
}

void ConfigurationStorage::LoadPerClientLayout(const Json& Value)
{
    if (Value.is_object() == false)
    {
        return;
    }

    for (Json::const_iterator Client = Value.begin(); Client != Value.end(); ++Client)
    {
        if (Client.value().is_object() == false)
        {
            continue;
        }

        const std::wstring ClientName = TextUtil::FromUtf8(Client.key());
        for (Json::const_iterator Thumbnail = Client.value().begin(); Thumbnail != Client.value().end(); ++Thumbnail)
        {
            Point Location;
            if (TryReadPoint(Thumbnail.value(), &Location) == false)
            {
                continue;
            }

            Configuration.PerClientLayout[ClientName][TextUtil::FromUtf8(Thumbnail.key())] = Location;
        }
    }
}

void ConfigurationStorage::LoadFlatLayout(const Json& Value)
{
    if (Value.is_object() == false)
    {
        return;
    }

    for (Json::const_iterator Thumbnail = Value.begin(); Thumbnail != Value.end(); ++Thumbnail)
    {
        Point Location;
        if (TryReadPoint(Thumbnail.value(), &Location) == false)
        {
            continue;
        }

        Configuration.FlatLayout[TextUtil::FromUtf8(Thumbnail.key())] = Location;
    }
}

void ConfigurationStorage::LoadClientLayouts(const Json& Value)
{
    if (Value.is_object() == false)
    {
        return;
    }

    for (Json::const_iterator Client = Value.begin(); Client != Value.end(); ++Client)
    {
        if (Client.value().is_object() == false)
        {
            continue;
        }

        ClientLayout Layout;
        TryReadInt(Client.value().value("X", Json()), &Layout.X);
        TryReadInt(Client.value().value("Y", Json()), &Layout.Y);
        TryReadInt(Client.value().value("Width", Json()), &Layout.Width);
        TryReadInt(Client.value().value("Height", Json()), &Layout.Height);

        const Json Maximized = Client.value().value("IsMaximized", Json());
        if (Maximized.is_boolean() == true)
        {
            Layout.IsMaximized = Maximized.get<bool>();
        }

        Configuration.ClientLayouts[TextUtil::FromUtf8(Client.key())] = Layout;
    }
}

void ConfigurationStorage::LoadClientHotkeys(const Json& Value)
{
    if (Value.is_object() == false)
    {
        return;
    }

    for (Json::const_iterator Client = Value.begin(); Client != Value.end(); ++Client)
    {
        if (Client.value().is_string() == false)
        {
            continue;
        }

        Configuration.ClientHotkeys[TextUtil::FromUtf8(Client.key())] = Client.value().get<std::string>();
    }
}

void ConfigurationStorage::LoadDisabledThumbnails(const Json& Value)
{
    if (Value.is_object() == false)
    {
        return;
    }

    for (Json::const_iterator Client = Value.begin(); Client != Value.end(); ++Client)
    {
        if (Client.value().is_boolean() == false)
        {
            continue;
        }

        Configuration.DisabledThumbnails[TextUtil::FromUtf8(Client.key())] = Client.value().get<bool>();
    }
}

void ConfigurationStorage::LoadSavedChannels(const Json& Value)
{
    Configuration.UniverseSavedChannels.clear();
    if (Value.is_array() == false)
    {
        return;
    }

    for (const Json& Channel : Value)
    {
        if (Channel.is_string() == true)
        {
            Configuration.UniverseSavedChannels.push_back(Channel.get<std::string>());
        }
    }
}

ConfigurationStorage::Json ConfigurationStorage::BuildStringList(const std::vector<std::string>& Source)
{
    Json List = Json::array();
    for (const std::string& Entry : Source)
    {
        List.push_back(Entry);
    }

    return List;
}

void ConfigurationStorage::LoadStringList(const Json& Value, std::vector<std::string>& Target)
{
    Target.clear();
    if (Value.is_array() == false)
    {
        return;
    }

    for (const Json& Entry : Value)
    {
        if (Entry.is_string() == true)
        {
            Target.push_back(Entry.get<std::string>());
        }
    }
}

void ConfigurationStorage::LoadPriorityClients(const Json& Value)
{
    if (Value.is_array() == false)
    {
        return;
    }

    for (const Json& Client : Value)
    {
        if (Client.is_string() == false)
        {
            continue;
        }

        Configuration.PriorityClients.push_back(TextUtil::FromUtf8(Client.get<std::string>()));
    }
}
