#include "UI/AccountSyncerTab.h"

#include <cfloat>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <system_error>

#include <Windows.h>
#include <commdlg.h>

#include "Config/TextUtil.h"
#include "Services/ProcessMonitor.h"
#include "UI/Theme.h"
#include "UI/Widgets.h"

const std::string& AccountSyncerTab::GetTitle() const
{
    return Title;
}

const std::string& AccountSyncerTab::GetDescription() const
{
    return Description;
}

void AccountSyncerTab::Draw()
{
    bool Changed = false;

    Widgets::BeginCard("##AccountSyncer");
    Changed = DrawFolderRow() == true || Changed == true;
    Widgets::RowDivider();
    Changed = DrawAccountRow() == true || Changed == true;
    Widgets::RowDivider();
    Changed = DrawNicknameRow() == true || Changed == true;
    Widgets::RowDivider();
    Changed = DrawCharacterRow() == true || Changed == true;
    Widgets::EndCard();

    ImGui::BeginDisabled(UserFilePath.empty() == true);
    const bool SyncPressed = ImGui::Button("Sync accounts", ImVec2(Theme::Px(130.0f), 0.0f));
    Widgets::HoverTip("Copy the chosen account's UI settings to all your other accounts. The files being replaced are backed up first.");
    if (SyncPressed == true)
    {
        RunSync(UserFilePath);
    }

    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(CharacterFilePath.empty() == true);
    const bool SyncCharactersPressed = ImGui::Button("Sync characters", ImVec2(Theme::Px(140.0f), 0.0f));
    Widgets::HoverTip("Copy the chosen character's window layout and other per-character settings to all your other characters. The files being replaced are backed up first.");
    if (SyncCharactersPressed == true)
    {
        RunSync(CharacterFilePath);
    }

    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(UndoPoint.Found == false);
    const bool UndoPressed = ImGui::Button("Undo last sync", ImVec2(Theme::Px(130.0f), 0.0f));
    Widgets::HoverTip(UndoTip.c_str());
    if (UndoPressed == true)
    {
        RunUndo();
    }

    ImGui::EndDisabled();

    ImGui::SameLine();
    const bool RescanPressed = ImGui::Button("Rescan", ImVec2(Theme::Px(100.0f), 0.0f));
    Widgets::HoverTip("Look for settings files again and retry character name lookups.");
    if (RescanPressed == true)
    {
        Resolver.ForgetFailures();
        Rescan();
    }

    if (Resolver.ConsumeUpdated() == true)
    {
        Changed = true;
    }

    ImGui::Dummy(ImVec2(0.0f, Theme::Px(10.0f)));
    if (Resolver.IsRunning() == true)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
        ImGui::TextUnformatted("Looking up character names...");
        ImGui::PopStyleColor();
    }

    if (Status.empty() == false)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, StatusIsError == true ? Theme::ACCENT : Theme::TEXT);
        ImGui::TextWrapped("%s", Status.c_str());
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0.0f, Theme::Px(4.0f)));
    }

    ImGui::PushStyleColor(ImGuiCol_Text, Theme::TEXT_DISABLED);
    ImGui::TextWrapped("Choose the account whose settings every other account should copy and press Sync accounts. To make window layouts and other per-character settings match as well, choose a character and press Sync characters. The two syncs are separate, and each only replaces files of its own kind. Close all EVE clients first. Every sync keeps a backup in the OriginalFiles folder next to the settings, and Undo restores the latest one, even after restarting this app. Backups are never deleted automatically. The likely character shown next to an account is a guess from matching file times; a nickname makes an account easy to recognise. You can also drop an account file onto this window.");
    ImGui::PopStyleColor();

    if (Changed == true)
    {
        SettingsChanged.Emit();
    }
}

void AccountSyncerTab::LoadFromConfiguration(const ThumbnailConfiguration& Configuration)
{
    UserFilePath = Configuration.SyncerUserFilePath;
    CharacterFilePath = Configuration.SyncerCharacterFilePath;
    Resolver.Seed(Configuration.CharacterNames);
    AccountNicknames = Configuration.AccountNicknames;
    NicknameFor = 0;

    CurrentFolder = UserFilePath.empty() == true ? std::filesystem::path() : std::filesystem::path(TextUtil::FromUtf8(UserFilePath)).parent_path();
    Rescan();
}

