#include "Universe/UniverseLayout.h"

#include <algorithm>
#include <cmath>

std::vector<LayoutPoint> UniverseLayout::Compute(const UniverseNeighborhood& Neighborhood)
{
    std::vector<LayoutPoint> Points = SeedOnRings(Neighborhood);
    Relax(Neighborhood, Points);
    Normalize(Points);
    return Points;
}

std::vector<LayoutPoint> UniverseLayout::SeedOnRings(const UniverseNeighborhood& Neighborhood)
{
    const size_t Count = Neighborhood.Nodes.size();
    std::vector<std::vector<size_t>> Children(Count);
    for (size_t Index = 1; Index < Count; Index++)
    {
        Children[static_cast<size_t>(Neighborhood.Nodes[Index].Parent)].push_back(Index);
    }

    std::vector<double> SectorStart(Count, 0.0);
    std::vector<double> SectorWidth(Count, 2.0 * PI);
    std::vector<LayoutPoint> Points(Count);

    // Nodes are stored in BFS order, so a parent is always placed before its children
    for (size_t Index = 0; Index < Count; Index++)
    {
        const double MiddleAngle = SectorStart[Index] + SectorWidth[Index] / 2.0;
        const double Radius = static_cast<double>(Neighborhood.Nodes[Index].Jumps) * IDEAL_LENGTH;
        Points[Index] = LayoutPoint{Radius * std::cos(MiddleAngle), Radius * std::sin(MiddleAngle)};

        const std::vector<size_t>& Kids = Children[Index];
        if (Kids.empty() == true)
        {
            continue;
        }

        const double ChildWidth = SectorWidth[Index] / static_cast<double>(Kids.size());
        for (size_t KidIndex = 0; KidIndex < Kids.size(); KidIndex++)
        {
            SectorStart[Kids[KidIndex]] = SectorStart[Index] + ChildWidth * static_cast<double>(KidIndex);
            SectorWidth[Kids[KidIndex]] = ChildWidth;
        }
    }

    return Points;
}

void UniverseLayout::Relax(const UniverseNeighborhood& Neighborhood, std::vector<LayoutPoint>& Points)
{
    const size_t Count = Points.size();
    if (Count < 3)
    {
        return;
    }

    std::vector<LayoutPoint> Forces(Count);
    double Temperature = IDEAL_LENGTH * 0.5;
    // The repulsion pass is quadratic, so large neighbourhoods get fewer iterations to keep the UI thread responsive
    const double PairCount = static_cast<double>(Count) * static_cast<double>(Count - 1) / 2.0;
    const int Iterations = std::clamp(static_cast<int>(PAIR_BUDGET / PairCount), MINIMUM_ITERATIONS, ITERATIONS);
    const double Cooling = Temperature / static_cast<double>(Iterations);
    const double MinimumDistanceSquared = MINIMUM_DISTANCE * MINIMUM_DISTANCE;

    for (int Iteration = 0; Iteration < Iterations; Iteration++)
    {
        std::fill(Forces.begin(), Forces.end(), LayoutPoint{});

        for (size_t First = 0; First < Count; First++)
        {
            for (size_t Second = First + 1; Second < Count; Second++)
            {
                const double DeltaX = Points[First].X - Points[Second].X;
                const double DeltaY = Points[First].Y - Points[Second].Y;
                const double DistanceSquared = std::max(DeltaX * DeltaX + DeltaY * DeltaY, MinimumDistanceSquared);
                const double Push = IDEAL_LENGTH * IDEAL_LENGTH / DistanceSquared;
                Forces[First].X += DeltaX * Push;
                Forces[First].Y += DeltaY * Push;
                Forces[Second].X -= DeltaX * Push;
                Forces[Second].Y -= DeltaY * Push;
            }
        }

        for (const NeighborhoodEdge& Edge : Neighborhood.Edges)
        {
            const size_t From = static_cast<size_t>(Edge.From);
            const size_t To = static_cast<size_t>(Edge.To);
            const double DeltaX = Points[From].X - Points[To].X;
            const double DeltaY = Points[From].Y - Points[To].Y;
            const double Distance = std::max(std::hypot(DeltaX, DeltaY), MINIMUM_DISTANCE);
            const double Pull = Distance / IDEAL_LENGTH;
            Forces[From].X -= DeltaX * Pull;
            Forces[From].Y -= DeltaY * Pull;
            Forces[To].X += DeltaX * Pull;
            Forces[To].Y += DeltaY * Pull;
        }

        for (size_t Index = 1; Index < Count; Index++)
        {
            Forces[Index].X -= Points[Index].X * CENTER_PULL;
            Forces[Index].Y -= Points[Index].Y * CENTER_PULL;

            const double Magnitude = std::hypot(Forces[Index].X, Forces[Index].Y);
            if (Magnitude < MINIMUM_DISTANCE * 0.001)
            {
                continue;
            }

            const double Step = std::min(Magnitude, Temperature);
            Points[Index].X += Forces[Index].X / Magnitude * Step;
            Points[Index].Y += Forces[Index].Y / Magnitude * Step;
        }

        Temperature -= Cooling;
    }
}

void UniverseLayout::Normalize(std::vector<LayoutPoint>& Points)
{
    double Extent = 0.0;
    for (const LayoutPoint& Point : Points)
    {
        Extent = std::max(Extent, std::hypot(Point.X, Point.Y));
    }

    if (Extent <= 0.0)
    {
        return;
    }

    for (LayoutPoint& Point : Points)
    {
        Point.X /= Extent;
        Point.Y /= Extent;
    }
}
