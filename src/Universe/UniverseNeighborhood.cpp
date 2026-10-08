#include "Universe/UniverseNeighborhood.h"

#include <queue>
#include <unordered_map>

UniverseNeighborhood UniverseNeighborhood::Build(const UniverseData& Data, const int CenterSystem, const int MaxJumps)
{
    UniverseNeighborhood Result;
    if (CenterSystem == UniverseData::NOT_FOUND)
    {
        return Result;
    }

    std::unordered_map<int, int> SystemToNode;
    std::queue<int> Pending;
    Result.Nodes.push_back(NeighborhoodNode{CenterSystem, 0, -1});
    SystemToNode[CenterSystem] = 0;
    Pending.push(0);

    while (Pending.empty() == false)
    {
        const int NodeIndex = Pending.front();
        Pending.pop();

        const NeighborhoodNode Current = Result.Nodes[static_cast<size_t>(NodeIndex)];
        if (Current.Jumps >= MaxJumps)
        {
            continue;
        }

        for (const int Neighbour : Data.Get(Current.SystemIndex).Neighbours)
        {
            if (SystemToNode.contains(Neighbour) == true)
            {
                continue;
            }

            const int NewIndex = static_cast<int>(Result.Nodes.size());
            SystemToNode[Neighbour] = NewIndex;
            Result.Nodes.push_back(NeighborhoodNode{Neighbour, Current.Jumps + 1, NodeIndex});
            Pending.push(NewIndex);
        }
    }

    for (size_t NodeIndex = 0; NodeIndex < Result.Nodes.size(); NodeIndex++)
    {
        for (const int Neighbour : Data.Get(Result.Nodes[NodeIndex].SystemIndex).Neighbours)
        {
            const std::unordered_map<int, int>::const_iterator Match = SystemToNode.find(Neighbour);
            if (Match == SystemToNode.end() || Match->second < static_cast<int>(NodeIndex))
            {
                continue;
            }

            Result.Edges.push_back(NeighborhoodEdge{static_cast<int>(NodeIndex), Match->second});
        }
    }

    return Result;
}
