#pragma once

#include "UI/MainFrame.h"

#include "Application/CompositionRoot.h"
#include "Application/Logger.h"

class ApplicationRunner
{
public:
    static int Run(const HINSTANCE Instance, const int ShowCommand)
    {
        CompositionRoot Root;
        Root.Initialize();

        _Module.Init(nullptr, Instance);

        int ExitCode = 0;
        {
            CMessageLoop MessageLoop;
            _Module.AddMessageLoop(&MessageLoop);

            MainFrame Frame(Root.GetConfiguration(), Root.GetStorage());
            if (Frame.Create(nullptr, CWindow::rcDefault, L"Eve Overlay") == nullptr)
            {
                Logger::Error("The main window could not be created, Windows error " + std::to_string(::GetLastError()));
                ExitCode = 1;
            }
            else
            {
                Root.ConnectPresenter(Frame.GetPresenter());
                Root.StartServices();

                Frame.ShowInitially(ShowCommand);
                ExitCode = MessageLoop.Run();

                // The thumbnail windows must go before the frame and the ATL module do
                Root.Shutdown();
            }

            _Module.RemoveMessageLoop();
        }

        _Module.Term();
        return ExitCode;
    }
};
