#include "Universe/ChatLogWatcher.h"

#include <fstream>
#include <utility>

#include <Windows.h>
#include <KnownFolders.h>
#include <ShlObj.h>

#include "Config/TextUtil.h"

void ChatLogWatcher::SetChannel(const std::string& NewFilter)
{
    if (TextUtil::EqualsIgnoreCase(NewFilter, ChannelFilter) == true)
    {
        return;
    }

    ChannelFilter = NewFilter;
    LoweredFilter = TextUtil::ToLower(NewFilter);
    Offsets.clear();
    Started = false;
}

void ChatLogWatcher::SetDirectory(std::filesystem::path NewDirectory)
{
    Directory = std::move(NewDirectory);
    Offsets.clear();
    Started = false;
}

std::filesystem::path ChatLogWatcher::GetDefaultDirectory()
{
    PWSTR Documents = nullptr;
    if (FAILED(::SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &Documents)))
    {
        ::CoTaskMemFree(Documents);
        return std::filesystem::path();
    }

    const std::filesystem::path Result = std::filesystem::path(Documents) / L"EVE" / L"logs" / L"Chatlogs";
    ::CoTaskMemFree(Documents);
    return Result;
}

std::vector<ChatMessage> ChatLogWatcher::Poll()
{
    std::vector<ChatMessage> Messages;
    if (ChannelFilter.empty() == true)
    {
        return Messages;
    }

    std::error_code Error;
    std::filesystem::directory_iterator Iterator(Directory, Error);
    if (Error.value() != 0)
    {
        return Messages;
    }

    if (Started == false)
    {
        FILETIME Now = {};
        ::GetSystemTimeAsFileTime(&Now);
        StartedAt = (static_cast<ULONGLONG>(Now.dwHighDateTime) << 32) | Now.dwLowDateTime;
    }

    const std::filesystem::file_time_type StaleBefore = std::filesystem::file_time_type::clock::now() - STALE_LOG_AGE;
    const std::filesystem::directory_iterator EndIterator;
    while (Iterator != EndIterator)
    {
        const std::filesystem::directory_entry Entry = *Iterator;
        Iterator.increment(Error);
        if (Error.value() != 0)
        {
            break;
        }

        if (::_wcsicmp(Entry.path().extension().c_str(), L".txt") != 0)
        {
            continue;
        }

        std::error_code TimeError;
        const std::filesystem::file_time_type LastWrite = Entry.last_write_time(TimeError);
        if (TimeError.value() == 0 && LastWrite < StaleBefore)
        {
            continue;
        }

        if (MatchesChannel(Entry.path()) == false)
        {
            continue;
        }

        ReadNewLines(Entry, Messages);
    }

    Started = true;
    return Messages;
}

bool ChatLogWatcher::TryParseLine(const std::string& Line, ChatMessage* const Result)
{
    if (Line.size() < 3 || Line[0] != '[')
    {
        return false;
    }

    const size_t Close = Line.find(']');
    if (Close == std::string::npos)
    {
        return false;
    }

    const size_t Separator = Line.find(" > ", Close);
    if (Separator == std::string::npos)
    {
        return false;
    }

    Result->Sender = TextUtil::Trim(Line.substr(Close + 1, Separator - Close - 1));
    Result->Text = TextUtil::Trim(Line.substr(Separator + 3));
    return true;
}

std::string ChatLogWatcher::DecodeUtf16(const std::string& Bytes)
{
    std::wstring Wide(reinterpret_cast<const wchar_t*>(Bytes.data()), Bytes.size() / sizeof(wchar_t));
    if (Wide.empty() == false && Wide[0] == 0xFEFF)
    {
        Wide.erase(0, 1);
    }

    return TextUtil::ToUtf8(Wide);
}

bool ChatLogWatcher::MatchesChannel(const std::filesystem::path& Path) const
{
    const std::string Stem = TextUtil::ToUtf8(Path.stem().wstring());
    const size_t Underscore = Stem.rfind('_');
    if (Underscore == std::string::npos)
    {
        return false;
    }

    // Strip "_<time>_<listener id>" and then "_<date>" to leave the channel name, which may itself contain underscores
    const size_t DatePosition = Stem.rfind('_', Underscore - 1);
    const size_t TimePosition = DatePosition == std::string::npos || DatePosition == 0 ? std::string::npos : Stem.rfind('_', DatePosition - 1);
    if (TimePosition == std::string::npos)
    {
        return false;
    }

    return TextUtil::ToLower(Stem.substr(0, TimePosition)).find(LoweredFilter) != std::string::npos;
}

