#include "Views/UniverseMapView.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_set>
#include <utility>

#include "Config/TextUtil.h"

const Gdiplus::Color UniverseMapView::ALERT_COLOR = Gdiplus::Color(255, 255, 30, 30);
const Gdiplus::Color UniverseMapView::CLEAR_COLOR = Gdiplus::Color(255, 40, 230, 90);
const Gdiplus::Color UniverseMapView::CAUTION_COLOR = Gdiplus::Color(255, 255, 215, 30);
const Gdiplus::Color UniverseMapView::CENTER_COLOR = Gdiplus::Color(255, 0, 200, 255);
const Gdiplus::Color UniverseMapView::LABEL_COLOR = Gdiplus::Color(255, 235, 240, 255);

void UniverseMapView::SetMap(const UniverseData* const Data, UniverseNeighborhood NewNeighborhood)
{
    Universe = Data;
    Neighborhood = std::move(NewNeighborhood);
    Points.clear();
    Labels.clear();
    Links.assign(Neighborhood.Nodes.size(), NodeLinks());
    Highlighted.assign(Neighborhood.Nodes.size(), NodeAlert());

    if (Universe == nullptr)
    {
        return;
    }

    std::unordered_set<int> DrawnSystems;
    for (const NeighborhoodNode& Node : Neighborhood.Nodes)
    {
        DrawnSystems.insert(Node.SystemIndex);
    }

    for (size_t Index = 0; Index < Neighborhood.Nodes.size(); Index++)
    {
        const SolarSystem& System = Universe->Get(Neighborhood.Nodes[Index].SystemIndex);
        Links[Index].DeadEnd = System.Neighbours.size() == 1;
        for (const int Neighbour : System.Neighbours)
        {
            if (DrawnSystems.contains(Neighbour) == false)
            {
                Links[Index].OutsideLinks++;
            }
        }
    }
}

void UniverseMapView::SetRotation(const float Degrees)
{
    RotationRadians = Degrees * PI / 180.0f;
}

bool UniverseMapView::SetSmartMode(const bool Enabled)
{
    if (Enabled == SmartMode)
    {
        return false;
    }

    SmartMode = Enabled;
    return true;
}

bool UniverseMapView::NodeAlert::IsActive() const
{
    return Cycles >= 0.0f;
}

bool UniverseMapView::NodeAlert::IsClearing() const
{
    return ClearStrength > 0.0f;
}

bool UniverseMapView::SetHighlights(std::vector<NodeAlert> NewHighlighted)
{
    if (NewHighlighted == Highlighted)
    {
        return false;
    }

    Highlighted = std::move(NewHighlighted);
    return true;
}