void AccountSyncerTab::StoreToConfiguration(ThumbnailConfiguration& Configuration) const
{
    Configuration.SyncerUserFilePath = UserFilePath;
    Configuration.SyncerCharacterFilePath = CharacterFilePath;
    Configuration.CharacterNames = Resolver.GetAll();
    Configuration.AccountNicknames = AccountNicknames;
}

void AccountSyncerTab::HandleDroppedFiles(const std::vector<std::filesystem::path>& Files)
{
    for (const std::filesystem::path& File : Files)
    {
        const ProfileSyncer::FileKind Kind = ProfileSyncer::Classify(File);
        if (Kind == ProfileSyncer::FileKind::Unknown)
        {
            continue;
        }

        AdoptFolder(File.parent_path());
        if (Kind == ProfileSyncer::FileKind::User)
        {
            UserFilePath = TextUtil::ToUtf8(File.wstring());
            SetStatus("Account chosen. Press Sync accounts to copy its settings to the other accounts.", false);
        }
        else
        {
            CharacterFilePath = TextUtil::ToUtf8(File.wstring());
            SetStatus("Character chosen. Press Sync characters to copy its settings to the other characters.", false);
        }

        SettingsChanged.Emit();
        return;
    }

    SetStatus("None of the dropped files is a core_user_<number>.dat or core_char_<number>.dat file.", true);
}

std::filesystem::path AccountSyncerTab::GetEveRoot()
{
    wchar_t LocalAppData[MAX_PATH] = {};
    const DWORD Length = ::GetEnvironmentVariableW(L"LOCALAPPDATA", LocalAppData, MAX_PATH);
    if (Length == 0 || Length >= MAX_PATH)
    {
        return std::filesystem::path();
    }

    return std::filesystem::path(LocalAppData) / L"CCP" / L"EVE";
}

std::string AccountSyncerTab::BuildFolderLabel(const std::filesystem::path& Folder)
{
    return TextUtil::ToUtf8((Folder.parent_path().filename() / Folder.filename()).wstring());
}

bool AccountSyncerTab::TryGetWriteTime(const std::filesystem::path& File, std::chrono::system_clock::time_point& WriteTime)
{
    std::error_code ErrorCode;
    const std::filesystem::file_time_type FileTime = std::filesystem::last_write_time(File, ErrorCode);
    if (ErrorCode.value() != 0)
    {
        return false;
    }

    WriteTime = std::chrono::clock_cast<std::chrono::system_clock>(FileTime);
    return true;
}

std::string AccountSyncerTab::FormatTime(const std::chrono::system_clock::time_point WriteTime)
{
    const std::time_t Seconds = std::chrono::system_clock::to_time_t(WriteTime);
    std::tm LocalTime = {};
    ::localtime_s(&LocalTime, &Seconds);

    char Buffer[32] = {};
    std::strftime(Buffer, sizeof(Buffer), "%Y-%m-%d %H:%M", &LocalTime);
    return Buffer;
}

std::string AccountSyncerTab::GetAccountDisplayName(const long long Identifier) const
{
    const std::map<long long, std::string>::const_iterator Found = AccountNicknames.find(Identifier);
    return Found == AccountNicknames.end() ? std::to_string(Identifier) : Found->second + " (" + std::to_string(Identifier) + ")";
}

std::string AccountSyncerTab::GetCharacterDisplayName(const long long Identifier) const
{
    const std::string Name = Resolver.GetName(Identifier);
    return Name.empty() == true ? std::to_string(Identifier) : Name;
}

std::string AccountSyncerTab::FindLikelyCharacterName(const std::chrono::system_clock::time_point UserTime) const
{
    long long MatchIdentifier = 0;
    int MatchCount = 0;
    for (const std::filesystem::path& CharacterFile : CharacterFiles)
    {
        const std::map<std::filesystem::path, std::chrono::system_clock::time_point>::const_iterator Found = WriteTimes.find(CharacterFile);
        if (Found == WriteTimes.end())
        {
            continue;
        }

        const std::chrono::seconds Difference = std::chrono::duration_cast<std::chrono::seconds>(Found->second - UserTime);
        if (Difference > PAIRING_WINDOW || Difference < -PAIRING_WINDOW)
        {
            continue;
        }

        MatchIdentifier = ProfileSyncer::GetIdentifier(CharacterFile);
        MatchCount++;
    }

    return MatchCount == 1 ? GetCharacterDisplayName(MatchIdentifier) : std::string();
}

