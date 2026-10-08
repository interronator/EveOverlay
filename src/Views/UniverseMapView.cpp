#include "Views/UniverseMapView.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "Config/TextUtil.h"

const Gdiplus::Color UniverseMapView::ALERT_COLOR = Gdiplus::Color(255, 255, 30, 30);
const Gdiplus::Color UniverseMapView::CENTER_COLOR = Gdiplus::Color(255, 0, 200, 255);
const Gdiplus::Color UniverseMapView::LABEL_COLOR = Gdiplus::Color(255, 235, 240, 255);

void UniverseMapView::SetMap(const UniverseData* const Data, UniverseNeighborhood NewNeighborhood)
{
    Universe = Data;
    Neighborhood = std::move(NewNeighborhood);
    Points.clear();
    Labels.clear();
    Highlighted.assign(Neighborhood.Nodes.size(), NodeAlert());
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

    if (Points.empty() == true)
    {
        Points = UniverseLayout::Compute(Neighborhood);
        Labels.clear();
        for (const NeighborhoodNode& Node : Neighborhood.Nodes)
        {
            Labels.push_back(TextUtil::FromUtf8(Universe->Get(Node.SystemIndex).Name));
        }
    }

    Canvas.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    Canvas.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

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
        const float Strength = IsAlert == true ? Alert.Remaining : 0.0f;
        const float Pulse = IsAlert == true ? 0.5f - 0.5f * std::cos(Alert.Cycles * 2.0f * PI) : 0.0f;

        // The size swing, like the colour, calms down toward the resting look as the alert runs out
        const float RestingRadius = IsCenter == true ? CENTER_RADIUS : NODE_RADIUS;
        const float PulseRadius = ALERT_MIN_RADIUS + (ALERT_MAX_RADIUS - ALERT_MIN_RADIUS) * Pulse;
        const float NodeRadius = (RestingRadius + (PulseRadius - RestingRadius) * Strength) * Scale;
        const Gdiplus::PointF Position = Screen[Index];

        if (IsAlert == true)
        {
            const float HaloRadius = NodeRadius * 1.9f;
            const Gdiplus::SolidBrush HaloBrush(Gdiplus::Color(static_cast<BYTE>((40.0f + 90.0f * Pulse) * Strength), 255, 0, 0));
            Canvas.FillEllipse(&HaloBrush, Position.X - HaloRadius, Position.Y - HaloRadius, HaloRadius * 2.0f, HaloRadius * 2.0f);
        }

        const Gdiplus::Color BaseColor = IsCenter == true ? CENTER_COLOR : SecurityColor(System.Security);
        const Gdiplus::SolidBrush NodeBrush(Mix(BaseColor, ALERT_COLOR, Strength));
        Canvas.FillEllipse(&NodeBrush, Position.X - NodeRadius, Position.Y - NodeRadius, NodeRadius * 2.0f, NodeRadius * 2.0f);
        Canvas.DrawEllipse(IsCenter == true ? &CenterPen : &OutlinePen, Position.X - NodeRadius, Position.Y - NodeRadius, NodeRadius * 2.0f, NodeRadius * 2.0f);

        const std::wstring& Label = Labels[Index];
        // An alerted name moves only with the slow fade instead of bobbing with the circle
        const float LabelRadius = RestingRadius + (ALERT_MAX_RADIUS - RestingRadius) * Strength;
        const float LabelTop = Position.Y + (IsAlert == true ? LabelRadius * Scale : NodeRadius) + 1.0f * Scale;
        const Gdiplus::RectF LabelRectangle(Position.X - 100.0f * Scale, LabelTop, 200.0f * Scale, 20.0f * Scale);
        const Gdiplus::RectF ShadowRectangle(LabelRectangle.X + 1.0f * Scale, LabelRectangle.Y + 1.0f * Scale, LabelRectangle.Width, LabelRectangle.Height);
        const Gdiplus::Font* const Font = IsAlert == true ? &AlertFont : &LabelFont;
        const Gdiplus::SolidBrush NameBrush(Mix(LABEL_COLOR, ALERT_COLOR, Strength));
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
        if (Highlighted[Index].IsActive() == false)
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
