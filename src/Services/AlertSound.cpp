#include "Services/AlertSound.h"

#include <algorithm>
#include <system_error>

#include <mmsystem.h>

#include "Application/AppPaths.h"
#include "Application/Logger.h"

AlertSound::~AlertSound()
{
    Close();
}

std::filesystem::path AlertSound::GetDefaultSoundPath()
{
    const std::filesystem::path Bundled = AppPaths::GetExecutableDirectory() / L"sounds" / L"defaultWarning.wav";
    std::error_code Error;
    if (std::filesystem::exists(Bundled, Error) == true)
    {
        return Bundled;
    }

    wchar_t Directory[MAX_PATH] = {};
    if (::GetWindowsDirectoryW(Directory, MAX_PATH) == 0)
    {
        return std::filesystem::path();
    }

    return std::filesystem::path(Directory) / L"Media" / L"Alarm01.wav";
}

void AlertSound::Play(const std::wstring& Setting, const int VolumePercent)
{
    Close();

    std::filesystem::path Path = Setting;
    std::error_code Error;
    bool IsBundled = false;
    if (Path.empty() == false && Path.has_parent_path() == false)
    {
        Path = AppPaths::GetExecutableDirectory() / L"sounds" / Path;
        IsBundled = true;
    }

    if (Path.empty() == true || std::filesystem::exists(Path, Error) == false)
    {
        Path = GetDefaultSoundPath();
        IsBundled = true;
    }

    if (Path.empty() == true || std::filesystem::exists(Path, Error) == false)
    {
        ::MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    const std::wstring Open = L"open \"" + Path.wstring() + L"\" type mpegvideo alias " + ALIAS;
    const MCIERROR OpenResult = ::mciSendStringW(Open.c_str(), nullptr, 0, nullptr);
    if (OpenResult != 0)
    {
        Logger::Warning("Could not open the alert sound " + Logger::DescribePath(Path) + ", MCI error " + std::to_string(OpenResult));
        ::MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    IsOpen = true;
    const int Volume = std::clamp(VolumePercent, 0, 100) * 10;
    const std::wstring SetVolume = std::wstring(L"setaudio ") + ALIAS + L" volume to " + std::to_wstring(Volume);
    ::mciSendStringW(SetVolume.c_str(), nullptr, 0, nullptr);
    std::wstring Play = std::wstring(L"play ") + ALIAS + L" from 0";
    if (IsBundled == true)
    {
        Play += L" to " + std::to_wstring(BUNDLED_SOUND_LENGTH_MS);
    }

    ::mciSendStringW(Play.c_str(), nullptr, 0, nullptr);
}

void AlertSound::Close()
{
    if (IsOpen == false)
    {
        return;
    }

    ::mciSendStringW((std::wstring(L"close ") + ALIAS).c_str(), nullptr, 0, nullptr);
    IsOpen = false;
}

std::vector<std::filesystem::path> AlertSound::GetBundledSounds()
{
    std::vector<std::filesystem::path> Sounds;
    std::error_code Error;
    std::filesystem::directory_iterator Iterator(AppPaths::GetExecutableDirectory() / L"sounds", Error);
    if (Error.value() != 0)
    {
        return Sounds;
    }

    for (const std::filesystem::directory_entry& Entry : Iterator)
    {
        std::wstring Extension = Entry.path().extension().wstring();
        std::transform(Extension.begin(), Extension.end(), Extension.begin(), ::towlower);
        if (Extension == L".mp3" || Extension == L".wav" || Extension == L".wma")
        {
            Sounds.push_back(Entry.path());
        }
    }

    std::sort(Sounds.begin(), Sounds.end());
    return Sounds;
}
