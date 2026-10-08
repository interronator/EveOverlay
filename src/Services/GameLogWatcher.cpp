#include "Services/GameLogWatcher.h"

#include <fstream>
#include <limits>
#include <utility>

#include <Windows.h>
#include <KnownFolders.h>
#include <ShlObj.h>

#include "Config/TextUtil.h"

std::filesystem::path GameLogWatcher::GetDefaultDirectory()
{
    PWSTR Documents = nullptr;
    if (FAILED(::SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &Documents)) == true)
    {
        ::CoTaskMemFree(Documents);
        return std::filesystem::path();
    }

    const std::filesystem::path Result = std::filesystem::path(Documents) / L"EVE" / L"logs" / L"Gamelogs";
    ::CoTaskMemFree(Documents);
    return Result;
}

void GameLogWatcher::SetDirectory(std::filesystem::path NewDirectory)
{
    Directory = std::move(NewDirectory);
    Reset();
}

void GameLogWatcher::Reset()
{
    Files.clear();
    Started = false;
}

void GameLogWatcher::SetScanInterval(const ULONGLONG Milliseconds)
{
    ScanIntervalMs = Milliseconds;
}

std::vector<GameLogEvent> GameLogWatcher::Poll()
{
    std::vector<GameLogEvent> Events;

    const ULONGLONG Now = ::GetTickCount64();
    if (Started == false || Now - LastScanTick >= ScanIntervalMs)
    {
        LastScanTick = Now;
        ScanDirectory();
    }

    for (std::map<std::filesystem::path, FileState>::iterator File = Files.begin(); File != Files.end(); ++File)
    {
        ReadNewLines(File->first, File->second, Events);
    }

    Started = true;
    return Events;
}

void GameLogWatcher::ScanDirectory()
{
    if (Started == false)
    {
        FILETIME Now = {};
        ::GetSystemTimeAsFileTime(&Now);
        StartedAt = (static_cast<ULONGLONG>(Now.dwHighDateTime) << 32) | Now.dwLowDateTime;
    }

    std::error_code Error;
    std::filesystem::directory_iterator Iterator(Directory, Error);
    if (Error.value() != 0)
    {
        Files.clear();
        return;
    }

    std::map<std::filesystem::path, FileState> Followed;
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
        const std::map<std::filesystem::path, FileState>::iterator Known = Files.find(Entry.path());
        if (Known != Files.end())
        {
            Followed.insert(Files.extract(Known));
            continue;
        }

        if (TimeError.value() == 0 && LastWrite < StaleBefore)
        {
            continue;
        }

        // A file created after the first scan is a new session and is read from its start; an older file must not replay its history
        FileState State;
        State.Listener = ReadListener(Entry.path());
        State.Offset = Started == true && WasCreatedSinceStart(Entry.path()) == true ? 0 : std::numeric_limits<std::uintmax_t>::max();
        Followed.emplace(Entry.path(), std::move(State));
    }

    Files = std::move(Followed);
}
std::string GameLogWatcher::StripMarkup(const std::string& Text)
{
    std::string Result;
    bool InsideTag = false;
    for (const char Character : Text)
    {
        if (Character == '<')
        {
            InsideTag = true;
            continue;
        }

        if (Character == '>' && InsideTag == true)
        {
            InsideTag = false;
            continue;
        }

        if (InsideTag == true)
        {
            continue;
        }

        const bool Blank = Character == ' ' || Character == '\t' || Character == '\r' || Character == '\n';
        if (Blank == true && (Result.empty() == true || Result.back() == ' '))
        {
            continue;
        }

        Result += Blank == true ? ' ' : Character;
    }

    return TextUtil::Trim(Result);
}

