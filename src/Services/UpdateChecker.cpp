#include "Services/UpdateChecker.h"

#include <algorithm>
#include <utility>

#include <winhttp.h>

#include <nlohmann/json.hpp>

#include "Application/Logger.h"
#include "Config/TextUtil.h"

UpdateChecker::HttpHandle::HttpHandle(const HINTERNET NewHandle)
    : Handle(NewHandle)
{
}

UpdateChecker::HttpHandle::~HttpHandle()
{
    if (Handle != nullptr)
    {
        ::WinHttpCloseHandle(Handle);
    }
}

HINTERNET UpdateChecker::HttpHandle::Get() const
{
    return Handle;
}

UpdateChecker::~UpdateChecker()
{
    if (Worker.joinable() == true)
    {
        Worker.join();
    }
}

bool UpdateChecker::ParseVersion(const std::string& Text, std::vector<int>* const Numbers)
{
    std::string Trimmed = TextUtil::Trim(Text);
    if (Trimmed.empty() == false && (Trimmed[0] == 'v' || Trimmed[0] == 'V'))
    {
        Trimmed.erase(0, 1);
    }

    std::vector<int> Parsed;
    for (const std::string& Part : TextUtil::Split(Trimmed, '.'))
    {
        int Number = 0;
        if (TextUtil::TryParseInt(Part, &Number) == false || Number < 0)
        {
            return false;
        }

        Parsed.push_back(Number);
    }

    if (Parsed.empty() == true)
    {
        return false;
    }

    *Numbers = std::move(Parsed);
    return true;
}

bool UpdateChecker::IsNewer(const std::string& Latest, const std::string& CurrentVersion)
{
    std::vector<int> LatestNumbers;
    std::vector<int> CurrentNumbers;
    if (ParseVersion(Latest, &LatestNumbers) == false || ParseVersion(CurrentVersion, &CurrentNumbers) == false)
    {
        return false;
    }

    const size_t Length = std::max(LatestNumbers.size(), CurrentNumbers.size());
    for (size_t Index = 0; Index < Length; Index++)
    {
        const int LatestPart = Index < LatestNumbers.size() ? LatestNumbers[Index] : 0;
        const int CurrentPart = Index < CurrentNumbers.size() ? CurrentNumbers[Index] : 0;
        if (LatestPart != CurrentPart)
        {
            return LatestPart > CurrentPart;
        }
    }

    return false;
}

bool UpdateChecker::ParseRelease(const std::string& Body, std::string* const Tag, std::string* const Url)
{
    const nlohmann::json Root = nlohmann::json::parse(Body, nullptr, false);
    if (Root.is_object() == false || Root.contains("tag_name") == false || Root["tag_name"].is_string() == false)
    {
        return false;
    }

    *Tag = Root["tag_name"].get<std::string>();
    *Url = Root.contains("html_url") == true && Root["html_url"].is_string() == true ? Root["html_url"].get<std::string>() : std::string();
    return true;
}

void UpdateChecker::Check(const std::string& CurrentVersion)
{
    if (Current.load() == State::Checking)
    {
        return;
    }

    if (Worker.joinable() == true)
    {
        Worker.join();
    }

    Current.store(State::Checking);
    Worker = std::thread([this, CurrentVersion]()
    {
        Run(CurrentVersion);
    });
}

UpdateChecker::State UpdateChecker::GetState() const
{
    return Current.load();
}

std::string UpdateChecker::GetLatestVersion() const
{
    const std::lock_guard<std::mutex> Guard(Lock);
    return LatestVersion;
}

std::string UpdateChecker::GetReleaseUrl() const
{
    const std::lock_guard<std::mutex> Guard(Lock);
    return ReleaseUrl;
}

void UpdateChecker::Run(const std::string& CurrentVersion)
{
    std::string Response;
    DWORD StatusCode = 0;
    std::string Tag;
    std::string Url;
    if (Fetch(Response, StatusCode) == false || StatusCode != 200 || ParseRelease(Response, &Tag, &Url) == false)
    {
        Logger::Warning("Update check failed, HTTP " + std::to_string(StatusCode));
        Current.store(State::Failed);
        return;
    }

    {
        const std::lock_guard<std::mutex> Guard(Lock);
        LatestVersion = Tag;
        ReleaseUrl = Url;
    }

    Current.store(IsNewer(Tag, CurrentVersion) == true ? State::Available : State::UpToDate);
}

bool UpdateChecker::Fetch(std::string& Response, DWORD& StatusCode)
{
    const UpdateChecker::HttpHandle Session(::WinHttpOpen(L"EveOverlay", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (Session.Get() == nullptr)
    {
        return false;
    }

    ::WinHttpSetTimeouts(Session.Get(), TIMEOUT_MS, TIMEOUT_MS, TIMEOUT_MS, TIMEOUT_MS);

    const UpdateChecker::HttpHandle Connection(::WinHttpConnect(Session.Get(), L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (Connection.Get() == nullptr)
    {
        return false;
    }

    const UpdateChecker::HttpHandle Request(::WinHttpOpenRequest(Connection.Get(), L"GET", L"/repos/interronator/EveOverlay/releases/latest", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE));
    if (Request.Get() == nullptr)
    {
        return false;
    }

    const wchar_t* const Headers = L"Accept: application/vnd.github+json";
    if (::WinHttpSendRequest(Request.Get(), Headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE || ::WinHttpReceiveResponse(Request.Get(), nullptr) == FALSE)
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
