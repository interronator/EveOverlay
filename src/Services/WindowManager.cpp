#include "Services/WindowManager.h"

#include "Services/Interop/DwmApi.h"
#include "Services/Interop/Gdi32Api.h"
#include "Services/Interop/User32Api.h"

WindowManager::WindowManager()
    : CompositionEnabled(DwmApi::IsCompositionEnabled())
{
}

bool WindowManager::IsCompositionEnabled() const
{
    return CompositionEnabled;
}

HWND WindowManager::GetForegroundWindowHandle() const
{
    return User32Api::GetForegroundWindowHandle();
}

void WindowManager::ActivateWindow(const HWND Window) const
{
    User32Api::SetForeground(Window);

    const LONG_PTR Style = User32Api::GetWindowStyle(Window);
    if ((Style & WS_MINIMIZE) != WS_MINIMIZE)
    {
        return;
    }

    User32Api::ShowAsync(Window, SW_RESTORE);
}

void WindowManager::MinimizeWindow(const HWND Window, const bool EnableAnimation) const
{
    if (EnableAnimation == true)
    {
        User32Api::SendMinimizeCommand(Window);
        return;
    }

    WINDOWPLACEMENT Placement = {};
    if (User32Api::GetPlacement(Window, &Placement) == false)
    {
        return;
    }

    // Used for clients that just lost the foreground; plain SW_MINIMIZE would activate the next window in z-order
    // and can steal the focus from the client that is being activated
    Placement.showCmd = SW_SHOWMINNOACTIVE;
    User32Api::SetPlacement(Window, Placement);
}

void WindowManager::MoveWindow(const HWND Window, const int Left, const int Top, const int Width, const int Height) const
{
    User32Api::Move(Window, Left, Top, Width, Height);
}

void WindowManager::MaximizeWindow(const HWND Window) const
{
    User32Api::ShowAsync(Window, SW_SHOWMAXIMIZED);
}

RECT WindowManager::GetWorkArea(const POINT Location) const
{
    return User32Api::GetWorkAreaNear(Location);
}

RECT WindowManager::GetWindowPosition(const HWND Window) const
{
    return User32Api::GetWindowRectangle(Window);
}

bool WindowManager::IsWindowMaximized(const HWND Window) const
{
    return User32Api::IsMaximized(Window);
}

bool WindowManager::IsWindowMinimized(const HWND Window) const
{
    return User32Api::IsMinimized(Window);
}

std::unique_ptr<IDwmThumbnail> WindowManager::GetLiveThumbnail(const HWND Destination, const HWND Source) const
{
    std::unique_ptr<IDwmThumbnail> Thumbnail = std::make_unique<DwmThumbnail>(*this);
    Thumbnail->Register(Destination, Source);
    return Thumbnail;
}

HBITMAP WindowManager::GetStaticThumbnail(const HWND Source) const
{
    const RECT ClientRectangle = User32Api::GetClientRectangle(Source);
    const int Width = ClientRectangle.right - ClientRectangle.left;
    const int Height = ClientRectangle.bottom - ClientRectangle.top;

    if (Width < WINDOW_SIZE_THRESHOLD || Height < WINDOW_SIZE_THRESHOLD)
    {
        return nullptr;
    }

    const HDC SourceContext = User32Api::AcquireDeviceContext(Source);
    if (SourceContext == nullptr)
    {
        return nullptr;
    }

    const HDC DestinationContext = Gdi32Api::CreateCompatibleContext(SourceContext);
    const HBITMAP Bitmap = Gdi32Api::CreateCompatibleBitmapHandle(SourceContext, Width, Height);

    const HGDIOBJ PreviousObject = Gdi32Api::SelectObjectHandle(DestinationContext, Bitmap);
    Gdi32Api::CopyBlock(DestinationContext, Width, Height, SourceContext);
    Gdi32Api::SelectObjectHandle(DestinationContext, PreviousObject);

    Gdi32Api::DeleteContext(DestinationContext);
    User32Api::ReleaseDeviceContext(Source, SourceContext);

    return Bitmap;
}
