#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct ChatMessage
{
    std::string Time;
    std::string Sender;
    std::string Text;
    std::string Channel;
    std::string Listener;
};

// Tails EVE chat logs (UTF-16LE, one file per channel per session, named "<Channel>_<yyyymmdd>_<hhmmss>_<listener id>.txt").
// Every file whose channel name contains one of the filters is followed, so several logged-in characters in the same channel work.
class ChatLogWatcher
{
public:
    // Several channels can be given at once, separated by commas or semicolons
    void SetChannel(const std::string& NewFilter);
    void SetDirectory(std::filesystem::path NewDirectory);

    // Local only reports the system a pilot is in through a line from "EVE System", so those lines are optionally delivered too
    void SetIncludeSystemMessages(const bool Include);

    // Matches the whole channel name instead of any name that contains the filter
    void SetExactChannelMatch(const bool Exact);

    static std::filesystem::path GetDefaultDirectory();

    // The first poll after a channel or directory change only records where each file currently ends, so old chatter is not replayed
    std::vector<ChatMessage> Poll();

    // The system named by the newest "Channel changed to" line across all matching logs from the last day; empty when there is none
    std::string FindCurrentSystem() const;

    static bool TryParseLine(const std::string& Line, ChatMessage* const Result);
    static std::string DecodeUtf16(const std::string& Bytes);

    // "Local : Jita" from "Channel changed to Local : Jita"; empty for any other text
    static std::string ParseChannelChange(const std::string& Text);

    // Every channel with a log written within MaxAge, most recently used first, one entry per name ignoring case. The names come
    // from the log file names, which the game pads with a date, a time and the listener's id.
    static std::vector<std::string> FindChannels(const std::filesystem::path& Directory, const std::chrono::hours MaxAge);

    // The part of a log file name in front of "_<date>_<time>_<listener id>"; empty when the name does not follow the pattern
    static std::string ExtractChannelName(const std::filesystem::path& Path);

private:
    static constexpr std::uintmax_t BYTE_ORDER_MARK_SIZE = 2;
    static constexpr size_t HEADER_BYTES = 1024;

    // Logs untouched for this long belong to finished sessions and are not opened
    static constexpr std::chrono::hours STALE_LOG_AGE{24};

    static constexpr std::uintmax_t TAIL_WINDOW_BYTES = 64 * 1024;

    // The time and system of the last "Channel changed to" line in one log; false when the log has none
    static bool FindLastChannelChange(const std::filesystem::path& Path, std::string* const Time, std::string* const System);

    bool MatchesChannel(const std::filesystem::path& Path) const;
    bool WasCreatedSinceStart(const std::filesystem::path& Path) const;
    std::string ReadListener(const std::filesystem::path& Path);
    void ReadNewLines(const std::filesystem::directory_entry& Entry, std::vector<ChatMessage>& Messages);

    std::filesystem::path Directory = GetDefaultDirectory();
    std::string ChannelFilter;
    std::vector<std::string> LoweredFilters;
    std::unordered_map<std::wstring, std::uintmax_t> Offsets;
    std::unordered_map<std::wstring, std::string> Listeners;
    bool Started = false;
    bool IncludeSystemMessages = false;
    bool ExactChannelMatch = false;
    unsigned long long StartedAt = 0;
};
