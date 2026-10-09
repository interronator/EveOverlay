#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Config/Geometry.h"

struct GridShape
{
    int Columns = 1;
    int Rows = 1;
    bool PartialRowFirst = false;

    bool operator==(const GridShape& Other) const;
};

struct ThumbnailArrangement
{
    GridShape Shape;
    int Gap = 0;
    Point Origin;

    // Which character goes in each slot, in reading order; an empty name leaves the slot to whoever is closest
    std::vector<std::string> SlotCharacters;

    // Starts the grid where most of the previews already sit instead of at Origin
    bool SmartStart = false;
};

struct LayoutPreset
{
    std::string Name;
    int ClientCount = 1;
    ThumbnailArrangement Arrangement;
};

class ThumbnailArranger
{
public:
    struct CellPosition
    {
        int Row = 0;
        float Column = 0.0f;
    };

    // Every grid that holds the count without a spare row or column, ordered from a single row to a single column.
    // A grid with an incomplete row is offered twice: with that row below the full rows and with it above them.
    static std::vector<GridShape> GetShapes(const int Count);

    // The grid closest to a square, preferring the wider one on a tie
    static GridShape GetDefaultShape(const int Count);

    static bool HasPartialRow(const int Columns, const int Count);

    // An incomplete row is centred over the full rows and sits either below them or above them; Column may be a half cell
    static CellPosition GetCellPosition(const int Index, const GridShape& Shape, const int Count);

    static std::vector<Point> GetLocations(const ThumbnailArrangement& Arrangement, const Size ThumbnailSize, const size_t Count);

    // For each current thumbnail location, the index of the slot it should take; slots and thumbnails are paired closest first
    static std::vector<size_t> AssignSlots(const std::vector<Point>& Current, const std::vector<Point>& Slots);

    // Like the above, but a thumbnail whose character is named for a slot takes that slot first; Characters has one name per thumbnail.
    // A slot named for a character that has no thumbnail is left to the others.
    static std::vector<size_t> AssignSlots(const std::vector<Point>& Current, const std::vector<Point>& Slots, const std::vector<std::string>& Characters, const std::vector<std::string>& SlotCharacters);

    // The thumbnails that sit close together and make up more than half of all of them; empty when no group is a majority
    static std::vector<size_t> FindMajorityGroup(const std::vector<Point>& Locations, const Size ThumbnailSize, const int Gap);

    // Where a grid should start so it takes the place of the majority group: the group's top-left corner; nothing when there is no majority
    static std::optional<Point> DeriveOrigin(const std::vector<Point>& Locations, const Size ThumbnailSize, const int Gap);

    // Moves the origin so a grid of Count thumbnails fits inside the area, as far as it can
    static Point FitToArea(const Point Origin, const ThumbnailArrangement& Arrangement, const Size ThumbnailSize, const size_t Count, const ScreenBounds& Area);
};
