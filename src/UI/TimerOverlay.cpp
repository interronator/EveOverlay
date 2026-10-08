#include "UI/TimerOverlay.h"

#include <algorithm>
#include <utility>

#include "Config/TextUtil.h"

TimerOverlay::TimerOverlay(std::filesystem::path SettingsFilePath)
    : Panel(std::move(SettingsFilePath), L"TimerOverlay", PANEL_WIDTH)
{
}

void TimerOverlay::Configure(const TimerOverlayOptions& Options)
{
    Settings = Options;
    SoundPath = TextUtil::FromUtf8(Options.SoundPath);

    if (TickerStarted == false)
    {
        TickerStarted = true;
        Ticker.Start(TICK_INTERVAL_MS, [this]()
        {
            Tick();
        });
    }

    Refresh();
}

void TimerOverlay::SetClientsOpen(const bool IsOpen)
{
    ClientsOpen = IsOpen;
    Refresh();
}

int TimerOverlay::StartTimer(const std::string& Label, const int Seconds)
{
    const int Id = Timers.Start(Label, static_cast<ULONGLONG>(std::max(1, Seconds)) * 1000ULL, ::GetTickCount64());
    Refresh();
    return Id;
}

void TimerOverlay::CancelTimer(const int Id)
{
    Timers.Cancel(Id);
    Refresh();
}

void TimerOverlay::MarkScan()
{
    Timers.MarkScan(::GetTickCount64());
    Refresh();
}

void TimerOverlay::ClearScan()
{
    Timers.ClearScan();
    Refresh();
}

void TimerOverlay::PlaySound()
{
    Sound.Play(SoundPath, Settings.SoundVolume);
}

const CountdownTimers& TimerOverlay::GetTimers() const
{
    return Timers;
}

void TimerOverlay::Tick()
{
    const std::vector<std::string> RanOut = Timers.Update(::GetTickCount64());
    if (RanOut.empty() == false && Settings.SoundEnabled == true)
    {
        PlaySound();
    }

    Refresh();
}

void TimerOverlay::Refresh()
{
    Panel.Update(BuildRows(::GetTickCount64()), ClientsOpen == true && Settings.ShowWindow == true);
}

std::vector<PanelRow> TimerOverlay::BuildRows(const ULONGLONG Now) const
{
    std::vector<PanelRow> Rows;
    if (Settings.ShowScanAge == true && Timers.HasScan() == true)
    {
        const ULONGLONG Age = Timers.GetScanAge(Now);
        PanelRow Scan;
        Scan.Label = "D-Scan";
        Scan.Value = CountdownTimers::FormatDuration(Age - Age % 1000);
        if (Age < SCAN_FRESH_MS)
        {
            Scan.Red = 120;
            Scan.Green = 230;
            Scan.Blue = 120;
        }
        else if (Age < SCAN_STALE_MS)
        {
            Scan.Red = 255;
            Scan.Green = 210;
            Scan.Blue = 90;
        }
        else
        {
            Scan.Red = 255;
            Scan.Green = 110;
            Scan.Blue = 90;
        }

        Rows.push_back(std::move(Scan));
    }

    const bool BlinkBright = (Now / BLINK_PERIOD_MS) % 2 == 0;
    for (const CountdownTimer& Timer : Timers.GetTimers())
    {
        PanelRow Entry;
        Entry.Label = Timer.Label;
        if (Timer.Finished == true)
        {
            Entry.Value = "DONE";
            Entry.Red = BlinkBright == true ? 255 : 150;
            Entry.Green = BlinkBright == true ? 70 : 40;
            Entry.Blue = BlinkBright == true ? 70 : 40;
        }
        else
        {
            const ULONGLONG Remaining = Timers.GetRemaining(Timer, Now);
            Entry.Value = CountdownTimers::FormatDuration(Remaining);
            if (Remaining <= 10000)
            {
                Entry.Red = 255;
                Entry.Green = 210;
                Entry.Blue = 90;
            }
        }

        Rows.push_back(std::move(Entry));
    }

    return Rows;
}
