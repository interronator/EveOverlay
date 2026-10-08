#pragma once

#include <vector>

#include "Universe/UniverseData.h"

struct NeighborhoodNode
{
    int SystemIndex = 0;
    int Jumps = 0;
    int Parent = -1;
};

struct NeighborhoodEdge
{
    int From = 0;
    int To = 0;
};

// Nodes are indices into Nodes; Nodes[0] is always the centre system. Edges reference node indices, not system indices.
class UniverseNeighborhood
{
public:
    static UniverseNeighborhood Build(const UniverseData& Data, const int CenterSystem, const int MaxJumps);

    std::vector<NeighborhoodNode> Nodes;
    std::vector<NeighborhoodEdge> Edges;
};
