#include "Services/TimerWindow.h"

#include <utility>

TimerWindow::~TimerWindow()
{
    Stop();
}

void TimerWindow::Start(const UINT IntervalMilliseconds, std::function<void()> TickCallback)
{
    Callback = std::move(TickCallback);

    if (m_hWnd == nullptr)
    {
        Create(HWND_MESSAGE, CWindow::rcDefault);
    }

    SetTimer(TIMER_ID, IntervalMilliseconds);
}

void TimerWindow::Stop()
{
    if (m_hWnd == nullptr)
    {
        return;
    }

    KillTimer(TIMER_ID);
    DestroyWindow();
}

LRESULT TimerWindow::OnTimer(UINT, WPARAM TimerId, LPARAM, BOOL& Handled)
{
    if (TimerId != TIMER_ID || Callback == nullptr)
    {
        Handled = FALSE;
        return 0;
    }

    Callback();
    return 0;
}
