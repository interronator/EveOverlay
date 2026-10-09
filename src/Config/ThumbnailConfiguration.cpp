#include "Config/ThumbnailConfiguration.h"

#include <algorithm>
#include <cmath>
#include <set>

#include "Config/TextUtil.h"

std::string ThumbnailConfiguration::GetCharacterName(const std::wstring& ClientTitle)
{
    constexpr const wchar_t* PREFIX = L"EVE - ";
    const size_t PrefixLength = std::char_traits<wchar_t>::length(PREFIX);
    if (ClientTitle.size() <= PrefixLength || ClientTitle.compare(0, PrefixLength, PREFIX) != 0)
    {
        return std::string();
    }

    return TextUtil::Trim(TextUtil::ToUtf8(ClientTitle.substr(PrefixLength)));
}

bool ThumbnailConfiguration::RememberCharacter(const std::wstring& ClientTitle)
{
    const std::string Name = GetCharacterName(ClientTitle);
    if (Name.empty() == true)
    {
        return false;
    }

    RemoveName(HiddenCharacters, Name);

    for (const std::string& Known : KnownCharacters)
    {
        if (TextUtil::EqualsIgnoreCase(Known, Name) == true)
        {
            return false;
        }
    }

    KnownCharacters.push_back(Name);
    return true;
}

void ThumbnailConfiguration::RemoveName(std::vector<std::string>& Names, const std::string& Name)
{
    Names.erase(std::remove_if(Names.begin(), Names.end(), [&Name](const std::string& Existing)
    {
        return TextUtil::EqualsIgnoreCase(Existing, Name) == true;
    }), Names.end());
}

bool ThumbnailConfiguration::IsCharacterHidden(const std::string& Name) const
{
    for (const std::string& Hidden : HiddenCharacters)
    {
        if (TextUtil::EqualsIgnoreCase(Hidden, Name) == true)
        {
            return true;
        }
    }

    return false;
}

bool ThumbnailConfiguration::ForgetCharacter(const std::string& Name)
{
    const std::string Trimmed = TextUtil::Trim(Name);
    if (Trimmed.empty() == true)
    {
        return false;
    }

    RemoveName(KnownCharacters, Trimmed);

    if (IsCharacterHidden(Trimmed) == false)
    {
        HiddenCharacters.push_back(Trimmed);
    }

    bool SelectionCleared = false;
    if (TextUtil::EqualsIgnoreCase(MainCharacter, Trimmed) == true)
    {
        MainCharacter.clear();
        SelectionCleared = true;
    }

    if (TextUtil::EqualsIgnoreCase(UniverseLocationCharacter, Trimmed) == true)
    {
        UniverseLocationCharacter.clear();
        SelectionCleared = true;
    }

    return SelectionCleared;
}

std::vector<std::string> ThumbnailConfiguration::GetSelectableCharacters() const
{
    std::vector<std::string> Candidates;
    for (const std::string& Known : KnownCharacters)
    {
        Candidates.push_back(Known);
    }

    for (const std::pair<const long long, std::string>& Entry : CharacterNames)
    {
        Candidates.push_back(Entry.second);
    }

    for (const std::pair<const std::wstring, std::string>& Entry : ClientHotkeys)
    {
        Candidates.push_back(GetCharacterName(Entry.first));
    }

    Candidates.erase(std::remove_if(Candidates.begin(), Candidates.end(), [this](const std::string& Candidate)
    {
        return IsCharacterHidden(Candidate) == true;
    }), Candidates.end());

    Candidates.push_back(MainCharacter);
    Candidates.push_back(UniverseLocationCharacter);

    std::vector<std::string> Characters;
    std::set<std::string> Seen;
    for (const std::string& Candidate : Candidates)
    {
        const std::string Name = TextUtil::Trim(Candidate);
        if (Name.empty() == true || Seen.insert(TextUtil::ToLower(Name)).second == false)
        {
            continue;
        }

        Characters.push_back(Name);
    }

    std::sort(Characters.begin(), Characters.end(), [](const std::string& Left, const std::string& Right)
    {
        return TextUtil::CompareIgnoreCase(Left, Right) < 0;
    });

    return Characters;
}

bool ThumbnailConfiguration::IsClientLayoutTrackingEnabled() const
{
    return ClientLayoutTrackingEnabled;
}