bool GameLogWatcher::TryParseLine(const std::string& Line, GameLogEventKind* const Kind, std::string* const Source)
{
    constexpr const char* COMBAT_MARKER = "] (combat) ";
    constexpr const char* FROM_MARKER = "from</font>";
    constexpr const char* DAMAGE_COLOR = "<color=0xffcc0000>";

    const size_t CombatPosition = Line.find(COMBAT_MARKER);
    if (Line.empty() == true || Line[0] != '[' || CombatPosition == std::string::npos)
    {
        return false;
    }

    const std::string Body = Line.substr(CombatPosition + std::char_traits<char>::length(COMBAT_MARKER));
    const size_t FromPosition = Body.find(FROM_MARKER);
    if (FromPosition == std::string::npos)
    {
        return false;
    }

    const std::string Head = Body.substr(0, FromPosition);
    if (Head.find("Warp scramble attempt") != std::string::npos)
    {
        *Kind = GameLogEventKind::WarpScramble;
    }
    else if (Head.find("Warp disruption attempt") != std::string::npos)
    {
        *Kind = GameLogEventKind::WarpDisruption;
    }
    else if (TextUtil::ToLower(Head.substr(0, std::char_traits<char>::length(DAMAGE_COLOR))) == DAMAGE_COLOR)
    {
        *Kind = GameLogEventKind::Damage;
    }
    else
    {
        return false;
    }

    std::string Remainder = StripMarkup(Body.substr(FromPosition + std::char_traits<char>::length(FROM_MARKER)));
    const size_t WeaponSeparator = Remainder.find(" - ");
    if (WeaponSeparator != std::string::npos)
    {
        Remainder.resize(WeaponSeparator);
    }

    if (Remainder.size() > MAX_SOURCE_LENGTH)
    {
        Remainder.resize(MAX_SOURCE_LENGTH);
    }

    *Source = Remainder;
    return true;
}

std::string GameLogWatcher::ReadListener(const std::filesystem::path& Path)
{
    std::ifstream Stream(Path, std::ios::binary);
    if (Stream.is_open() == false)
    {
        return std::string();
    }

    std::string Header(HEADER_BYTES, '\0');
    Stream.read(Header.data(), static_cast<std::streamsize>(Header.size()));
    Header.resize(static_cast<size_t>(Stream.gcount()));

    constexpr const char* MARKER = "Listener:";
    const size_t MarkerPosition = Header.find(MARKER);
    if (MarkerPosition == std::string::npos)
    {
        return std::string();
    }

    const size_t ValueStart = MarkerPosition + std::char_traits<char>::length(MARKER);
    const size_t LineEnd = Header.find_first_of("\r\n", ValueStart);
    return TextUtil::Trim(Header.substr(ValueStart, LineEnd == std::string::npos ? std::string::npos : LineEnd - ValueStart));
}

bool GameLogWatcher::WasCreatedSinceStart(const std::filesystem::path& Path) const
{
    WIN32_FILE_ATTRIBUTE_DATA Attributes = {};
    if (::GetFileAttributesExW(Path.c_str(), GetFileExInfoStandard, &Attributes) == FALSE)
    {
        return false;
    }

    const ULONGLONG Created = (static_cast<ULONGLONG>(Attributes.ftCreationTime.dwHighDateTime) << 32) | Attributes.ftCreationTime.dwLowDateTime;
    return Created >= StartedAt;
}

void GameLogWatcher::ReadNewLines(const std::filesystem::path& Path, FileState& State, std::vector<GameLogEvent>& Events)
{
    // The size must come from an open handle: while the game holds a log open, directory listings report a stale size
    std::ifstream Stream(Path, std::ios::binary);
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
    constexpr std::uintmax_t UNSET_OFFSET = std::numeric_limits<std::uintmax_t>::max();
    if (State.Offset == UNSET_OFFSET)
    {
        State.Offset = Size;
        return;
    }

    if (Size < State.Offset)
    {
        State.Offset = 0;
    }

    if (Size == State.Offset)
    {
        return;
    }

    if (State.Listener.empty() == true)
    {
        State.Listener = ReadListener(Path);
    }

    Stream.seekg(static_cast<std::streamoff>(State.Offset));
    std::string Bytes(static_cast<size_t>(Size - State.Offset), '\0');
    Stream.read(Bytes.data(), static_cast<std::streamsize>(Bytes.size()));
    Bytes.resize(static_cast<size_t>(Stream.gcount()));

    // Only consume up to the last complete line; the game may be mid-write
    const size_t LastNewline = Bytes.rfind('\n');
    if (LastNewline == std::string::npos)
    {
        return;
    }

    State.Offset += LastNewline + 1;
    if (State.Listener.empty() == true)
    {
        return;
    }

    for (const std::string& RawLine : TextUtil::Split(Bytes.substr(0, LastNewline), '\n'))
    {
        GameLogEvent Event;
        if (TryParseLine(TextUtil::Trim(RawLine), &Event.Kind, &Event.Source) == false)
        {
            continue;
        }

        Event.Character = State.Listener;
        Events.push_back(std::move(Event));
    }
}