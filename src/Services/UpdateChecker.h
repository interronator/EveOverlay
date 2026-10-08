#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <Windows.h>
#include <winhttp.h>

// Asks GitHub for the newest release on a background thread, only when Check is called
class UpdateChecker
{
public:
    enum class State
    {
        Idle,
        Checking,
        UpToDate,
        Available,
        Failed
    };

    UpdateChecker() = default;
    UpdateChecker(const UpdateChecker&) = delete;
    UpdateChecker& operator=(const UpdateChecker&) = delete;

    ~UpdateChecker();

    // "v1.2.3" or "1.2.3" into its numbers; false when it is not a version
    static bool ParseVersion(const std::string& Text, std::vector<int>* const Numbers);

    // True when Latest is a higher version than Current; versions that cannot be read are never newer
    static bool IsNewer(const std::string& Latest, const std::string& Current);

    // Reads the tag and page address out of a GitHub "latest release" response
    static bool ParseRelease(const std::string& Body, std::string* const Tag, std::string* const Url);

    void Check(const std::string& CurrentVersion);
    State GetState() const;
    std::string GetLatestVersion() const;
    std::string GetReleaseUrl() const;

private:
    static constexpr DWORD TIMEOUT_MS = 5000;

    class HttpHandle
    {
    public:
        explicit HttpHandle(const HINTERNET NewHandle);
        HttpHandle(const HttpHandle&) = delete;
        HttpHandle& operator=(const HttpHandle&) = delete;

        ~HttpHandle();

        HINTERNET Get() const;

    private:
        const HINTERNET Handle;
    };

    static bool Fetch(std::string& Response, DWORD& StatusCode);

    void Run(const std::string& CurrentVersion);

    mutable std::mutex Lock;
    std::string LatestVersion;
    std::string ReleaseUrl;
    std::atomic<State> Current{State::Idle};
    std::thread Worker;
};