std::string AccountSyncerTab::BuildFileLabel(const std::filesystem::path& File) const
{
    const long long Identifier = ProfileSyncer::GetIdentifier(File);
    const bool IsCharacter = ProfileSyncer::Classify(File) == ProfileSyncer::FileKind::Character;
    std::string Label = IsCharacter == true ? GetCharacterDisplayName(Identifier) : GetAccountDisplayName(Identifier);

    const std::map<std::filesystem::path, std::chrono::system_clock::time_point>::const_iterator Found = WriteTimes.find(File);
    if (Found == WriteTimes.end())
    {
        return Label;
    }

    Label += "   " + FormatTime(Found->second);
    if (IsCharacter == true)
    {
        return Label;
    }

    const std::string Likely = FindLikelyCharacterName(Found->second);
    return Likely.empty() == true ? Label : Label + "   likely " + Likely;
}

void AccountSyncerTab::CacheWriteTime(const std::filesystem::path& File)
{
    std::chrono::system_clock::time_point WriteTime;
    if (TryGetWriteTime(File, WriteTime) == true)
    {
        WriteTimes[File] = WriteTime;
    }
}

void AccountSyncerTab::Rescan()
{
    Folders = ProfileSyncer::FindSettingsFolders(GetEveRoot());
    if (CurrentFolder.empty() == true && Folders.size() == 1)
    {
        CurrentFolder = Folders.front();
    }

    AddFolderIfMissing(CurrentFolder);
    UserFiles = ProfileSyncer::ListFiles(CurrentFolder, ProfileSyncer::FileKind::User);
    CharacterFiles = ProfileSyncer::ListFiles(CurrentFolder, ProfileSyncer::FileKind::Character);

    WriteTimes.clear();
    for (const std::filesystem::path& File : UserFiles)
    {
        CacheWriteTime(File);
    }

    std::vector<long long> CharacterIdentifiers;
    for (const std::filesystem::path& File : CharacterFiles)
    {
        CacheWriteTime(File);
        CharacterIdentifiers.push_back(ProfileSyncer::GetIdentifier(File));
    }

    Resolver.Request(CharacterIdentifiers);
    RefreshUndoPoint();
}

void AccountSyncerTab::AddFolderIfMissing(const std::filesystem::path& Folder)
{
    std::error_code ErrorCode;
    if (Folder.empty() == true || std::filesystem::is_directory(Folder, ErrorCode) == false)
    {
        return;
    }

    for (const std::filesystem::path& Known : Folders)
    {
        if (Known == Folder)
        {
            return;
        }
    }

    Folders.push_back(Folder);
}

void AccountSyncerTab::AdoptFolder(const std::filesystem::path& Folder)
{
    if (Folder == CurrentFolder)
    {
        return;
    }

    CurrentFolder = Folder;
    ClearPathsOutsideCurrentFolder();
    Rescan();
}

void AccountSyncerTab::ClearPathsOutsideCurrentFolder()
{
    if (UserFilePath.empty() == false && std::filesystem::path(TextUtil::FromUtf8(UserFilePath)).parent_path() != CurrentFolder)
    {
        UserFilePath.clear();
    }

    if (CharacterFilePath.empty() == false && std::filesystem::path(TextUtil::FromUtf8(CharacterFilePath)).parent_path() != CurrentFolder)
    {
        CharacterFilePath.clear();
    }
}

