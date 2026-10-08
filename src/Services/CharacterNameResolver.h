#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <Windows.h>
#include <winhttp.h>

// Resolves character ids to names through the public ESI API on a background thread and remembers what it learned
class CharacterNameResolver
{
public:
    CharacterNameResolver() = default;
    CharacterNameResolver(const CharacterNameResolver&) = delete;
    CharacterNameResolver& operator=(const CharacterNameResolver&) = delete;

    ~CharacterNameResolver();

    // Reads the characters out of an ESI /universe/names/ response; anything that is not a character is ignored
    static std::map<long long, std::string> ParseNames(const std::string& Body);

    void Seed(const std::map<long long, std::string>& KnownNames);
    std::string GetName(const long long Identifier) const;
    std::map<long long, std::string> GetAll() const;
    bool IsRunning() const;

    // True once after new names arrived, so the owner knows to save them
    bool ConsumeUpdated();

    void ForgetFailures();

    // Looks up the ids that are neither known nor already tried; does nothing while a lookup is still running
    void Request(const std::vector<long long>& Identifiers);

private:
    static constexpr DWORD CONNECT_TIMEOUT_MS = 3000;
    static constexpr DWORD TRANSFER_TIMEOUT_MS = 5000;
    static constexpr DWORD HTTP_NOT_FOUND = 404;
    static constexpr size_t MAX_IDENTIFIERS_PER_REQUEST = 1000;

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

    static bool Post(const std::vector<long long>& Identifiers, std::string& Response, DWORD& StatusCode);

    // A batch containing an unknown id is rejected as a whole, so a failed batch is retried one id at a time
    void Resolve(const std::vector<long long>& Identifiers);
    void ResolveBatch(const std::vector<long long>& Identifiers);
    void Store(const std::map<long long, std::string>& Found);

    mutable std::mutex Lock;
    std::map<long long, std::string> Names;
    std::set<long long> Attempted;
    std::thread Worker;
    std::atomic<bool> Running{false};
    std::atomic<bool> Updated{false};
    std::atomic<bool> Cancelled{false};
};
