#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

// Single-threaded replacement for the MediatR notifications of the C# app. Not thread safe.
template <typename... Arguments>
class Signal
{
public:
    using Handler = std::function<void(Arguments...)>;

    size_t Connect(Handler NewHandler)
    {
        const size_t Id = NextId;
        NextId++;
        Handlers.emplace_back(Id, std::move(NewHandler));
        return Id;
    }

    void Disconnect(const size_t Id)
    {
        for (typename std::vector<Entry>::iterator Current = Handlers.begin(); Current != Handlers.end(); ++Current)
        {
            if (Current->first != Id)
            {
                continue;
            }

            Handlers.erase(Current);
            return;
        }
    }

    void Emit(Arguments... Values) const
    {
        const std::vector<Entry> Snapshot = Handlers;
        for (const Entry& Current : Snapshot)
        {
            Current.second(Values...);
        }
    }

private:
    using Entry = std::pair<size_t, Handler>;

    std::vector<Entry> Handlers;
    size_t NextId = 0;
};