void ThumbnailConfiguration::SetClientLayoutTrackingEnabled(const bool Enabled)
{
    if (Enabled == false)
    {
        ClientLayouts.clear();
    }

    ClientLayoutTrackingEnabled = Enabled;
}

bool ThumbnailConfiguration::IsPerClientThumbnailLayoutsEnabled() const
{
    return PerClientThumbnailLayoutsEnabled;
}

void ThumbnailConfiguration::SetPerClientThumbnailLayoutsEnabled(const bool Enabled)
{
    if (Enabled == false)
    {
        PerClientLayout.clear();
    }

    PerClientThumbnailLayoutsEnabled = Enabled;
}

Point ThumbnailConfiguration::GetDefaultThumbnailLocation() const
{
    return Point{5, 5};
}

Point ThumbnailConfiguration::GetThumbnailLocation(const std::wstring& CurrentClient, const std::wstring& ActiveClient, const Point DefaultLocation) const
{
    if (PerClientThumbnailLayoutsEnabled == true && ActiveClient.empty() == false)
    {
        const std::map<std::wstring, std::map<std::wstring, Point>>::const_iterator Layout = PerClientLayout.find(ActiveClient);
        if (Layout != PerClientLayout.end())
        {
            const std::map<std::wstring, Point>::const_iterator Location = Layout->second.find(CurrentClient);
            if (Location != Layout->second.end())
            {
                return Location->second;
            }
        }
    }

    const std::map<std::wstring, Point>::const_iterator Location = FlatLayout.find(CurrentClient);
    if (Location == FlatLayout.end())
    {
        return DefaultLocation;
    }

    return Location->second;
}

void ThumbnailConfiguration::SetThumbnailLocation(const std::wstring& CurrentClient, const std::wstring& ActiveClient, const Point Location)
{
    if (PerClientThumbnailLayoutsEnabled == false)
    {
        FlatLayout[CurrentClient] = Location;
        return;
    }

    if (ActiveClient.empty() == true)
    {
        return;
    }

    PerClientLayout[ActiveClient][CurrentClient] = Location;
}

std::optional<ClientLayout> ThumbnailConfiguration::GetClientLayout(const std::wstring& CurrentClient) const
{
    const std::map<std::wstring, ClientLayout>::const_iterator Layout = ClientLayouts.find(CurrentClient);
    if (Layout == ClientLayouts.end())
    {
        return std::nullopt;
    }

    return Layout->second;
}

void ThumbnailConfiguration::SetClientLayout(const std::wstring& CurrentClient, const ClientLayout& Layout)
{
    ClientLayouts[CurrentClient] = Layout;
}

Hotkey ThumbnailConfiguration::GetClientHotkey(const std::wstring& CurrentClient) const
{
    const std::map<std::wstring, std::string>::const_iterator Entry = ClientHotkeys.find(CurrentClient);
    if (Entry == ClientHotkeys.end())
    {
        return Hotkey();
    }

    return Hotkey::Parse(Entry->second);
}

void ThumbnailConfiguration::SetClientHotkey(const std::wstring& CurrentClient, const Hotkey& Value)
{
    ClientHotkeys[CurrentClient] = Value.ToString();
}

std::vector<std::wstring> ThumbnailConfiguration::GetClientHotkeyTitles() const
{
    std::vector<std::wstring> Titles;
    for (const std::pair<const std::wstring, std::string>& Entry : ClientHotkeys)
    {
        Titles.push_back(Entry.first);
    }

    return Titles;
}

std::string ThumbnailConfiguration::GetHotkeySignature() const
{
    std::string Signature = TogglePreviewsHotkey + "|" + MinimizeAllHotkey;
    for (const std::pair<const std::wstring, std::string>& Entry : ClientHotkeys)
    {
        Signature += "|" + std::to_string(Entry.first.size()) + ":" + Entry.second;
        for (const wchar_t Character : Entry.first)
        {
            Signature += std::to_string(static_cast<unsigned>(Character)) + ",";
        }
    }

    for (const CycleGroup& Group : CycleGroups)
    {
        Signature += "|G" + Group.Next.ToString() + "/" + Group.Previous.ToString();
        for (const std::wstring& Member : Group.Members)
        {
            Signature += "," + std::to_string(Member.size());
            for (const wchar_t Character : Member)
            {
                Signature += std::to_string(static_cast<unsigned>(Character)) + ".";
            }
        }
    }

    return Signature;
}

