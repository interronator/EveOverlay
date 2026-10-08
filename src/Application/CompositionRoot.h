#pragma once

#include "Config/ConfigurationStorage.h"
#include "Config/ThumbnailConfiguration.h"
#include "Presenters/SettingsPresenter.h"
#include "Services/IProcessMonitor.h"
#include "Services/IThumbnailManager.h"
#include "Services/IWindowManager.h"
#include "Services/ProcessMonitor.h"
#include "Services/ThumbnailManager.h"
#include "Services/WindowManager.h"
#include "Views/ThumbnailViewFactory.h"

// Owns the long-lived services in dependency order
class CompositionRoot
{
public:
    CompositionRoot();
    CompositionRoot(const CompositionRoot&) = delete;
    CompositionRoot& operator=(const CompositionRoot&) = delete;

    void Initialize();

    // Replaces the mediator messages of the C# app with direct signal connections between manager and presenter
    void ConnectPresenter(SettingsPresenter& Presenter);

    void StartServices();
    void Shutdown();

    ThumbnailConfiguration& GetConfiguration();
    ConfigurationStorage& GetStorage();
    IWindowManager& GetWindowManager();
    IProcessMonitor& GetProcessMonitor();
    IThumbnailManager& GetThumbnailManager();

private:
    ThumbnailConfiguration Configuration;
    ConfigurationStorage Storage;
    WindowManager WindowManagerInstance;
    ProcessMonitor ProcessMonitorInstance;
    ThumbnailViewFactory ViewFactory;
    ThumbnailManager Manager;
};
