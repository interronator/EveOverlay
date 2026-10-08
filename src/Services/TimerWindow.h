#pragma once

#include <functional>

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

// Message-only window that turns WM_TIMER into a callback on the UI thread
class TimerWindow : public CWindowImpl<TimerWindow>
{
public:
    DECLARE_WND_CLASS_EX(L"EveOverlayTimerWindow", 0, 0)

    BEGIN_MSG_MAP(TimerWindow)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
    END_MSG_MAP()

    TimerWindow() = default;
    ~TimerWindow();

    void Start(const UINT IntervalMilliseconds, std::function<void()> TickCallback);
    void Stop();

private:
    static constexpr UINT_PTR TIMER_ID = 1;

    LRESULT OnTimer(UINT, WPARAM TimerId, LPARAM, BOOL& Handled);

    std::function<void()> Callback;
};