void UniverseMapView::Paint(Gdiplus::Graphics& Canvas, const float Width, const float Height, const float Scale)
{
    if (Universe == nullptr || Neighborhood.Nodes.empty() == true)
    {
        return;
    }

    EnsureLayout();

    Canvas.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Canvas.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

    const std::vector<Gdiplus::PointF> Screen = ComputeScreenPoints(Width, Height, Scale);
    const std::vector<char> Visible = BuildVisibleNodes();

    const Gdiplus::Pen EdgePen(Gdiplus::Color(210, 130, 150, 190), 1.5f * Scale);
    if (SmartMode == true)
    {
        for (size_t Index = 1; Index < Visible.size(); Index++)
        {
            if (Visible[Index] == 0)
            {
                continue;
            }

            Canvas.DrawLine(&EdgePen, Screen[Index], Screen[static_cast<size_t>(Neighborhood.Nodes[Index].Parent)]);
        }
    }
    else
    {
        for (const NeighborhoodEdge& Edge : Neighborhood.Edges)
        {
            Canvas.DrawLine(&EdgePen, Screen[static_cast<size_t>(Edge.From)], Screen[static_cast<size_t>(Edge.To)]);
        }
    }

    const std::vector<float> AwayAngles = ComputeAwayAngles(Screen, Gdiplus::PointF(Width / 2.0f, Height / 2.0f));
    DrawConnectionMarkers(Canvas, Screen, Visible, AwayAngles, Scale);

    const Gdiplus::FontFamily Family(L"Segoe UI");
    const Gdiplus::Font LabelFont(&Family, 12.0f * Scale, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    const Gdiplus::Font AlertFont(&Family, 12.0f * Scale, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat LabelFormat;
    LabelFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
    LabelFormat.SetLineAlignment(Gdiplus::StringAlignmentNear);
    LabelFormat.SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap);

    const Gdiplus::SolidBrush ShadowBrush(Gdiplus::Color(200, 0, 0, 0));
    const Gdiplus::Pen OutlinePen(Gdiplus::Color(200, 0, 0, 0), 1.0f * Scale);
    const Gdiplus::Pen CenterPen(Gdiplus::Color(255, 255, 255, 255), 2.0f * Scale);

    for (size_t Index = 0; Index < Points.size(); Index++)
    {
        if (Visible[Index] == 0)
        {
            continue;
        }

        const SolarSystem& System = Universe->Get(Neighborhood.Nodes[Index].SystemIndex);
        const bool IsCenter = Index == 0;
        const NodeAlert Alert = Index < Highlighted.size() ? Highlighted[Index] : NodeAlert();
        const bool IsAlert = Alert.IsActive();
        const bool IsClear = IsAlert == false && Alert.IsClearing() == true;
        const bool HasEffect = IsAlert == true || IsClear == true;
        const Gdiplus::Color& EffectColor = IsClear == true ? (Alert.Caution == true ? CAUTION_COLOR : CLEAR_COLOR) : ALERT_COLOR;
        const float Strength = IsAlert == true ? Alert.Remaining : (IsClear == true ? Alert.ClearStrength : 0.0f);
        const float Cycles = IsAlert == true ? Alert.Cycles : Alert.ClearCycles;
        const float Pulse = HasEffect == true ? 0.5f - 0.5f * std::cos(Cycles * 2.0f * PI) : 0.0f;

        // The size swing, like the colour, calms down toward the resting look as the alert runs out
        const float RestingRadius = IsCenter == true ? CENTER_RADIUS : NODE_RADIUS;
        const float PulseRadius = ALERT_MIN_RADIUS + (ALERT_MAX_RADIUS - ALERT_MIN_RADIUS) * Pulse;
        const float NodeRadius = (RestingRadius + (PulseRadius - RestingRadius) * Strength) * Scale;
        const Gdiplus::PointF Position = Screen[Index];

        if (HasEffect == true)
        {
            const float HaloRadius = NodeRadius * 1.9f;
            const Gdiplus::SolidBrush HaloBrush(Gdiplus::Color(static_cast<BYTE>((40.0f + 90.0f * Pulse) * Strength), EffectColor.GetR(), EffectColor.GetG(), EffectColor.GetB()));
            Canvas.FillEllipse(&HaloBrush, Position.X - HaloRadius, Position.Y - HaloRadius, HaloRadius * 2.0f, HaloRadius * 2.0f);
        }

        const Gdiplus::Color BaseColor = IsCenter == true ? CENTER_COLOR : SecurityColor(System.Security);
        const Gdiplus::SolidBrush NodeBrush(Mix(BaseColor, EffectColor, Strength));
        Canvas.FillEllipse(&NodeBrush, Position.X - NodeRadius, Position.Y - NodeRadius, NodeRadius * 2.0f, NodeRadius * 2.0f);
        Canvas.DrawEllipse(IsCenter == true ? &CenterPen : &OutlinePen, Position.X - NodeRadius, Position.Y - NodeRadius, NodeRadius * 2.0f, NodeRadius * 2.0f);

        const std::wstring& Label = Labels[Index];
        // An alerted name moves only with the slow fade instead of bobbing with the circle
        const float LabelRadius = RestingRadius + (ALERT_MAX_RADIUS - RestingRadius) * Strength;
        const float LabelTop = Position.Y + (HasEffect == true ? LabelRadius * Scale : NodeRadius) + 1.0f * Scale + GetCapDrop(Index, AwayAngles[Index], Scale);
        const Gdiplus::RectF LabelRectangle(Position.X - 100.0f * Scale, LabelTop, 200.0f * Scale, 20.0f * Scale);
        const Gdiplus::RectF ShadowRectangle(LabelRectangle.X + 1.0f * Scale, LabelRectangle.Y + 1.0f * Scale, LabelRectangle.Width, LabelRectangle.Height);
        const Gdiplus::Font* const Font = HasEffect == true ? &AlertFont : &LabelFont;
        const Gdiplus::SolidBrush NameBrush(Mix(LABEL_COLOR, EffectColor, Strength));
        Canvas.DrawString(Label.c_str(), static_cast<INT>(Label.size()), Font, ShadowRectangle, &LabelFormat, &ShadowBrush);
        Canvas.DrawString(Label.c_str(), static_cast<INT>(Label.size()), Font, LabelRectangle, &LabelFormat, &NameBrush);

        if (IsAlert == false)
        {
            continue;
        }

        // The distance stays fully visible for most of the alert and only fades out over its last third
        const float DistanceVisibility = std::min(1.0f, Strength * 3.0f);
        const int Jumps = Neighborhood.Nodes[Index].Jumps;
        const std::wstring Distance = std::to_wstring(Jumps) + (Jumps == 1 ? L" jump" : L" jumps");
        const Gdiplus::RectF DistanceRectangle(LabelRectangle.X, LabelRectangle.Y + 15.0f * Scale, LabelRectangle.Width, LabelRectangle.Height);
        const Gdiplus::RectF DistanceShadow(DistanceRectangle.X + 1.0f * Scale, DistanceRectangle.Y + 1.0f * Scale, DistanceRectangle.Width, DistanceRectangle.Height);
        const Gdiplus::SolidBrush DistanceShadowBrush(Gdiplus::Color(static_cast<BYTE>(200.0f * DistanceVisibility), 0, 0, 0));
        const Gdiplus::SolidBrush DistanceBrush(Gdiplus::Color(static_cast<BYTE>(255.0f * DistanceVisibility), 255, 30, 30));
        Canvas.DrawString(Distance.c_str(), static_cast<INT>(Distance.size()), &AlertFont, DistanceShadow, &LabelFormat, &DistanceShadowBrush);
        Canvas.DrawString(Distance.c_str(), static_cast<INT>(Distance.size()), &AlertFont, DistanceRectangle, &LabelFormat, &DistanceBrush);
    }
}

void UniverseMapView::EnsureLayout()
{
    if (Points.empty() == false)
    {
        return;
    }

    Points = UniverseLayout::Compute(Neighborhood);
    Labels.clear();
    for (const NeighborhoodNode& Node : Neighborhood.Nodes)
    {
        Labels.push_back(TextUtil::FromUtf8(Universe->Get(Node.SystemIndex).Name));
    }
}

std::vector<Gdiplus::PointF> UniverseMapView::ComputeScreenPoints(const float Width, const float Height, const float Scale) const
{
    const float CenterX = Width / 2.0f;
    const float CenterY = Height / 2.0f;
    const float Radius = std::max(10.0f, std::min(Width, Height) / 2.0f - MARGIN * Scale);
    const float RotationCosine = std::cos(RotationRadians);
    const float RotationSine = std::sin(RotationRadians);

    std::vector<Gdiplus::PointF> Screen(Points.size());
    for (size_t Index = 0; Index < Points.size(); Index++)
    {
        const float PointX = static_cast<float>(Points[Index].X);
        const float PointY = static_cast<float>(Points[Index].Y);
        Screen[Index] = Gdiplus::PointF(CenterX + (PointX * RotationCosine - PointY * RotationSine) * Radius, CenterY + (PointX * RotationSine + PointY * RotationCosine) * Radius);
    }

    return Screen;
}

std::vector<float> UniverseMapView::ComputeAwayAngles(const std::vector<Gdiplus::PointF>& Screen, const Gdiplus::PointF& MapCenter) const
{
    std::vector<Gdiplus::PointF> TowardKnown(Screen.size(), Gdiplus::PointF(0.0f, 0.0f));
    for (const NeighborhoodEdge& Edge : Neighborhood.Edges)
    {
        const Gdiplus::PointF& From = Screen[static_cast<size_t>(Edge.From)];
        const Gdiplus::PointF& To = Screen[static_cast<size_t>(Edge.To)];
        const float DeltaX = To.X - From.X;
        const float DeltaY = To.Y - From.Y;
        const float Length = std::hypot(DeltaX, DeltaY);
        if (Length < 0.001f)
        {
            continue;
        }

        TowardKnown[static_cast<size_t>(Edge.From)].X += DeltaX / Length;
        TowardKnown[static_cast<size_t>(Edge.From)].Y += DeltaY / Length;
        TowardKnown[static_cast<size_t>(Edge.To)].X -= DeltaX / Length;
        TowardKnown[static_cast<size_t>(Edge.To)].Y -= DeltaY / Length;
    }

    std::vector<float> Angles(Screen.size());
    for (size_t Index = 0; Index < Screen.size(); Index++)
    {
        if (std::hypot(TowardKnown[Index].X, TowardKnown[Index].Y) > 0.001f)
        {
            Angles[Index] = std::atan2(-TowardKnown[Index].Y, -TowardKnown[Index].X);
            continue;
        }

        const float FromCenterX = Screen[Index].X - MapCenter.X;
        const float FromCenterY = Screen[Index].Y - MapCenter.Y;
        Angles[Index] = std::hypot(FromCenterX, FromCenterY) > 0.001f ? std::atan2(FromCenterY, FromCenterX) : -PI / 2.0f;
    }

    return Angles;
}

void UniverseMapView::DrawConnectionMarkers(Gdiplus::Graphics& Canvas, const std::vector<Gdiplus::PointF>& Screen, const std::vector<char>& Visible, const std::vector<float>& AwayAngles, const float Scale) const
{
    const Gdiplus::Color StubStart(240, 150, 170, 210);
    const Gdiplus::Color StubEnd(0, 150, 170, 210);
    const Gdiplus::Pen DeadEndPen(Gdiplus::Color(235, 235, 240, 255), 2.0f * Scale);
    const float StubFan = STUB_FAN_DEGREES * PI / 180.0f;

    for (size_t Index = 0; Index < Screen.size(); Index++)
    {
        if (Visible[Index] == 0 || (Links[Index].OutsideLinks == 0 && Links[Index].DeadEnd == false))
        {
            continue;
        }

        const Gdiplus::PointF Position = Screen[Index];
        const float NodeRadius = (Index == 0 ? CENTER_RADIUS : NODE_RADIUS) * Scale;
        const float AwayAngle = AwayAngles[Index];

        // Gates leading outside the map fan out in the direction away from the rest of the map, fading as if they continue
        const int StubCount = std::min(Links[Index].OutsideLinks, MAX_STUBS);
        for (int Stub = 0; Stub < StubCount; Stub++)
        {
            const float Angle = AwayAngle + (static_cast<float>(Stub) - (static_cast<float>(StubCount) - 1.0f) / 2.0f) * StubFan;
            const Gdiplus::PointF Start(Position.X + std::cos(Angle) * (NodeRadius + 1.0f * Scale), Position.Y + std::sin(Angle) * (NodeRadius + 1.0f * Scale));
            const Gdiplus::PointF End(Position.X + std::cos(Angle) * (NodeRadius + STUB_LENGTH * Scale), Position.Y + std::sin(Angle) * (NodeRadius + STUB_LENGTH * Scale));
            const Gdiplus::LinearGradientBrush Fade(Start, End, StubStart, StubEnd);
            const Gdiplus::Pen StubPen(&Fade, 1.5f * Scale);
            Canvas.DrawLine(&StubPen, Start, End);
        }

        if (Links[Index].DeadEnd == false)
        {
            continue;
        }

        // The only gate is the line already drawn (or the fading stub), so the cap goes on the opposite side of the circle
        const float CapAngle = Links[Index].OutsideLinks > 0 ? AwayAngle + PI : AwayAngle;
        const float DirectionX = std::cos(CapAngle);
        const float DirectionY = std::sin(CapAngle);
        const Gdiplus::PointF CapBase(Position.X + DirectionX * (NodeRadius + 2.0f * Scale), Position.Y + DirectionY * (NodeRadius + 2.0f * Scale));
        const Gdiplus::PointF CapTip(Position.X + DirectionX * (NodeRadius + DEAD_END_LENGTH * Scale), Position.Y + DirectionY * (NodeRadius + DEAD_END_LENGTH * Scale));
        const float BarX = -DirectionY * DEAD_END_BAR_HALF_WIDTH * Scale;
        const float BarY = DirectionX * DEAD_END_BAR_HALF_WIDTH * Scale;
        Canvas.DrawLine(&DeadEndPen, CapBase, CapTip);
        Canvas.DrawLine(&DeadEndPen, Gdiplus::PointF(CapTip.X - BarX, CapTip.Y - BarY), Gdiplus::PointF(CapTip.X + BarX, CapTip.Y + BarY));
    }
}

Gdiplus::RectF UniverseMapView::GetMoveBounds(const float Width, const float Height, const float Scale)
{
    const Gdiplus::RectF WholeCanvas(0.0f, 0.0f, Width, Height);
    if (SmartMode == true || Universe == nullptr || Neighborhood.Nodes.empty() == true)
    {
        return WholeCanvas;
    }

    EnsureLayout();
    const std::vector<Gdiplus::PointF> Screen = ComputeScreenPoints(Width, Height, Scale);

    float Left = Width;
    float Top = Height;
    float Right = 0.0f;
    float Bottom = 0.0f;
    for (size_t Index = 0; Index < Screen.size(); Index++)
    {
        const float HalfLabel = static_cast<float>(Labels[Index].size()) * LABEL_CHARACTER_WIDTH * Scale / 2.0f;
        const float Reach = MARKER_REACH * Scale;
        Left = std::min(Left, Screen[Index].X - std::max(Reach, HalfLabel));
        Right = std::max(Right, Screen[Index].X + std::max(Reach, HalfLabel));
        Top = std::min(Top, Screen[Index].Y - Reach);
        Bottom = std::max(Bottom, Screen[Index].Y + std::max(Reach, LABEL_BLOCK_HEIGHT * Scale));
    }

    const float Padding = MOVE_PADDING * Scale;
    Left = std::max(0.0f, Left - Padding);
    Top = std::max(0.0f, Top - Padding);
    Right = std::min(Width, Right + Padding);
    Bottom = std::min(Height, Bottom + Padding);
    return Gdiplus::RectF(Left, Top, Right - Left, Bottom - Top);
}

Gdiplus::Color UniverseMapView::SecurityColor(const double Security)
{
    if (Security >= HIGH_SEC_THRESHOLD)
    {
        return Gdiplus::Color(255, 76, 175, 80);
    }

    if (Security > 0.0)
    {
        return Gdiplus::Color(255, 255, 152, 0);
    }

    return Gdiplus::Color(255, 171, 71, 188);
}

std::vector<char> UniverseMapView::BuildVisibleNodes() const
{
    std::vector<char> Visible(Neighborhood.Nodes.size(), SmartMode == true ? 0 : 1);
    if (SmartMode == false)
    {
        return Visible;
    }

    Visible[0] = 1;
    for (size_t Index = 1; Index < Visible.size() && Index < Highlighted.size(); Index++)
    {
        if (Highlighted[Index].IsActive() == false && Highlighted[Index].IsClearing() == false)
        {
            continue;
        }

        int Current = static_cast<int>(Index);
        while (Current > 0 && Visible[static_cast<size_t>(Current)] == 0)
        {
            Visible[static_cast<size_t>(Current)] = 1;
            Current = Neighborhood.Nodes[static_cast<size_t>(Current)].Parent;
        }
    }

    return Visible;
}

BYTE UniverseMapView::Lerp(const BYTE Start, const BYTE End, const float Amount)
{
    return static_cast<BYTE>(Start + (End - Start) * Amount);
}

Gdiplus::Color UniverseMapView::Mix(const Gdiplus::Color& From, const Gdiplus::Color& To, const float Amount)
{
    const float Clamped = std::clamp(Amount, 0.0f, 1.0f);
    return Gdiplus::Color(Lerp(From.GetA(), To.GetA(), Clamped), Lerp(From.GetR(), To.GetR(), Clamped), Lerp(From.GetG(), To.GetG(), Clamped), Lerp(From.GetB(), To.GetB(), Clamped));
}

float UniverseMapView::GetCapDrop(const size_t Index, const float AwayAngle, const float Scale) const
{
    if (Links[Index].DeadEnd == false)
    {
        return 0.0f;
    }

    const float CapAngle = Links[Index].OutsideLinks > 0 ? AwayAngle + PI : AwayAngle;
    const float Downward = std::sin(CapAngle);
    return Downward > 0.0f ? DEAD_END_LENGTH * Downward * Scale : 0.0f;
}
