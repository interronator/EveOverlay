#pragma once

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
};
