#pragma once

#include <vector>

#include "Universe/UniverseNeighborhood.h"

struct LayoutPoint
{
    double X = 0.0;
    double Y = 0.0;
};

// Positions are normalised so the largest extent from the centre system (always at the origin) is 1.0
class UniverseLayout
{
public:
    static std::vector<LayoutPoint> Compute(const UniverseNeighborhood& Neighborhood);

private:
    static constexpr double PI = 3.14159265358979323846;
    static constexpr int ITERATIONS = 250;
    static constexpr int MINIMUM_ITERATIONS = 30;
    static constexpr double PAIR_BUDGET = 60000000.0;
    static constexpr double IDEAL_LENGTH = 1.0;
    static constexpr double CENTER_PULL = 0.02;
    static constexpr double MINIMUM_DISTANCE = 0.01;

    static std::vector<LayoutPoint> SeedOnRings(const UniverseNeighborhood& Neighborhood);

    // Fruchterman-Reingold with the centre system pinned at the origin
    static void Relax(const UniverseNeighborhood& Neighborhood, std::vector<LayoutPoint>& Points);

    static void Normalize(std::vector<LayoutPoint>& Points);
};