bool ThumbnailConfiguration::IsPriorityClient(const std::wstring& CurrentClient) const
{
    for (const std::wstring& Client : PriorityClients)
    {
        if (Client == CurrentClient)
        {
            return true;
        }
    }

    return false;
}

bool ThumbnailConfiguration::IsThumbnailDisabled(const std::wstring& CurrentClient) const
{
    const std::map<std::wstring, bool>::const_iterator Entry = DisabledThumbnails.find(CurrentClient);
    if (Entry == DisabledThumbnails.end())
    {
        return false;
    }

    return Entry->second;
}

void ThumbnailConfiguration::ToggleThumbnail(const std::wstring& CurrentClient, const bool IsDisabled)
{
    DisabledThumbnails[CurrentClient] = IsDisabled;
}

void ThumbnailConfiguration::ApplyRestrictions()
{
    ThumbnailRefreshPeriod = Restrict(ThumbnailRefreshPeriod, 300, 1000);
    MainWindowWidth = Restrict(MainWindowWidth, 0, 10000);
    MainWindowHeight = Restrict(MainWindowHeight, 0, 10000);
    ThumbnailMinimumSize = Size{std::max(ThumbnailMinimumSize.Width, 1), std::max(ThumbnailMinimumSize.Height, 1)};
    ThumbnailMaximumSize = Size{
        NormalizeMaximum(ThumbnailMaximumSize.Width, ThumbnailMinimumSize.Width),
        NormalizeMaximum(ThumbnailMaximumSize.Height, ThumbnailMinimumSize.Height)};
    ThumbnailSize = Size{
        Restrict(ThumbnailSize.Width, ThumbnailMinimumSize.Width, ThumbnailMaximumSize.Width),
        Restrict(ThumbnailSize.Height, ThumbnailMinimumSize.Height, ThumbnailMaximumSize.Height)};
    // Rounded rather than truncated: 0.29 * 100.0 is 28.999..., which would lose a percent on every reload
    ThumbnailOpacity = Restrict(static_cast<int>(std::lround(ThumbnailOpacity * 100.0)), 20, 100) / 100.0;
    ThumbnailZoomFactor = Restrict(ThumbnailZoomFactor, 2, 10);
    ActiveClientHighlightThickness = Restrict(ActiveClientHighlightThickness, 1, 6);
    UniverseJumps = Restrict(UniverseJumps, 0, UNIVERSE_MAX_JUMPS);
    UniverseMapSize = Restrict(UniverseMapSize, 300, 1600);
    UniverseMapRotation = Restrict(UniverseMapRotation, 0, 359);
    UniverseAlertVolume = Restrict(UniverseAlertVolume, 0, 100);

    for (LayoutPreset& Preset : LayoutPresets)
    {
        Preset.ClientCount = Restrict(Preset.ClientCount, 1, 40);
        Preset.Arrangement.Shape.Columns = Restrict(Preset.Arrangement.Shape.Columns, 1, 40);
        Preset.Arrangement.Shape.Rows = Restrict(Preset.Arrangement.Shape.Rows, 1, 40);
        Preset.Arrangement.Gap = Restrict(Preset.Arrangement.Gap, 0, 200);
    }
    UniverseAlertTimeout = Restrict(UniverseAlertTimeout, 10, 1800);
    AttackAlertVolume = Restrict(AttackAlertVolume, 0, 100);
    AttackAlertSeconds = Restrict(AttackAlertSeconds, 3, 120);
    QuickTimerSeconds = Restrict(QuickTimerSeconds, 5, 86400);
    DScanShowSeconds = Restrict(DScanShowSeconds, 5, 120);
    TimerVolume = Restrict(TimerVolume, 0, 100);
}

int ThumbnailConfiguration::Restrict(const int Value, const int Minimum, const int Maximum)
{
    if (Value <= Minimum)
    {
        return Minimum;
    }

    if (Value >= Maximum)
    {
        return Maximum;
    }

    return Value;
}

int ThumbnailConfiguration::NormalizeMaximum(const int Maximum, const int Minimum)
{
    if (Maximum <= 0)
    {
        return std::max(UNLIMITED_THUMBNAIL_DIMENSION, Minimum);
    }

    return std::max(Maximum, Minimum);
}
