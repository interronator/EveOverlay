#include "Config/ThumbnailArrangement.h"

#include <algorithm>
#include <cstdlib>

#include "Config/TextUtil.h"

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

std::vector<size_t> ThumbnailArranger::AssignSlots(const std::vector<Point>& Current, const std::vector<Point>& Slots, const std::vector<std::string>& Characters, const std::vector<std::string>& SlotCharacters)
{
    std::vector<size_t> SlotForThumbnail(Current.size(), 0);
    std::vector<bool> ThumbnailTaken(Current.size(), false);
    std::vector<bool> SlotTaken(Slots.size(), false);

    const size_t NamedSlots = std::min(Slots.size(), SlotCharacters.size());
    for (size_t Slot = 0; Slot < NamedSlots; Slot++)
    {
        if (SlotCharacters[Slot].empty() == true)
        {
            continue;
        }

        for (size_t Thumbnail = 0; Thumbnail < Current.size() && Thumbnail < Characters.size(); Thumbnail++)
        {
            if (ThumbnailTaken[Thumbnail] == true || TextUtil::EqualsIgnoreCase(Characters[Thumbnail], SlotCharacters[Slot]) == false)
            {
                continue;
            }

            ThumbnailTaken[Thumbnail] = true;
            SlotTaken[Slot] = true;
            SlotForThumbnail[Thumbnail] = Slot;
            break;
        }
    }

    std::vector<size_t> RemainingThumbnails;
    std::vector<Point> RemainingLocations;
    for (size_t Thumbnail = 0; Thumbnail < Current.size(); Thumbnail++)
    {
        if (ThumbnailTaken[Thumbnail] == false)
        {
            RemainingThumbnails.push_back(Thumbnail);
            RemainingLocations.push_back(Current[Thumbnail]);
        }
    }

    std::vector<size_t> RemainingSlots;
    std::vector<Point> RemainingSlotLocations;
    for (size_t Slot = 0; Slot < Slots.size(); Slot++)
    {
        if (SlotTaken[Slot] == false)
        {
            RemainingSlots.push_back(Slot);
            RemainingSlotLocations.push_back(Slots[Slot]);
        }
    }

    const std::vector<size_t> Paired = AssignSlots(RemainingLocations, RemainingSlotLocations);
    for (size_t Index = 0; Index < RemainingThumbnails.size() && Index < Paired.size(); Index++)
    {
        SlotForThumbnail[RemainingThumbnails[Index]] = RemainingSlots[Paired[Index]];
    }

    return SlotForThumbnail;
}

// Thumbnails are linked when they are within one and a half thumbnails of each other, so neighbours in a grid, with or without a gap, form one group
std::vector<size_t> ThumbnailArranger::FindMajorityGroup(const std::vector<Point>& Locations, const Size ThumbnailSize, const int Gap)
{
    const int ReachX = ThumbnailSize.Width + ThumbnailSize.Width / 2 + std::max(Gap, 0);
    const int ReachY = ThumbnailSize.Height + ThumbnailSize.Height / 2 + std::max(Gap, 0);

    std::vector<bool> Assigned(Locations.size(), false);
    std::vector<size_t> Best;
    for (size_t Start = 0; Start < Locations.size(); Start++)
    {
        if (Assigned[Start] == true)
        {
            continue;
        }

        std::vector<size_t> Members{Start};
        Assigned[Start] = true;
        for (size_t Head = 0; Head < Members.size(); Head++)
        {
            const Point Current = Locations[Members[Head]];
            for (size_t Other = 0; Other < Locations.size(); Other++)
            {
                if (Assigned[Other] == true || std::abs(Locations[Other].X - Current.X) > ReachX || std::abs(Locations[Other].Y - Current.Y) > ReachY)
                {
                    continue;
                }

                Assigned[Other] = true;
                Members.push_back(Other);
            }
        }

        if (Members.size() > Best.size())
        {
            Best = Members;
        }
    }

    if (Best.size() * 2 <= Locations.size())
    {
        return std::vector<size_t>();
    }

    std::sort(Best.begin(), Best.end());
    return Best;
}

std::optional<Point> ThumbnailArranger::DeriveCenter(const std::vector<Point>& Locations, const Size ThumbnailSize)
{
    if (Locations.empty() == true)
    {
        return std::nullopt;
    }

    int Left = Locations.front().X;
    int Top = Locations.front().Y;
    int Right = Left + ThumbnailSize.Width;
    int Bottom = Top + ThumbnailSize.Height;
    for (const Point& Location : Locations)
    {
        Left = std::min(Left, Location.X);
        Top = std::min(Top, Location.Y);
        Right = std::max(Right, Location.X + ThumbnailSize.Width);
        Bottom = std::max(Bottom, Location.Y + ThumbnailSize.Height);
    }

    return Point{(Left + Right) / 2, (Top + Bottom) / 2};
}

Point ThumbnailArranger::GetOriginForCenter(const Point Center, const ThumbnailArrangement& Arrangement, const Size ThumbnailSize, const size_t Count)
{
    ThumbnailArrangement Probe = Arrangement;
    Probe.Origin = Point{0, 0};

    const std::vector<Point> Locations = GetLocations(Probe, ThumbnailSize, Count);
    if (Locations.empty() == true)
    {
        return Center;
    }

    int Left = Locations.front().X;
    int Top = Locations.front().Y;
    int Right = Left + ThumbnailSize.Width;
    int Bottom = Top + ThumbnailSize.Height;
    for (const Point& Location : Locations)
    {
        Left = std::min(Left, Location.X);
        Top = std::min(Top, Location.Y);
        Right = std::max(Right, Location.X + ThumbnailSize.Width);
        Bottom = std::max(Bottom, Location.Y + ThumbnailSize.Height);
    }

    return Point{Center.X - (Left + Right) / 2, Center.Y - (Top + Bottom) / 2};
}

Point ThumbnailArranger::FitToArea(const Point Origin, const ThumbnailArrangement& Arrangement, const Size ThumbnailSize, const size_t Count, const ScreenBounds& Area)
{
    ThumbnailArrangement Probe = Arrangement;
    Probe.Origin = Origin;

    const std::vector<Point> Locations = GetLocations(Probe, ThumbnailSize, Count);
    if (Locations.empty() == true)
    {
        return Origin;
    }

    int Left = Locations.front().X;
    int Top = Locations.front().Y;
    int Right = Left + ThumbnailSize.Width;
    int Bottom = Top + ThumbnailSize.Height;
    for (const Point& Location : Locations)
    {
        Left = std::min(Left, Location.X);
        Top = std::min(Top, Location.Y);
        Right = std::max(Right, Location.X + ThumbnailSize.Width);
        Bottom = std::max(Bottom, Location.Y + ThumbnailSize.Height);
    }

    Point Fitted = Origin;
    if (Right > Area.Right)
    {
        Fitted.X -= Right - Area.Right;
        Left -= Right - Area.Right;
    }

    if (Left < Area.Left)
    {
        Fitted.X += Area.Left - Left;
    }

    if (Bottom > Area.Bottom)
    {
        Fitted.Y -= Bottom - Area.Bottom;
        Top -= Bottom - Area.Bottom;
    }

    if (Top < Area.Top)
    {
        Fitted.Y += Area.Top - Top;
    }

    return Fitted;
}
