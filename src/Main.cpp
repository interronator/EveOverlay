#include "Application/ApplicationRunner.h"

#include <exception>
#include <string>

#include "Application/ExceptionHandler.h"
#include "Application/Logger.h"
#include "Application/SingleInstanceGuard.h"

CAppModule _Module;

int WINAPI wWinMain(const HINSTANCE Instance, HINSTANCE, LPWSTR, const int ShowCommand)
{
    const SingleInstanceGuard Guard;
    if (Guard.IsFirstInstance() == false)
    {
        return 0;
    }

    ExceptionHandler::Install();
    Logger::PruneOldLogs();

    const HRESULT InitResult = ::CoInitialize(nullptr);
    if (FAILED(InitResult) == true)
    {
        Logger::Error("CoInitialize failed, HRESULT " + std::to_string(static_cast<long>(InitResult)));
        return 1;
    }

    int ExitCode = 0;
    try
    {
        ExitCode = ApplicationRunner::Run(Instance, ShowCommand);
    }
    catch (const std::exception& Failure)
    {
        ExceptionHandler::Report(Failure.what());
        ExitCode = 1;
    }
    catch (...)
    {
        ExceptionHandler::Report("Unknown exception");
        ExitCode = 1;
    }

    ::CoUninitialize();
    return ExitCode;
}
