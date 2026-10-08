// Stand-in for an EVE client: a plain window titled from the command line. Built as ExeFile.exe because that is
// the process name the app watches. The yellow block moves every 500 ms so live thumbnails visibly update.
#include <string>

#include <Windows.h>

class FakeClientWindow
{
public:
    static int Run(const HINSTANCE Instance, const std::wstring& CommandLine)
    {
        const std::wstring Title = BuildTitle(CommandLine);
        Background = ChooseBackground(Title);

        WNDCLASSW WindowClass = {};
        WindowClass.lpfnWndProc = &FakeClientWindow::WindowProc;
        WindowClass.hInstance = Instance;
        WindowClass.lpszClassName = L"FakeEve";
        WindowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        ::RegisterClassW(&WindowClass);

        const HWND Window = ::CreateWindowW(L"FakeEve", Title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, 200, 200, 640, 480, nullptr, nullptr, Instance, nullptr);
        ::SetTimer(Window, TIMER_ID, 500, nullptr);

        MSG Message = {};
        while (::GetMessageW(&Message, nullptr, 0, 0) > 0)
        {
            ::DispatchMessageW(&Message);
        }

        return 0;
    }

private:
    static constexpr UINT_PTR TIMER_ID = 1;

    static std::wstring BuildTitle(const std::wstring& CommandLine)
    {
        std::wstring Title = CommandLine.empty() == true ? L"EVE - Test Pilot" : CommandLine;
        while (Title.empty() == false && (Title.back() == L' ' || Title.back() == L'"'))
        {
            Title.pop_back();
        }

        return Title;
    }

    static COLORREF ChooseBackground(const std::wstring& Title)
    {
        if (Title.find(L"Beta") != std::wstring::npos)
        {
            return RGB(40, 120, 200);
        }

        if (Title.find(L"Gamma") != std::wstring::npos)
        {
            return RGB(40, 160, 70);
        }

        return RGB(200, 40, 40);
    }

    static void Paint(const HWND Window)
    {
        PAINTSTRUCT Paint = {};
        const HDC DeviceContext = ::BeginPaint(Window, &Paint);

        RECT Client = {};
        ::GetClientRect(Window, &Client);
        const HBRUSH BackgroundBrush = ::CreateSolidBrush(Background);
        ::FillRect(DeviceContext, &Client, BackgroundBrush);
        ::DeleteObject(BackgroundBrush);

        const RECT Marker = {20 + Phase * 120, 20, 120 + Phase * 120, 120};
        const HBRUSH MarkerBrush = ::CreateSolidBrush(RGB(255, 255, 0));
        ::FillRect(DeviceContext, &Marker, MarkerBrush);
        ::DeleteObject(MarkerBrush);

        wchar_t Text[256] = {};
        ::GetWindowTextW(Window, Text, 256);
        ::SetBkMode(DeviceContext, TRANSPARENT);
        ::SetTextColor(DeviceContext, RGB(255, 255, 255));
        ::TextOutW(DeviceContext, 20, 200, Text, static_cast<int>(::wcslen(Text)));

        ::EndPaint(Window, &Paint);
    }

    static LRESULT CALLBACK WindowProc(const HWND Window, const UINT Message, const WPARAM WParameter, const LPARAM LParameter)
    {
        if (Message == WM_DESTROY)
        {
            ::PostQuitMessage(0);
            return 0;
        }

        if (Message == WM_TIMER)
        {
            Phase = (Phase + 1) % 4;
            ::InvalidateRect(Window, nullptr, FALSE);
            return 0;
        }

        if (Message == WM_PAINT)
        {
            Paint(Window);
            return 0;
        }

        return ::DefWindowProcW(Window, Message, WParameter, LParameter);
    }

    static inline int Phase = 0;
    static inline COLORREF Background = RGB(200, 40, 40);
};

int WINAPI wWinMain(const HINSTANCE Instance, HINSTANCE, const LPWSTR CommandLine, int)
{
    return FakeClientWindow::Run(Instance, CommandLine == nullptr ? std::wstring() : std::wstring(CommandLine));
}
