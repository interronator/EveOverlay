#include "Services/CountdownTimers.h"

#include <cstdio>

int CountdownTimers::Start(const std::string& Label, const ULONGLONG DurationMs, const ULONGLONG Now)
{
    CountdownTimer Timer;
    Timer.Id = NextId++;
    Timer.Label = Label;
    Timer.EndTick = Now + DurationMs;
    Timers.push_back(std::move(Timer));
    return Timers.back().Id;
}

void CountdownTimers::Cancel(const int Id)
{
    for (std::vector<CountdownTimer>::iterator Current = Timers.begin(); Current != Timers.end(); ++Current)
    {
        if (Current->Id != Id)
        {
            continue;
        }

        Timers.erase(Current);
        return;
    }
}

void CountdownTimers::Clear()
{
    Timers.clear();
}

std::vector<std::string> CountdownTimers::Update(const ULONGLONG Now)
{
    std::vector<std::string> RanOut;
    for (std::vector<CountdownTimer>::iterator Current = Timers.begin(); Current != Timers.end();)
    {
        if (Current->Finished == false && Now >= Current->EndTick)
        {
            Current->Finished = true;
            RanOut.push_back(Current->Label);
        }

        if (Current->Finished == true && Now - Current->EndTick >= FINISHED_VISIBLE_MS)
        {
            Current = Timers.erase(Current);
            continue;
        }

        ++Current;
    }

    return RanOut;
}

const std::vector<CountdownTimer>& CountdownTimers::GetTimers() const
{
    return Timers;
}

ULONGLONG CountdownTimers::GetRemaining(const CountdownTimer& Timer, const ULONGLONG Now) const
{
    return Now >= Timer.EndTick ? 0 : Timer.EndTick - Now;
}

void CountdownTimers::MarkScan(const ULONGLONG Now)
{
    ScanMarked = true;
    ScanTick = Now;
}

void CountdownTimers::ClearScan()
{
    ScanMarked = false;
}

bool CountdownTimers::HasScan() const
{
    return ScanMarked;
}

ULONGLONG CountdownTimers::GetScanAge(const ULONGLONG Now) const
{
    return ScanMarked == true && Now >= ScanTick ? Now - ScanTick : 0;
}

std::string CountdownTimers::FormatDuration(const ULONGLONG Milliseconds)
{
    const ULONGLONG TotalSeconds = (Milliseconds + 999) / 1000;
    const ULONGLONG Hours = TotalSeconds / 3600;
    const ULONGLONG Minutes = (TotalSeconds / 60) % 60;
    const ULONGLONG Seconds = TotalSeconds % 60;

    char Text[32] = {};
    if (Hours > 0)
    {
        std::snprintf(Text, sizeof(Text), "%llu:%02llu:%02llu", Hours, Minutes, Seconds);
    }
    else
    {
        std::snprintf(Text, sizeof(Text), "%llu:%02llu", Minutes, Seconds);
    }

    return Text;
}
