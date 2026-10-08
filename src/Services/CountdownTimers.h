#pragma once

#include <string>
#include <vector>

#include <Windows.h>

struct CountdownTimer
{
    int Id = 0;
    std::string Label;
    ULONGLONG EndTick = 0;
    bool Finished = false;
};

// Manual countdowns plus a "scan age" stopwatch for the last directional scan. The caller passes the current tick count to
// every call, so the logic does not depend on the clock.
class CountdownTimers
{
public:
    // A timer that has run out stays on the list this long so it can be noticed
    static constexpr ULONGLONG FINISHED_VISIBLE_MS = 15000;

    int Start(const std::string& Label, const ULONGLONG DurationMs, const ULONGLONG Now);
    void Cancel(const int Id);
    void Clear();

    // Marks the timers that ran out and drops the ones that ran out long ago. Returns the labels of those that ran out just now.
    std::vector<std::string> Update(const ULONGLONG Now);

    const std::vector<CountdownTimer>& GetTimers() const;
    ULONGLONG GetRemaining(const CountdownTimer& Timer, const ULONGLONG Now) const;

    void MarkScan(const ULONGLONG Now);
    void ClearScan();
    bool HasScan() const;
    ULONGLONG GetScanAge(const ULONGLONG Now) const;

    // "4:05", or "1:02:03" from an hour up; partial seconds round up so a timer never shows 0:00 while it is still running
    static std::string FormatDuration(const ULONGLONG Milliseconds);

private:
    std::vector<CountdownTimer> Timers;
    int NextId = 1;
    bool ScanMarked = false;
    ULONGLONG ScanTick = 0;
};
