#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

struct ChatMessage
{
    std::string Sender;
    std::string Text;
};

// Tails EVE chat logs (UTF-16LE, one file per channel per session, named "<Channel>_<yyyymmdd>_<hhmmss>_<listener id>.txt").
// Every file whose channel name contains the filter is followed, so several logged-in characters in the same channel work.
class ChatLogWatcher
{
public:
    void SetChannel(const std::string& NewFilter);
    void SetDirectory(std::filesystem::path NewDirectory);

    static std::filesystem::path GetDefaultDirectory();

    // The first poll after a channel or directory change only records where each file currently ends, so old chatter is not replayed
    std::vector<ChatMessage> Poll();

    static bool TryParseLine(const std::string& Line, ChatMessage* const Result);
    static std::string DecodeUtf16(const std::string& Bytes);

private:
    static constexpr std::uintmax_t BYTE_ORDER_MARK_SIZE = 2;

    // Logs untouched for this long belong to finished sessions and are not opened
    static constexpr std::chrono::hours STALE_LOG_AGE{24};

    bool MatchesChannel(const std::filesystem::path& Path) const;
    bool WasCreatedSinceStart(const std::filesystem::path& Path) const;
    void ReadNewLines(const std::filesystem::directory_entry& Entry, std::vector<ChatMessage>& Messages);

    std::filesystem::path Directory = GetDefaultDirectory();
    std::string ChannelFilter;
    std::string LoweredFilter;
    std::unordered_map<std::wstring, std::uintmax_t> Offsets;
    bool Started = false;
    unsigned long long StartedAt = 0;
};
