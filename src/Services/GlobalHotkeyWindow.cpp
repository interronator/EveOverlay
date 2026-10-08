#include "Services/GlobalHotkeyWindow.h"

#include <utility>

GlobalHotkeyWindow::~GlobalHotkeyWindow()
{
    Clear();
    if (m_hWnd != nullptr)
    {
        DestroyWindow();
    }
}

bool GlobalHotkeyWindow::Add(const Hotkey& Key, std::function<void()> Action)
{
    if (Key.IsNone() == true)
    {
        return false;
    }

    if (m_hWnd == nullptr)
    {
        Create(HWND_MESSAGE, CWindow::rcDefault);
    }

    std::unique_ptr<HotkeyHandler> Handler = std::make_unique<HotkeyHandler>(m_hWnd, Key);
    if (Handler->Register() == false)
    {
        return false;
    }

    Bindings.push_back(Binding{std::move(Handler), std::move(Action)});
    return true;
}

void GlobalHotkeyWindow::Clear()
{
    Bindings.clear();
}

size_t GlobalHotkeyWindow::GetCount() const
{
    return Bindings.size();
}

LRESULT GlobalHotkeyWindow::OnHotkey(UINT, WPARAM HotkeyId, LPARAM, BOOL& Handled)
{
    for (const Binding& Current : Bindings)
    {
        if (static_cast<WPARAM>(Current.Handler->GetId()) != HotkeyId)
        {
            continue;
        }

        if (Current.Action != nullptr)
        {
            Current.Action();
        }

        return 0;
    }

    Handled = FALSE;
    return 0;
}
