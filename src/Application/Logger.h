#pragma once

#include <chrono>
#include <filesystem>
#include <mutex>
#include <string>

// Appends errors and warnings to one log file per day under "logs" next to the exe and deletes the files that fall out of the retention window
class Logger
{
public:
    static constexpr int RETENTION_DAYS = 7;

    static void Error(const std::string& Message);
    static void Warning(const std::string& Message);
    static void Info(const std::string& Message);

    // Defaults to the logs folder next to the exe
    static void SetDirectory(const std::filesystem::path& NewDirectory);
    static std::filesystem::path GetDirectory();

    static void PruneOldLogs();

    static std::string DescribePath(const std::filesystem::path& Path);

    static std::wstring FormatFileName(const std::chrono::year_month_day Day);
    static bool TryParseFileName(const std::wstring& Name, std::chrono::year_month_day* const Day);

private:
    static constexpr int REPEAT_SUPPRESSION_SECONDS = 60;
    static constexpr int LOCK_TIMEOUT_MILLISECONDS = 2000;

    static void Write(const char* const Level, const std::string& Message);
    static void PruneLocked(const std::chrono::sys_days Today);

    inline static std::timed_mutex Lock;
    inline static std::filesystem::path Directory;
    inline static std::chrono::sys_days PrunedDay{};
    inline static std::string LastEntry;
    inline static std::chrono::steady_clock::time_point LastEntryTime;
    inline static int SuppressedCount = 0;
};
