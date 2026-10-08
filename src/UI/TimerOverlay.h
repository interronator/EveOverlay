#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "Services/AlertSound.h"
#include "Services/CountdownTimers.h"
#include "Services/TimerWindow.h"
#include "UI/InfoPanelWindow.h"

struct TimerOverlayOptions
{
    bool ShowWindow = true;
    bool ShowScanAge = true;
    bool SoundEnabled = true;
    int SoundVolume = 70;
    std::string SoundPath;
};

// Runs the countdowns and the scan age, and shows them in a small panel over the game while an EVE client is open
class TimerOverlay
{
public:
    // Scan age colour thresholds: fresh, getting old, stale
    static constexpr ULONGLONG SCAN_FRESH_MS = 30000;
    static constexpr ULONGLONG SCAN_STALE_MS = 120000;

    explicit TimerOverlay(std::filesystem::path SettingsFilePath);
    TimerOverlay(const TimerOverlay&) = delete;
    TimerOverlay& operator=(const TimerOverlay&) = delete;

    void Configure(const TimerOverlayOptions& Options);

    void SetClientsOpen(const bool IsOpen);

    int StartTimer(const std::string& Label, const int Seconds);
    void CancelTimer(const int Id);
    void MarkScan();
    void ClearScan();
    void PlaySound();

    const CountdownTimers& GetTimers() const;

private:
    static constexpr UINT TICK_INTERVAL_MS = 250;
    static constexpr int BLINK_PERIOD_MS = 500;
    static constexpr float PANEL_WIDTH = 230.0f;

    std::vector<PanelRow> BuildRows(const ULONGLONG Now) const;
    void Tick();
    void Refresh();

    InfoPanelWindow Panel;
    TimerWindow Ticker;
    CountdownTimers Timers;
    TimerOverlayOptions Settings;
    AlertSound Sound;
    std::wstring SoundPath;
    bool ClientsOpen = false;
    bool TickerStarted = false;
};
