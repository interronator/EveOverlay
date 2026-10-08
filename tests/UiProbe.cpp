#include <cstdio>
#include <string>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>

CAppModule _Module;

#include <atlwin.h>

#include "Config/ThumbnailArrangement.h"
#include "UI/ClientsTab.h"
#include "UI/ResourceIds.h"
#include "UI/TrayIcon.h"

class Checker
{
public:
    void Expect(const bool Condition, const char* const Description)
    {
        if (Condition == true)
        {
            return;
        }

        FailureCount++;
        std::printf("FAIL: %s\n", Description);
    }

    int FailureCount = 0;
};

void TestTrayIcon(Checker& Check, const HWND Window)
{
    TrayIcon Tray;
    const HICON Icon = ::LoadIconW(nullptr, IDI_APPLICATION);

    Check.Expect(Tray.Add(Window, WM_APP + 1, Icon, L"Eve Overlay probe") == true, "tray icon added");
    Check.Expect(Tray.IsAdded() == true, "IsAdded after Add");

    Tray.Remove();
    Check.Expect(Tray.IsAdded() == false, "IsAdded after Remove");
}

void TestClientsTab(Checker& Check)
{
    ClientsTab Tab;

    int EventCount = 0;
    std::wstring LastTitle;
    bool LastDisabled = false;
    Tab.ThumbnailStateChanged.Connect([&EventCount, &LastTitle, &LastDisabled](const std::wstring& Title, const bool IsDisabled)
    {
        EventCount++;
        LastTitle = Title;
        LastDisabled = IsDisabled;
    });

    Tab.AddThumbnail(L"EVE - Alpha", false);
    Tab.AddThumbnail(L"EVE - Beta", true);
    Check.Expect(Tab.GetThumbnailCount() == 2, "two thumbnails listed");
    Check.Expect(EventCount == 0, "adding thumbnails raises no state events");
    Check.Expect(Tab.IsThumbnailDisabled(0) == false, "Alpha starts enabled");
    Check.Expect(Tab.IsThumbnailDisabled(1) == true, "Beta starts force-hidden");

    Tab.SetThumbnailDisabled(0, true);
    Check.Expect(EventCount == 1 && LastTitle == L"EVE - Alpha" && LastDisabled == true, "hiding Alpha raises (Alpha, disabled)");

    Tab.SetThumbnailDisabled(1, false);
    Check.Expect(EventCount == 2 && LastTitle == L"EVE - Beta" && LastDisabled == false, "showing Beta raises (Beta, enabled)");

    Tab.SetThumbnailDisabled(1, false);
    Check.Expect(EventCount == 2, "setting an unchanged state raises no event");

    Tab.RemoveThumbnail(L"EVE - Alpha");
    Check.Expect(Tab.GetThumbnailCount() == 1, "Alpha removed");
    Check.Expect(EventCount == 2, "removing a thumbnail raises no state event");

    Tab.RemoveThumbnail(L"EVE - Missing");
    Check.Expect(Tab.GetThumbnailCount() == 1, "removing an unknown title is a no-op");
}

void TestArranger(Checker& Check)
{
    const std::vector<GridShape> Nine = ThumbnailArranger::GetShapes(9);
    Check.Expect(Nine.size() == 7, "nine clients offer five grids, two of them with a top variant");
    Check.Expect(Nine.front() == GridShape{9, 1} && Nine.back() == GridShape{1, 9}, "nine clients run from one row to one column");
    Check.Expect(ThumbnailArranger::GetDefaultShape(9) == GridShape{3, 3}, "nine clients default to 3x3");
    Check.Expect(ThumbnailArranger::GetDefaultShape(6) == GridShape{3, 2}, "six clients default to the wider 3x2");
    Check.Expect(ThumbnailArranger::GetShapes(0).empty() == true, "no clients offer no grids");

    const ThumbnailArrangement Arrangement{GridShape{3, 3}, 10, Point{100, 50}};
    const std::vector<Point> Locations = ThumbnailArranger::GetLocations(Arrangement, Size{384, 216}, 5);
    Check.Expect(Locations.size() == 5, "one location per thumbnail");
    Check.Expect(Locations[0].X == 100 && Locations[0].Y == 50, "first thumbnail sits at the origin");
    Check.Expect(Locations[2].X == 100 + 2 * 394 && Locations[2].Y == 50, "third thumbnail ends the first row");
    Check.Expect(Locations[3].X == 100 + 197 && Locations[3].Y == 50 + 226, "short last row is centred under the full row");

    const ThumbnailArrangement Centered{GridShape{2, 2}, 0, Point{0, 0}};
    const std::vector<Point> CenteredLocations = ThumbnailArranger::GetLocations(Centered, Size{100, 50}, 3);
    Check.Expect(CenteredLocations[2].X == 50 && CenteredLocations[2].Y == 50, "odd thumbnail is centred between the two above");
    Check.Expect(ThumbnailArranger::GetLocations(Centered, Size{100, 50}, 4)[3].X == 100, "a full last row is not shifted");

    const std::vector<size_t> Assigned = ThumbnailArranger::AssignSlots({Point{500, 0}, Point{0, 0}, Point{250, 0}}, {Point{0, 0}, Point{250, 0}, Point{500, 0}});
    Check.Expect(Assigned[0] == 2 && Assigned[1] == 0 && Assigned[2] == 1, "thumbnails take the slot nearest to where they were");

    const ThumbnailArrangement OnTop{GridShape{2, 2, true}, 0, Point{0, 0}};
    const std::vector<Point> TopLocations = ThumbnailArranger::GetLocations(OnTop, Size{100, 50}, 3);
    Check.Expect(TopLocations[0].X == 50 && TopLocations[0].Y == 0, "top variant puts the odd thumbnail centred on the first row");
    Check.Expect(TopLocations[1].X == 0 && TopLocations[1].Y == 50 && TopLocations[2].X == 100 && TopLocations[2].Y == 50, "top variant fills the rows below");
}

int wmain()
{
    Checker Check;

    _Module.Init(nullptr, ::GetModuleHandleW(nullptr));

    const HWND Window = ::CreateWindowExW(0, L"STATIC", L"probe", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, ::GetModuleHandleW(nullptr), nullptr);
    Check.Expect(Window != nullptr, "probe window created");
    if (Window != nullptr)
    {
        TestTrayIcon(Check, Window);
        ::DestroyWindow(Window);
    }

    TestClientsTab(Check);
    TestArranger(Check);

    _Module.Term();
    std::printf("%s (%d failed)\n", Check.FailureCount == 0 ? "ALL PASSED" : "FAILURES", Check.FailureCount);
    return Check.FailureCount;
}
