#include "Config/ThumbnailArrangement.h"

#include <algorithm>
#include <cstdlib>

bool GridShape::operator==(const GridShape& Other) const
{
    return Columns == Other.Columns && Rows == Other.Rows && PartialRowFirst == Other.PartialRowFirst;
}

std::vector<GridShape> ThumbnailArranger::GetShapes(const int Count)
{
    std::vector<GridShape> Shapes;
    if (Count < 1)
    {
        return Shapes;
    }

    int PreviousColumns = 0;
    for (int Rows = 1; Rows <= Count; Rows++)
    {
        const int Columns = (Count + Rows - 1) / Rows;
        if (Columns == PreviousColumns)
        {
            continue;
        }

        PreviousColumns = Columns;
        Shapes.push_back(GridShape{Columns, Rows, false});
        if (HasPartialRow(Columns, Count) == true)
        {
            Shapes.push_back(GridShape{Columns, Rows, true});
        }
    }

    return Shapes;
}

GridShape ThumbnailArranger::GetDefaultShape(const int Count)
{
    const std::vector<GridShape> Shapes = GetShapes(Count);
    if (Shapes.empty() == true)
    {
        return GridShape{};
    }

    GridShape Best = Shapes.front();
    for (const GridShape& Shape : Shapes)
    {
        const int Difference = std::abs(Shape.Columns - Shape.Rows);
        const int BestDifference = std::abs(Best.Columns - Best.Rows);
        if (Difference < BestDifference || (Difference == BestDifference && Shape.Columns > Best.Columns))
        {
            Best = Shape;
        }
    }

    return Best;
}

bool ThumbnailArranger::HasPartialRow(const int Columns, const int Count)
{
    return Columns > 1 && Count > 0 && Count % Columns != 0;
}

ThumbnailArranger::CellPosition ThumbnailArranger::GetCellPosition(const int Index, const GridShape& Shape, const int Count)
{
    const int Columns = std::max(1, Shape.Columns);
    if (HasPartialRow(Columns, Count) == false)
    {
        return CellPosition{Index / Columns, static_cast<float>(Index % Columns)};
    }

    const int PartialCount = Count % Columns;
    const float PartialStart = static_cast<float>(Columns - PartialCount) * 0.5f;
    if (Shape.PartialRowFirst == true)
    {
        if (Index < PartialCount)
        {
            return CellPosition{0, PartialStart + static_cast<float>(Index)};
        }

        const int FullIndex = Index - PartialCount;
        return CellPosition{1 + FullIndex / Columns, static_cast<float>(FullIndex % Columns)};
    }

    if (Index < Count - PartialCount)
    {
        return CellPosition{Index / Columns, static_cast<float>(Index % Columns)};
    }

    return CellPosition{(Count - 1) / Columns, PartialStart + static_cast<float>(Index - (Count - PartialCount))};
}

std::vector<Point> ThumbnailArranger::GetLocations(const ThumbnailArrangement& Arrangement, const Size ThumbnailSize, const size_t Count)
{
    std::vector<Point> Locations;
    const int CellWidth = ThumbnailSize.Width + Arrangement.Gap;
    const int CellHeight = ThumbnailSize.Height + Arrangement.Gap;

    for (size_t Index = 0; Index < Count; Index++)
    {
        const CellPosition Position = GetCellPosition(static_cast<int>(Index), Arrangement.Shape, static_cast<int>(Count));
        Locations.push_back(Point{
            Arrangement.Origin.X + static_cast<int>(Position.Column * static_cast<float>(CellWidth)),
            Arrangement.Origin.Y + Position.Row * CellHeight});
    }

    return Locations;
}

// Repeatedly gives the closest remaining thumbnail and slot to each other, so thumbnails stay near where they were
std::vector<size_t> ThumbnailArranger::AssignSlots(const std::vector<Point>& Current, const std::vector<Point>& Slots)
{
    const size_t Count = std::min(Current.size(), Slots.size());
    std::vector<size_t> SlotForThumbnail(Current.size());
    std::vector<bool> ThumbnailTaken(Current.size(), false);
    std::vector<bool> SlotTaken(Slots.size(), false);

    for (size_t Round = 0; Round < Count; Round++)
    {
        long long BestDistance = -1;
        size_t BestThumbnail = 0;
        size_t BestSlot = 0;

        for (size_t Thumbnail = 0; Thumbnail < Current.size(); Thumbnail++)
        {
            if (ThumbnailTaken[Thumbnail] == true)
            {
                continue;
            }

            for (size_t Slot = 0; Slot < Slots.size(); Slot++)
            {
                if (SlotTaken[Slot] == true)
                {
                    continue;
                }

                const long long DeltaX = Current[Thumbnail].X - Slots[Slot].X;
                const long long DeltaY = Current[Thumbnail].Y - Slots[Slot].Y;
                const long long Distance = DeltaX * DeltaX + DeltaY * DeltaY;
                if (BestDistance >= 0 && Distance >= BestDistance)
                {
                    continue;
                }

                BestDistance = Distance;
                BestThumbnail = Thumbnail;
                BestSlot = Slot;
            }
        }

        ThumbnailTaken[BestThumbnail] = true;
        SlotTaken[BestSlot] = true;
        SlotForThumbnail[BestThumbnail] = BestSlot;
    }

    return SlotForThumbnail;
}
