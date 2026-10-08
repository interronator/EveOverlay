#include "Application/Logger.h"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <system_error>

#include <Windows.h>

#include "Application/AppPaths.h"
#include "Config/TextUtil.h"

void Logger::Error(const std::string& Message)
{
    Write("ERROR", Message);
}

void Logger::Warning(const std::string& Message)
{
    Write("WARN ", Message);
}

void Logger::SetDirectory(const std::filesystem::path& NewDirectory)
{
    const std::lock_guard<std::timed_mutex> Guard(Lock);
    Directory = NewDirectory;
    PrunedDay = std::chrono::sys_days{};
    LastEntry.clear();
    SuppressedCount = 0;
}

std::filesystem::path Logger::GetDirectory()
{
    if (Directory.empty() == true)
    {
        return AppPaths::GetLogDirectory();
    }

    return Directory;
}

void Logger::PruneOldLogs()
{
    std::unique_lock<std::timed_mutex> Held(Lock, std::defer_lock);
    static_cast<void>(Held.try_lock_for(std::chrono::milliseconds(LOCK_TIMEOUT_MILLISECONDS)));
    PruneLocked(std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now()));
}

std::wstring Logger::FormatFileName(const std::chrono::year_month_day Day)
{
    wchar_t Buffer[64] = {};
    std::swprintf(Buffer, sizeof(Buffer) / sizeof(Buffer[0]), L"Eve Overlay %04d-%02u-%02u.log",
        static_cast<int>(Day.year()), static_cast<unsigned>(Day.month()), static_cast<unsigned>(Day.day()));
    return Buffer;
}

bool Logger::TryParseFileName(const std::wstring& Name, std::chrono::year_month_day* const Day)
{
    int Year = 0;
    unsigned Month = 0;
    unsigned DayOfMonth = 0;
    int Consumed = 0;
    if (::swscanf_s(Name.c_str(), L"Eve Overlay %4d-%2u-%2u.log%n", &Year, &Month, &DayOfMonth, &Consumed) != 3)
    {
        return false;
    }

    if (static_cast<size_t>(Consumed) != Name.size())
    {
        return false;
    }

    const std::chrono::year_month_day Parsed{std::chrono::year{Year}, std::chrono::month{Month}, std::chrono::day{DayOfMonth}};
    if (Parsed.ok() == false)
    {
        return false;
    }

    *Day = Parsed;
    return true;
}

void Logger::Write(const char* const Level, const std::string& Message)
{
    std::unique_lock<std::timed_mutex> Held(Lock, std::defer_lock);
    static_cast<void>(Held.try_lock_for(std::chrono::milliseconds(LOCK_TIMEOUT_MILLISECONDS)));

    const std::chrono::system_clock::time_point Now = std::chrono::system_clock::now();
    const std::chrono::steady_clock::time_point SteadyNow = std::chrono::steady_clock::now();

    const std::string Entry = std::string(Level) + Message;
    if (Entry == LastEntry && SteadyNow - LastEntryTime < std::chrono::seconds(REPEAT_SUPPRESSION_SECONDS))
    {
        SuppressedCount++;
        return;
    }

    const std::time_t Seconds = std::chrono::system_clock::to_time_t(Now);
    std::tm LocalTime = {};
    ::localtime_s(&LocalTime, &Seconds);

    const long long Milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(Now.time_since_epoch()).count() % 1000;
    char Stamp[48] = {};
    std::snprintf(Stamp, sizeof(Stamp), "%04d-%02d-%02d %02d:%02d:%02d.%03lld", LocalTime.tm_year + 1900, LocalTime.tm_mon + 1, LocalTime.tm_mday,
        LocalTime.tm_hour, LocalTime.tm_min, LocalTime.tm_sec, Milliseconds);

    const std::chrono::year_month_day Today{std::chrono::year{LocalTime.tm_year + 1900}, std::chrono::month{static_cast<unsigned>(LocalTime.tm_mon + 1)},
        std::chrono::day{static_cast<unsigned>(LocalTime.tm_mday)}};

    const std::filesystem::path LogDirectory = GetDirectory();
    std::error_code Error;
    std::filesystem::create_directories(LogDirectory, Error);

    const std::chrono::sys_days TodayDays{Today};
    if (TodayDays != PrunedDay)
    {
        PruneLocked(TodayDays);
    }

    std::ofstream Log(LogDirectory / FormatFileName(Today), std::ios::binary | std::ios::app);
    if (Log.is_open() == false)
    {
        return;
    }

    if (SuppressedCount > 0)
    {
        Log << Stamp << " [INFO ] previous message repeated " << SuppressedCount << " more times\r\n";
        SuppressedCount = 0;
    }

    Log << Stamp << " [" << Level << "] " << Message << "\r\n";

    LastEntry = Entry;
    LastEntryTime = SteadyNow;
}

void Logger::PruneLocked(const std::chrono::sys_days Today)
{
    PrunedDay = Today;

    std::error_code Error;
    std::filesystem::directory_iterator Iterator(GetDirectory(), Error);
    if (Error.value() != 0)
    {
        return;
    }

    const std::chrono::sys_days OldestKept = Today - std::chrono::days(RETENTION_DAYS - 1);
    for (; Iterator != std::filesystem::directory_iterator(); Iterator.increment(Error))
    {
        if (Error.value() != 0)
        {
            return;
        }

        std::chrono::year_month_day FileDay;
        if (TryParseFileName(Iterator->path().filename().wstring(), &FileDay) == false)
        {
            continue;
        }

        if (std::chrono::sys_days{FileDay} >= OldestKept)
        {
            continue;
        }

        std::error_code RemoveError;
        std::filesystem::remove(Iterator->path(), RemoveError);
    }
}

std::string Logger::DescribePath(const std::filesystem::path& Path)
{
    return TextUtil::ToUtf8(Path.wstring());
}
