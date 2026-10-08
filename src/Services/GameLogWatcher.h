#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <Windows.h>

enum class GameLogEventKind
{
    Damage,
    WarpScramble,
    WarpDisruption
};

struct GameLogEvent
{
    std::string Character;
    GameLogEventKind Kind = GameLogEventKind::Damage;
    std::string Source;
};

// Tails the EVE game logs ("<yyyymmdd>_<hhmmss>_<character id>.txt", UTF-8, one file per character per session) and reports
// what is being done to each character: incoming damage and warp scramble or disruption attempts. The game writes these lines
// as HTML-like markup, e.g. "(combat) <color=0xffcc0000><b>43</b> <color=0x77ffffff><font size=10>from</font> ...".
class GameLogWatcher
{
public:
    static std::filesystem::path GetDefaultDirectory();

    void SetDirectory(std::filesystem::path NewDirectory);

    // How often the folder is listed to find new sessions; files already being followed are read on every poll.
    // The folder holds thousands of logs, so listing it is the expensive part.
    void SetScanInterval(const ULONGLONG Milliseconds);

    // Forgets every file position, so the next poll starts from where the logs currently end
    void Reset();

    // The first poll only records where each file ends, so old fights are not replayed
    std::vector<GameLogEvent> Poll();

    static bool TryParseLine(const std::string& Line, GameLogEventKind* const Kind, std::string* const Source);

    // The text with every <...> tag removed and runs of blanks folded into one
    static std::string StripMarkup(const std::string& Text);

private:
    static constexpr size_t HEADER_BYTES = 1024;
    static constexpr size_t MAX_SOURCE_LENGTH = 60;
    static constexpr ULONGLONG DEFAULT_SCAN_INTERVAL_MS = 5000;

    // Logs untouched for this long belong to finished sessions and are not followed
    static constexpr std::chrono::hours STALE_LOG_AGE{24};

    struct FileState
    {
        std::uintmax_t Offset = 0;
        std::string Listener;
    };

    static std::string ReadListener(const std::filesystem::path& Path);
    bool WasCreatedSinceStart(const std::filesystem::path& Path) const;
    void ScanDirectory();
    void ReadNewLines(const std::filesystem::path& Path, FileState& State, std::vector<GameLogEvent>& Events);

    std::filesystem::path Directory = GetDefaultDirectory();
    std::map<std::filesystem::path, FileState> Files;
    ULONGLONG ScanIntervalMs = DEFAULT_SCAN_INTERVAL_MS;
    ULONGLONG LastScanTick = 0;
    bool Started = false;
    unsigned long long StartedAt = 0;
};