bool AccountSyncerTab::DrawNicknameRow()
{
    const long long Identifier = UserFilePath.empty() == true ? 0 : ProfileSyncer::GetIdentifier(std::filesystem::path(TextUtil::FromUtf8(UserFilePath)));
    if (Identifier != NicknameFor)
    {
        NicknameFor = Identifier;
        const std::map<long long, std::string>::const_iterator Found = AccountNicknames.find(Identifier);
        strcpy_s(NicknameBuffer, Found == AccountNicknames.end() ? "" : Found->second.c_str());
    }

    const float FieldWidth = Theme::Px(230.0f);
    Widgets::RowLabel("Account nickname", FieldWidth);
    ImGui::SetNextItemWidth(FieldWidth);
    ImGui::BeginDisabled(Identifier == 0);
    const bool Entered = ImGui::InputTextWithHint("##Nickname", "e.g. Main account", NicknameBuffer, sizeof(NicknameBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
    Widgets::HoverTip("Type a name and press Enter to save it.");
    const bool Committed = Entered == true || ImGui::IsItemDeactivatedAfterEdit() == true;
    ImGui::EndDisabled();
    Widgets::EndRow();

    if (Committed == false || Identifier == 0)
    {
        return false;
    }

    const std::string Nickname = TextUtil::Trim(NicknameBuffer);
    if (Nickname.empty() == true)
    {
        return AccountNicknames.erase(Identifier) > 0;
    }

    std::string& Stored = AccountNicknames[Identifier];
    if (Stored == Nickname)
    {
        return false;
    }

    Stored = Nickname;
    return true;
}

bool AccountSyncerTab::DrawFolderRow()
{
    const float FieldWidth = Theme::Px(300.0f);
    Widgets::RowLabel("Settings folder", FieldWidth);
    ImGui::SetNextItemWidth(FieldWidth);

    const std::string Preview = CurrentFolder.empty() == true ? "Select a folder..." : BuildFolderLabel(CurrentFolder);
    bool Changed = false;
    if (ImGui::BeginCombo("##Folder", Preview.c_str()) == true)
    {
        if (Folders.empty() == true)
        {
            ImGui::TextDisabled("No EVE settings folders found");
        }

        for (const std::filesystem::path& Folder : Folders)
        {
            if (Widgets::DropdownOption(BuildFolderLabel(Folder).c_str(), Folder == CurrentFolder) == false)
            {
                continue;
            }

            const std::filesystem::path Picked = Folder;
            AdoptFolder(Picked);
            Status.clear();
            Changed = true;
            break;
        }

        ImGui::EndCombo();
    }

    Widgets::EndRow();
    return Changed;
}

bool AccountSyncerTab::DrawAccountRow()
{
    const float ComboWidth = Theme::Px(230.0f);
    const float ButtonWidth = Theme::Px(76.0f);
    const float Gap = Theme::Px(6.0f);
    Widgets::RowLabel("Account to copy", ComboWidth + Gap + ButtonWidth);

    const float ControlY = ImGui::GetCursorPosY();
    ImGui::SetNextItemWidth(ComboWidth);
    ImGui::SetNextWindowSizeConstraints(ImVec2(Theme::Px(POPUP_MIN_WIDTH), 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const std::filesystem::path Current = UserFilePath.empty() == true ? std::filesystem::path() : std::filesystem::path(TextUtil::FromUtf8(UserFilePath));
    const std::string Preview = Current.empty() == true ? "Select an account..." : BuildFileLabel(Current);
    bool Changed = false;
    if (ImGui::BeginCombo("##Account", Preview.c_str()) == true)
    {
        if (UserFiles.empty() == true)
        {
            ImGui::TextDisabled("No account files found in this folder");
        }

        for (const std::filesystem::path& File : UserFiles)
        {
            ImGui::PushID(TextUtil::ToUtf8(File.filename().wstring()).c_str());
            if (Widgets::DropdownOption(BuildFileLabel(File).c_str(), File == Current) == true)
            {
                UserFilePath = TextUtil::ToUtf8(File.wstring());
                Status.clear();
                Changed = true;
            }

            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine(0.0f, Gap);
    ImGui::SetCursorPosY(ControlY);
    const bool BrowsePressed = ImGui::Button("Browse...", ImVec2(ButtonWidth, 0.0f));
    Widgets::HoverTip("Pick an account file that is not in the list.");
    if (BrowsePressed == true)
    {
        Changed = Browse() == true || Changed == true;
    }

    Widgets::EndRow();
    return Changed;
}

bool AccountSyncerTab::DrawCharacterRow()
{
    const float ComboWidth = Theme::Px(230.0f);
    Widgets::RowLabel("Character to copy", ComboWidth);
    ImGui::SetNextItemWidth(ComboWidth);
    ImGui::SetNextWindowSizeConstraints(ImVec2(Theme::Px(POPUP_MIN_WIDTH), 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const std::filesystem::path Current = CharacterFilePath.empty() == true ? std::filesystem::path() : std::filesystem::path(TextUtil::FromUtf8(CharacterFilePath));
    const std::string Preview = Current.empty() == true ? "Select a character..." : BuildFileLabel(Current);
    bool Changed = false;
    if (ImGui::BeginCombo("##Character", Preview.c_str()) == true)
    {
        if (CharacterFiles.empty() == true)
        {
            ImGui::TextDisabled("No character files found in this folder");
        }

        for (const std::filesystem::path& File : CharacterFiles)
        {
            ImGui::PushID(TextUtil::ToUtf8(File.filename().wstring()).c_str());
            if (Widgets::DropdownOption(BuildFileLabel(File).c_str(), File == Current) == true)
            {
                CharacterFilePath = TextUtil::ToUtf8(File.wstring());
                Status.clear();
                Changed = true;
            }

            ImGui::PopID();
        }

        ImGui::EndCombo();
    }

    Widgets::EndRow();
    return Changed;
}

bool AccountSyncerTab::Browse()
{
    const std::wstring InitialDirectory = CurrentFolder.empty() == true ? GetEveRoot().wstring() : CurrentFolder.wstring();
    wchar_t SelectedPath[MAX_PATH] = {};
    OPENFILENAMEW Dialog = {};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = ::GetActiveWindow();
    Dialog.lpstrFilter = L"EVE account settings (core_user_*.dat)\0core_user_*.dat\0";
    Dialog.lpstrFile = SelectedPath;
    Dialog.nMaxFile = MAX_PATH;
    Dialog.lpstrInitialDir = InitialDirectory.empty() == true ? nullptr : InitialDirectory.c_str();
    Dialog.lpstrTitle = L"Choose the account whose settings to copy";
    Dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (::GetOpenFileNameW(&Dialog) == FALSE)
    {
        return false;
    }

    if (ProfileSyncer::Classify(SelectedPath) != ProfileSyncer::FileKind::User)
    {
        SetStatus("That file is not a core_user_<number>.dat account file.", true);
        return false;
    }

    AdoptFolder(std::filesystem::path(SelectedPath).parent_path());
    UserFilePath = TextUtil::ToUtf8(SelectedPath);
    Status.clear();
    return true;
}

void AccountSyncerTab::RunSync(const std::string& MasterPath)
{
    if (IsClientRunning() == true)
    {
        return;
    }

    const ProfileSyncer::Result Outcome = ProfileSyncer::Sync(std::filesystem::path(TextUtil::FromUtf8(MasterPath)));
    SetStatus(Outcome.Message, Outcome.Succeeded == false);
    Rescan();
}

void AccountSyncerTab::RunUndo()
{
    if (IsClientRunning() == true)
    {
        return;
    }

    const ProfileSyncer::Result Outcome = ProfileSyncer::Undo(CurrentFolder);
    SetStatus(Outcome.Message, Outcome.Succeeded == false);
    Rescan();
}

void AccountSyncerTab::RefreshUndoPoint()
{
    UndoPoint = CurrentFolder.empty() == true ? ProfileSyncer::UndoPoint() : ProfileSyncer::FindUndoPoint(CurrentFolder);
    if (UndoPoint.Found == false)
    {
        UndoTip = "There is no sync to undo in this folder.";
        return;
    }

    UndoTip = "Restore the " + std::to_string(UndoPoint.FileCount) + " files replaced by the sync of " + ProfileSyncer::FormatStamp(UndoPoint.Stamp) + ".";
}

// EVE rewrites its settings when a client closes, which would silently undo a sync or an undo made while one is running
bool AccountSyncerTab::IsClientRunning()
{
    if (ProcessMonitor::EnumerateMonitoredProcessIds().empty() == true)
    {
        return false;
    }

    SetStatus("Close all EVE clients first; they rewrite these settings when they exit.", true);
    return true;
}

void AccountSyncerTab::SetStatus(const std::string& Message, const bool IsError)
{
    Status = Message;
    StatusIsError = IsError;
}