bool ChatLogWatcher::WasCreatedSinceStart(const std::filesystem::path& Path) const
{
    WIN32_FILE_ATTRIBUTE_DATA Attributes = {};
    if (::GetFileAttributesExW(Path.c_str(), GetFileExInfoStandard, &Attributes) == FALSE)
    {
        return false;
    }

    const ULONGLONG Created = (static_cast<ULONGLONG>(Attributes.ftCreationTime.dwHighDateTime) << 32) | Attributes.ftCreationTime.dwLowDateTime;
    return Created >= StartedAt;
}

void ChatLogWatcher::ReadNewLines(const std::filesystem::directory_entry& Entry, std::vector<ChatMessage>& Messages)
{
    // The size must come from an open handle: while the game holds a log open, directory listings report a stale size
    std::ifstream Stream(Entry.path(), std::ios::binary);
    if (Stream.is_open() == false)
    {
        return;
    }

    Stream.seekg(0, std::ios::end);
    const std::streamoff EndPosition = Stream.tellg();
    if (EndPosition < 0)
    {
        return;
    }

    const std::uintmax_t Size = static_cast<std::uintmax_t>(EndPosition);

    const std::wstring Key = Entry.path().wstring();
    const std::unordered_map<std::wstring, std::uintmax_t>::iterator Known = Offsets.find(Key);
    if (Known == Offsets.end())
    {
        // Files created after the first poll are new sessions and are read from their start; an older file that was
        // merely idle (stale) or newly matching must not replay its history
        const bool IsNewSession = Started == true && WasCreatedSinceStart(Entry.path()) == true;
        const std::uintmax_t StartOffset = IsNewSession == true ? BYTE_ORDER_MARK_SIZE : Size - (Size % sizeof(wchar_t));
        Offsets[Key] = StartOffset;
        if (IsNewSession == false)
        {
            return;
        }
    }

    std::uintmax_t& Offset = Offsets[Key];
    if (Size < Offset)
    {
        Offset = BYTE_ORDER_MARK_SIZE;
    }

    if (Size - Offset < sizeof(wchar_t))
    {
        return;
    }

    Stream.seekg(static_cast<std::streamoff>(Offset));
    std::string Bytes(static_cast<size_t>(Size - Offset), '\0');
    Stream.read(Bytes.data(), static_cast<std::streamsize>(Bytes.size()));
    Bytes.resize(static_cast<size_t>(Stream.gcount()));
    Bytes.resize(Bytes.size() - Bytes.size() % sizeof(wchar_t));

    // Only consume up to the last complete line; the game may be mid-write
    const std::wstring Wide(reinterpret_cast<const wchar_t*>(Bytes.data()), Bytes.size() / sizeof(wchar_t));
    const size_t LastNewline = Wide.rfind(L'\n');
    if (LastNewline == std::wstring::npos)
    {
        return;
    }

    Offset += (LastNewline + 1) * sizeof(wchar_t);

    // EVE writes a byte order mark in front of every line, not just at the top of the file
    std::wstring Body = Wide.substr(0, LastNewline);
    std::erase(Body, static_cast<wchar_t>(0xFEFF));
    const std::string Text = TextUtil::ToUtf8(Body);
    size_t LineStart = 0;
    while (LineStart <= Text.size())
    {
        const size_t LineEnd = Text.find('\n', LineStart);
        const std::string Line = TextUtil::Trim(Text.substr(LineStart, LineEnd == std::string::npos ? std::string::npos : LineEnd - LineStart));
        LineStart = LineEnd == std::string::npos ? Text.size() + 1 : LineEnd + 1;

        ChatMessage Message;
        if (TryParseLine(Line, &Message) == false || Message.Sender == "EVE System")
        {
            continue;
        }

        Messages.push_back(std::move(Message));
    }
}
