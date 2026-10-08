#include "Application/CompositionRoot.h"

#include <string>
#include <vector>

#include "Application/AppPaths.h"

CompositionRoot::CompositionRoot()
    : Storage(Configuration, AppPaths::GetConfigurationFilePath())
    , ViewFactory(WindowManagerInstance, Configuration)
    , Manager(Configuration, Storage, ProcessMonitorInstance, WindowManagerInstance, ViewFactory)
{
}

void CompositionRoot::Initialize()
{
    Storage.Load();
}

void CompositionRoot::ConnectPresenter(SettingsPresenter& Presenter)
{
    Manager.ThumbnailListUpdated.Connect([&Presenter](const std::vector<std::wstring>& Added, const std::vector<std::wstring>& Removed)
    {
        Presenter.RemoveThumbnails(Removed);
        Presenter.AddThumbnails(Added);
    });

    Manager.ThumbnailActiveSizeUpdated.Connect([&Presenter](const Size NewSize)
    {
        Presenter.UpdateThumbnailSize(NewSize);
    });

    Presenter.ThumbnailSizeChanged.Connect([this]()
    {
        Manager.UpdateThumbnailsSize();
    });

    Presenter.ArrangeRequested.Connect([this](const ThumbnailArrangement& Arrangement)
    {
        Manager.ArrangeThumbnails(Arrangement);
    });

    Presenter.HotkeysChanged.Connect([this]()
    {
        Manager.UpdateHotkeys();
    });

    Presenter.FrameSettingsChanged.Connect([this]()
    {
        Manager.UpdateThumbnailFrames();
    });
}

void CompositionRoot::StartServices()
{
    Manager.Start();
}

void CompositionRoot::Shutdown()
{
    Manager.Stop();
    Manager.CloseAllViews();
    Storage.Save();
}

ThumbnailConfiguration& CompositionRoot::GetConfiguration()
{
    return Configuration;
}

ConfigurationStorage& CompositionRoot::GetStorage()
{
    return Storage;
}

IWindowManager& CompositionRoot::GetWindowManager()
{
    return WindowManagerInstance;
}

IProcessMonitor& CompositionRoot::GetProcessMonitor()
{
    return ProcessMonitorInstance;
}

IThumbnailManager& CompositionRoot::GetThumbnailManager()
{
    return Manager;
}
