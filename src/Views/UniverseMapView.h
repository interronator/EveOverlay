#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <Windows.h>
#include <objidl.h>

namespace Gdiplus
{
using std::max;
using std::min;
}

#include <gdiplus.h>

#include "Universe/UniverseLayout.h"

// Draws the neighbourhood onto a transparent surface: only the nodes, gate lines and system names are painted
class UniverseMapView
{
public:
    // Cycles is the position in the flash cycle in whole and fractional cycles, negative when the node is not alerted. Remaining
    // is the share of the alert time left, from 1 down to 0, and drives how red the node still is. The caller advances both,
    // which lets the flashing slow down and the colour fade without the view tracking any clock.
    struct NodeAlert
    {
        float Cycles = -1.0f;
        float Remaining = 0.0f;

        bool IsActive() const;
        bool operator==(const NodeAlert& Other) const = default;
    };

    void SetMap(const UniverseData* const Data, UniverseNeighborhood NewNeighborhood);

    // Smart mode shows only the centre system plus the route to every highlighted system. Positions still come from the
    // full neighbourhood, so nodes do not move as routes appear and disappear.
    bool SetSmartMode(const bool Enabled);

    // Turns the whole map clockwise around the base system; labels stay upright
    void SetRotation(const float Degrees);

    // One entry per node. Returns true when anything changed.
    bool SetHighlights(std::vector<NodeAlert> NewHighlighted);

    void Paint(Gdiplus::Graphics& Canvas, const float Width, const float Height, const float Scale);

private:
    static constexpr float PI = 3.14159265358979f;
    static constexpr float NODE_RADIUS = 5.0f;
    static constexpr float CENTER_RADIUS = 10.0f;
    static constexpr float ALERT_MIN_RADIUS = 3.0f;
    static constexpr float ALERT_MAX_RADIUS = 11.0f;
    static constexpr float MARGIN = 60.0f;
    static constexpr double HIGH_SEC_THRESHOLD = 0.45;

    static const Gdiplus::Color ALERT_COLOR;
    static const Gdiplus::Color CENTER_COLOR;
    static const Gdiplus::Color LABEL_COLOR;

    static Gdiplus::Color SecurityColor(const double Security);

    static BYTE Lerp(const BYTE Start, const BYTE End, const float Amount);
    static Gdiplus::Color Mix(const Gdiplus::Color& From, const Gdiplus::Color& To, const float Amount);

    // Nodes are in breadth-first order, so following parents from a highlighted node always reaches the centre
    std::vector<char> BuildVisibleNodes() const;

    const UniverseData* Universe = nullptr;
    UniverseNeighborhood Neighborhood;
    std::vector<LayoutPoint> Points;
    std::vector<std::wstring> Labels;
    std::vector<NodeAlert> Highlighted;
    bool SmartMode = false;
    float RotationRadians = 0.0f;
};
