#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

#include "Application/CompositionRoot.h"
#include "Application/Logger.h"
#include "Application/SingleInstanceGuard.h"
#include "Services/CountdownTimers.h"
#include "Services/IThumbnailManager.h"

class Checker
{
public:
    void Expect(const bool Condition, const char* const Description)
    {
        if (Condition == true)
        {
            return;
        }

        FailureCount++;
        std::printf("FAIL: %s\n", Description);
    }

    int FailureCount = 0;
};

class FakeThumbnailManager : public IThumbnailManager
{
public:
    void Start() override
    {
    }

    void Stop() override
    {
    }

    void UpdateHotkeys() override
    {
    }

    void UpdateThumbnailsSize() override
    {
    }

    void UpdateThumbnailFrames() override
    {
    }

    void ArrangeThumbnails(const ThumbnailArrangement&) override
    {
    }
};

int wmain()
{
    Checker Check;

    FakeThumbnailManager Manager;
    int Calls = 0;
    std::vector<std::wstring> LastAdded;
    const size_t FirstId = Manager.ThumbnailListUpdated.Connect([&Calls, &LastAdded](const std::vector<std::wstring>& Added, const std::vector<std::wstring>&)
    {
        Calls++;
        LastAdded = Added;
    });
    Manager.ThumbnailActiveSizeUpdated.Connect([&Calls](const Size NewSize)
    {
        Calls += NewSize.Width;
    });

    Manager.ThumbnailListUpdated.Emit({L"Alpha"}, {});
    Check.Expect(Calls == 1 && LastAdded.size() == 1, "list signal delivered");
    Manager.ThumbnailActiveSizeUpdated.Emit(Size{10, 5});
    Check.Expect(Calls == 11, "size signal delivered");
    Manager.ThumbnailListUpdated.Disconnect(FirstId);
    Manager.ThumbnailListUpdated.Emit({L"Beta"}, {});
    Check.Expect(Calls == 11, "disconnected handler not called");

    Signal<> SelfDisconnecting;
    int SelfCalls = 0;
    size_t SelfId = 0;
    SelfId = SelfDisconnecting.Connect([&SelfDisconnecting, &SelfCalls, &SelfId]()
    {
        SelfCalls++;
        SelfDisconnecting.Disconnect(SelfId);
    });
    SelfDisconnecting.Emit();
    SelfDisconnecting.Emit();
    Check.Expect(SelfCalls == 1, "handler may disconnect itself while emitting");

    {
        const SingleInstanceGuard First;
        const SingleInstanceGuard Second;
        Check.Expect(First.IsFirstInstance() == true, "first guard owns the mutex");
        Check.Expect(Second.IsFirstInstance() == false, "second guard is rejected");
    }

    {
        const std::filesystem::path LogDirectory = std::filesystem::temp_directory_path() / L"EveOverlayLoggerProbe";
        std::error_code LogError;
        std::filesystem::remove_all(LogDirectory, LogError);
        std::filesystem::create_directories(LogDirectory, LogError);
        Logger::SetDirectory(LogDirectory);

        const std::chrono::sys_days Today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
        const std::filesystem::path Stale = LogDirectory / Logger::FormatFileName(std::chrono::year_month_day{Today - std::chrono::days(10)});
        const std::filesystem::path Kept = LogDirectory / Logger::FormatFileName(std::chrono::year_month_day{Today - std::chrono::days(Logger::RETENTION_DAYS - 1)});
        const std::filesystem::path Unrelated = LogDirectory / L"notes.log";
        for (const std::filesystem::path& Seed : {Stale, Kept, Unrelated})
        {
            std::ofstream(Seed) << "old";
        }

        Logger::Error("probe failure");
        Logger::Error("probe failure");

        Check.Expect(std::filesystem::exists(Stale) == false, "log older than the retention window is deleted");
        Check.Expect(std::filesystem::exists(Kept) == true, "log inside the retention window is kept");
        Check.Expect(std::filesystem::exists(Unrelated) == true, "unrelated files are never deleted");

        std::chrono::year_month_day Parsed;
        Check.Expect(Logger::TryParseFileName(L"Eve Overlay 2026-10-08.log", &Parsed) == true && static_cast<unsigned>(Parsed.day()) == 8, "log name parses");
        Check.Expect(Logger::TryParseFileName(L"Eve Overlay 2026-13-08.log", &Parsed) == false, "invalid month is rejected");

        std::ifstream Written(LogDirectory / Logger::FormatFileName(std::chrono::year_month_day{Today}));
        const std::string Content((std::istreambuf_iterator<char>(Written)), std::istreambuf_iterator<char>());
        Check.Expect(Content.find("[ERROR] probe failure") != std::string::npos, "error is written to today's log");
        Check.Expect(Content.find("probe failure") == Content.rfind("probe failure"), "an immediate repeat is suppressed");

        Logger::SetDirectory(std::filesystem::path());
        std::filesystem::remove_all(LogDirectory, LogError);
    }

    {
        CountdownTimers Timers;
        const int Short = Timers.Start("Short", 5000, 1000);
        const int Long = Timers.Start("Long", 600000, 1000);
        Check.Expect(Short != Long && Timers.GetTimers().size() == 2, "timers get their own ids");
        Check.Expect(Timers.Update(3000).empty() == true, "nothing runs out early");
        Check.Expect(Timers.GetRemaining(Timers.GetTimers()[0], 3000) == 3000, "remaining time counts down");

        const std::vector<std::string> RanOut = Timers.Update(6000);
        Check.Expect(RanOut.size() == 1 && RanOut[0] == "Short" && Timers.GetTimers()[0].Finished == true, "a timer reports once when it runs out");
        Check.Expect(Timers.Update(7000).empty() == true, "a finished timer does not report again");
        Check.Expect(Timers.GetRemaining(Timers.GetTimers()[0], 7000) == 0, "a finished timer has nothing left");

        Timers.Update(6000 + CountdownTimers::FINISHED_VISIBLE_MS);
        Check.Expect(Timers.GetTimers().size() == 1 && Timers.GetTimers()[0].Id == Long, "a long finished timer is dropped, a running one stays");
        Timers.Cancel(Long);
        Check.Expect(Timers.GetTimers().empty() == true, "a timer can be cancelled");

        Check.Expect(Timers.HasScan() == false && Timers.GetScanAge(5000) == 0, "no scan to begin with");
        Timers.MarkScan(10000);
        Check.Expect(Timers.HasScan() == true && Timers.GetScanAge(52000) == 42000, "scan age counts up from the mark");
        Timers.ClearScan();
        Check.Expect(Timers.HasScan() == false, "the scan can be cleared");

        Check.Expect(CountdownTimers::FormatDuration(0) == "0:00" && CountdownTimers::FormatDuration(1) == "0:01", "a partial second rounds up");
        Check.Expect(CountdownTimers::FormatDuration(245000) == "4:05" && CountdownTimers::FormatDuration(3723000) == "1:02:03", "minutes and hours are formatted");
    }

    CompositionRoot Root;
    Root.Initialize();
    Check.Expect(Root.GetConfiguration().ThumbnailRefreshPeriod >= 300, "root exposes loaded configuration");
    Check.Expect(&Root.GetWindowManager() != nullptr && &Root.GetProcessMonitor() != nullptr, "root exposes services");

    std::printf("%s (%d failed)\n", Check.FailureCount == 0 ? "ALL PASSED" : "FAILURES", Check.FailureCount);
    return Check.FailureCount;
}
