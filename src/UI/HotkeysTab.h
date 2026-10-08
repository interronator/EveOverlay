#pragma once

#include <map>
#include <string>
#include <vector>

#include "Config/CycleGroup.h"
#include "UI/HotkeyField.h"
#include "UI/ITabPage.h"
#include "UI/Widgets.h"

// Hotkeys for single clients, for stepping through groups of clients, and for showing or hiding every preview
class HotkeysTab : public ITabPage
{
public:
    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    void AddClient(const std::wstring& ClientTitle);
    void RemoveClient(const std::wstring& ClientTitle);

private:
    static constexpr float FIELD_WIDTH = 190.0f;
    static constexpr size_t NAME_BUFFER_SIZE = 64;
    static constexpr size_t MAXIMUM_GROUPS = 8;

    struct GroupEditor
    {
        CycleGroup Group;
        char NameBuffer[NAME_BUFFER_SIZE] = {};
    };

    // "Mierk" from "EVE - Mierk"
    static std::string GetDisplayName(const std::wstring& ClientTitle);

    // Open clients plus every client mentioned by a saved hotkey or group, in name order
    std::vector<std::wstring> GetKnownClients() const;

    bool DrawClientHotkeys();
    bool DrawGlobalHotkeys();
    bool DrawGroup(const size_t Index, bool& Removed);
    bool DrawGroups();

    const std::string Title = "Hotkeys";
    const std::string Description = "Jump to a client, step through groups of clients, show or hide previews.";

    std::vector<std::wstring> OpenClients;
    std::map<std::wstring, Hotkey> ClientHotkeys;
    std::vector<GroupEditor> Groups;
    Hotkey TogglePreviews;
    Hotkey MinimizeAll;
};
