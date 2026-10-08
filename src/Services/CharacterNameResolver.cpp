#include "Services/CharacterNameResolver.h"

#include <algorithm>

#include "Application/Logger.h"

#include <nlohmann/json.hpp>

CharacterNameResolver::HttpHandle::HttpHandle(const HINTERNET NewHandle)
    : Handle(NewHandle)
{
}

CharacterNameResolver::HttpHandle::~HttpHandle()
{
    if (Handle != nullptr)
    {
        ::WinHttpCloseHandle(Handle);
    }
}

HINTERNET CharacterNameResolver::HttpHandle::Get() const
{
    return Handle;
}

CharacterNameResolver::~CharacterNameResolver()
{
    Cancelled.store(true);

    if (Worker.joinable() == true)
    {
        Worker.join();
    }
}

std::map<long long, std::string> CharacterNameResolver::ParseNames(const std::string& Body)
{
    std::map<long long, std::string> Names;
    const nlohmann::json Root = nlohmann::json::parse(Body, nullptr, false);
    if (Root.is_array() == false)
    {
        return Names;
    }

    for (const nlohmann::json& Entry : Root)
    {
        if (Entry.is_object() == false || Entry.value("category", std::string()) != "character")
        {
            continue;
        }

        const nlohmann::json Identifier = Entry.value("id", nlohmann::json());
        const nlohmann::json Name = Entry.value("name", nlohmann::json());
        if (Identifier.is_number_integer() == false || Name.is_string() == false)
        {
            continue;
        }

        Names[Identifier.get<long long>()] = Name.get<std::string>();
    }

    return Names;
}

void CharacterNameResolver::Seed(const std::map<long long, std::string>& KnownNames)
{
    const std::lock_guard<std::mutex> Guard(Lock);
    for (const std::pair<const long long, std::string>& Known : KnownNames)
    {
        Names[Known.first] = Known.second;
    }
}

std::string CharacterNameResolver::GetName(const long long Identifier) const
{
    const std::lock_guard<std::mutex> Guard(Lock);
    const std::map<long long, std::string>::const_iterator Found = Names.find(Identifier);
    return Found == Names.end() ? std::string() : Found->second;
}

std::map<long long, std::string> CharacterNameResolver::GetAll() const
{
    const std::lock_guard<std::mutex> Guard(Lock);
    return Names;
}

bool CharacterNameResolver::IsRunning() const
{
    return Running.load();
}

bool CharacterNameResolver::ConsumeUpdated()
{
    return Updated.exchange(false);
}

void CharacterNameResolver::ForgetFailures()
{
    const std::lock_guard<std::mutex> Guard(Lock);
    Attempted.clear();
}

void CharacterNameResolver::Request(const std::vector<long long>& Identifiers)
{
    if (Running.load() == true)
    {
        return;
    }

    std::vector<long long> Pending;
    {
        const std::lock_guard<std::mutex> Guard(Lock);
        for (const long long Identifier : Identifiers)
        {
            if (Identifier == 0 || Names.contains(Identifier) == true || Attempted.contains(Identifier) == true)
            {
                continue;
            }

            Attempted.insert(Identifier);
            Pending.push_back(Identifier);
        }
    }

    if (Pending.empty() == true)
    {
        return;
    }

    if (Worker.joinable() == true)
    {
        Worker.join();
    }

    Running.store(true);
    Worker = std::thread([this, Pending]()
    {
        Resolve(Pending);
        Running.store(false);
    });
}

void CharacterNameResolver::Resolve(const std::vector<long long>& Identifiers)
{
    for (size_t Start = 0; Start < Identifiers.size() && Cancelled.load() == false; Start += MAX_IDENTIFIERS_PER_REQUEST)
    {
        const size_t End = std::min(Start + MAX_IDENTIFIERS_PER_REQUEST, Identifiers.size());
        ResolveBatch(std::vector<long long>(Identifiers.begin() + static_cast<std::ptrdiff_t>(Start), Identifiers.begin() + static_cast<std::ptrdiff_t>(End)));
    }
}

void CharacterNameResolver::ResolveBatch(const std::vector<long long>& Identifiers)
{
    DWORD StatusCode = 0;
    std::string Response;
    if (Post(Identifiers, Response, StatusCode) == false)
    {
        Logger::Warning("Character name lookup failed to reach ESI, Windows error " + std::to_string(::GetLastError()));
        return;
    }

    if (StatusCode != 200 && StatusCode != HTTP_NOT_FOUND)
    {
        Logger::Warning("Character name lookup returned HTTP " + std::to_string(StatusCode));
    }

    if (StatusCode == HTTP_NOT_FOUND && Identifiers.size() > 1)
    {
        for (const long long Identifier : Identifiers)
        {
            if (Cancelled.load() == true)
            {
                return;
            }

            Response.clear();
            if (Post(std::vector<long long>{Identifier}, Response, StatusCode) == true && StatusCode == 200)
            {
                Store(ParseNames(Response));
            }
        }

        return;
    }

    if (StatusCode == 200)
    {
        Store(ParseNames(Response));
    }
}

void CharacterNameResolver::Store(const std::map<long long, std::string>& Found)
{
    if (Found.empty() == true)
    {
        return;
    }

    {
        const std::lock_guard<std::mutex> Guard(Lock);
        for (const std::pair<const long long, std::string>& Entry : Found)
        {
            Names[Entry.first] = Entry.second;
        }
    }

    Updated.store(true);
}

bool CharacterNameResolver::Post(const std::vector<long long>& Identifiers, std::string& Response, DWORD& StatusCode)
{
    const std::string Body = nlohmann::json(Identifiers).dump();

    const HttpHandle Session(::WinHttpOpen(L"EveOverlay/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (Session.Get() == nullptr)
    {
        return false;
    }

    ::WinHttpSetTimeouts(Session.Get(), CONNECT_TIMEOUT_MS, CONNECT_TIMEOUT_MS, TRANSFER_TIMEOUT_MS, TRANSFER_TIMEOUT_MS);

    const HttpHandle Connection(::WinHttpConnect(Session.Get(), L"esi.evetech.net", INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (Connection.Get() == nullptr)
    {
        return false;
    }

    const HttpHandle Request(::WinHttpOpenRequest(Connection.Get(), L"POST", L"/latest/universe/names/", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (Request.Get() == nullptr)
    {
        return false;
    }

    const wchar_t* const Headers = L"Content-Type: application/json\r\nAccept: application/json";
    const DWORD BodyLength = static_cast<DWORD>(Body.size());
    if (::WinHttpSendRequest(Request.Get(), Headers, static_cast<DWORD>(-1), const_cast<char*>(Body.data()), BodyLength, BodyLength, 0) == FALSE
        || ::WinHttpReceiveResponse(Request.Get(), nullptr) == FALSE)
    {
        return false;
    }

    DWORD StatusSize = sizeof(StatusCode);
    if (::WinHttpQueryHeaders(Request.Get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &StatusCode, &StatusSize, WINHTTP_NO_HEADER_INDEX) == FALSE)
    {
        return false;
    }

    while (true)
    {
        DWORD Available = 0;
        if (::WinHttpQueryDataAvailable(Request.Get(), &Available) == FALSE)
        {
            return false;
        }

        if (Available == 0)
        {
            return true;
        }

        std::string Chunk(Available, '\0');
        DWORD BytesRead = 0;
        if (::WinHttpReadData(Request.Get(), Chunk.data(), Available, &BytesRead) == FALSE)
        {
            return false;
        }

        Response.append(Chunk, 0, BytesRead);
    }
}
