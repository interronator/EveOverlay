#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "Services/CharacterNameResolver.h"
#include "Services/ProfileSyncer.h"
#include "UI/ITabPage.h"

class AccountSyncerTab : public ITabPage
{
public:
    const std::string& GetTitle() const override;
    const std::string& GetDescription() const override;
    void Draw() override;
    void LoadFromConfiguration(const ThumbnailConfiguration& Configuration) override;
    void StoreToConfiguration(ThumbnailConfiguration& Configuration) const override;

    void HandleDroppedFiles(const std::vector<std::filesystem::path>& Files);

private:
    static constexpr std::chrono::seconds PAIRING_WINDOW{2};
    static constexpr float POPUP_MIN_WIDTH = 400.0f;

    static std::filesystem::path GetEveRoot();
    static std::string BuildFolderLabel(const std::filesystem::path& Folder);
    static bool TryGetWriteTime(const std::filesystem::path& File, std::chrono::system_clock::time_point& WriteTime);
    static std::string FormatTime(const std::chrono::system_clock::time_point WriteTime);

    std::string GetAccountDisplayName(const long long Identifier) const;
    std::string GetCharacterDisplayName(const long long Identifier) const;

    // EVE writes a client's account and character files at the same moment, so a character file with the same time is probably the account's
    std::string FindLikelyCharacterName(const std::chrono::system_clock::time_point UserTime) const;

    // Characters show their name once looked up; accounts only have an id, so they get the likely character instead
    std::string BuildFileLabel(const std::filesystem::path& File) const;

    void CacheWriteTime(const std::filesystem::path& File);
    void Rescan();
    void AddFolderIfMissing(const std::filesystem::path& Folder);
    void AdoptFolder(const std::filesystem::path& Folder);
    void ClearPathsOutsideCurrentFolder();

    // Names the account of the selected user file; the name is only committed on Enter or when the field loses focus
    bool DrawNicknameRow();

    bool DrawFolderRow();
    bool DrawAccountRow();
    bool Browse();
    void RunSync();
    void RunUndo();
    void RefreshUndoPoint();
    bool IsClientRunning();
    void SetStatus(const std::string& Message, const bool IsError);

    const std::string Title = "Account Syncer";
    const std::string Description = "Make every account use the same UI settings as the one you choose.";

    std::string UserFilePath;
    ProfileSyncer::UndoPoint UndoPoint;
    std::string UndoTip;
    std::filesystem::path CurrentFolder;
    std::vector<std::filesystem::path> Folders;
    std::vector<std::filesystem::path> UserFiles;
    std::vector<std::filesystem::path> CharacterFiles;
    std::map<std::filesystem::path, std::chrono::system_clock::time_point> WriteTimes;
    CharacterNameResolver Resolver;
    std::map<long long, std::string> AccountNicknames;
    char NicknameBuffer[64] = {};
    long long NicknameFor = 0;
    std::string Status;
    bool StatusIsError = false;
};
