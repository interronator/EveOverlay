#pragma once

#include <set>
#include <string>
#include <vector>

#include "Application/Signal.h"
#include "Config/ConfigurationStorage.h"
#include "Config/ThumbnailConfiguration.h"
#include "UI/ClientsTab.h"
#include "UI/HotkeysTab.h"
#include "UI/ITabPage.h"
#include "UI/OrganizerTab.h"
#include "UI/ThumbnailTab.h"

// Moves settings between the tabs and the configuration, and persists every change
class SettingsPresenter
{
public:
    Signal<> FrameSettingsChanged;
    Signal<> ThumbnailSizeChanged;
    Signal<bool> ClientsOpenChanged;
    Signal<> HotkeysChanged;
    Signal<const ThumbnailArrangement&> ArrangeRequested;

    SettingsPresenter(ThumbnailConfiguration& ConfigurationReference, ConfigurationStorage& StorageReference,
        const std::vector<ITabPage*>& TabPages, ThumbnailTab& ThumbnailTabReference, ClientsTab& ClientsTabReference, OrganizerTab& OrganizerTabReference, HotkeysTab& HotkeysTabReference);

    void LoadSettings();
    void AddThumbnails(const std::vector<std::wstring>& Titles);
    void RemoveThumbnails(const std::vector<std::wstring>& Titles);

    // A resize reaches here once per size message while dragging, so the config is only updated; it is written by the next save
    void UpdateThumbnailSize(const Size NewSize);

private:
    void SaveSettings();
    void NotifyClientsOpen();
    void OnThumbnailSizeChanged();
    void OnThumbnailStateChanged(const std::wstring& Title, const bool IsDisabled);

    ThumbnailConfiguration& Configuration;
    ConfigurationStorage& Storage;
    std::vector<ITabPage*> Pages;
    ThumbnailTab& ThumbnailPage;
    ClientsTab& ClientsPage;
    OrganizerTab& OrganizerPage;
    HotkeysTab& HotkeysPage;
    std::set<std::wstring> OpenClients;
    bool ClientsOpen = false;
};
