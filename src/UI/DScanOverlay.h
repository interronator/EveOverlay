#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Application/Signal.h"
#include "Services/TimerWindow.h"
#include "UI/InfoPanelWindow.h"
#include "Universe/DScanAnalyzer.h"
#include "Universe/ShipCatalog.h"

struct DScanOverlayOptions
{
    bool AutoRead = false;
    int ShowSeconds = 30;
    bool CopyShipList = true;
};

// Reads a directional scan copied from the game and summarises it in a small panel over the game. Nothing is read from the
// clipboard unless automatic reading is switched on, and only text shaped like a scan is looked at.
class DScanOverlay
{
public:
    // Emitted whenever a scan was read, so the scan age can start from zero
    Signal<> ScanRead;

    DScanOverlay(std::filesystem::path DataPath, std::filesystem::path SettingsFilePath);
    DScanOverlay(const DScanOverlay&) = delete;
    DScanOverlay& operator=(const DScanOverlay&) = delete;

    void Configure(const DScanOverlayOptions& Options);
    void SetClientsOpen(const bool IsOpen);

    // Reads whatever text is on the clipboard now; false when it is not a scan
    bool ReadClipboardNow();

    // Puts the ship list of the last scan on the clipboard; false when there is nothing to copy or the clipboard is busy
    bool CopyShipListToClipboard();

    // The result of the last scan that was read; empty before the first one
    const DScanResult& GetLastResult() const;
    bool HasResult() const;
    bool IsCatalogLoaded() const;

private:
    static constexpr UINT TICK_INTERVAL_MS = 500;
    static constexpr float PANEL_WIDTH = 290.0f;
    static constexpr size_t MAXIMUM_FLAG_ROWS = 5;
    static constexpr size_t MAXIMUM_SHIP_ROWS = 8;

    // Opened tells whether the clipboard could be opened at all, which is true even when it holds no text
    static bool TrySetClipboardText(const std::string& Text);
    static bool TryGetClipboardText(std::string* const Text, bool* const Opened = nullptr);

    bool Accept(const std::string& Text);
    std::vector<PanelRow> BuildRows() const;
    void Tick();
    void Refresh();

    InfoPanelWindow Panel;
    TimerWindow Ticker;
    std::filesystem::path CatalogPath;
    DScanOverlayOptions Settings;
    DScanResult LastResult;
    bool ResultReady = false;
    ULONGLONG HideAtTick = 0;
    DWORD LastClipboardSequence = 0;
    bool ClientsOpen = false;
    bool TickerStarted = false;
};
