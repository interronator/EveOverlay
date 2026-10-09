#include "Presenters/SettingsPresenter.h"

SettingsPresenter::SettingsPresenter(ThumbnailConfiguration& ConfigurationReference, ConfigurationStorage& StorageReference,
    const std::vector<ITabPage*>& TabPages, ThumbnailTab& ThumbnailTabReference, ClientsTab& ClientsTabReference, OrganizerTab& OrganizerTabReference, HotkeysTab& HotkeysTabReference)
    : Configuration(ConfigurationReference)
    , Storage(StorageReference)
    , Pages(TabPages)
    , ThumbnailPage(ThumbnailTabReference)
    , ClientsPage(ClientsTabReference)
    , OrganizerPage(OrganizerTabReference)
    , HotkeysPage(HotkeysTabReference)
{
    for (ITabPage* const Page : Pages)
    {
        Page->SettingsChanged.Connect([this]()
        {
            SaveSettings();
        });
    }

    ThumbnailPage.ThumbnailSizeChanged.Connect([this]()
    {
        OnThumbnailSizeChanged();
    });

    ClientsPage.ThumbnailStateChanged.Connect([this](const std::wstring& Title, const bool IsDisabled)
    {
        OnThumbnailStateChanged(Title, IsDisabled);
    });

    OrganizerPage.ArrangeRequested.Connect([this](const ThumbnailArrangement& Arrangement)
    {
        ArrangeRequested.Emit(Arrangement);
    });
}

void SettingsPresenter::LoadSettings()
{
    for (ITabPage* const Page : Pages)
    {
        Page->LoadFromConfiguration(Configuration);
    }
}

void SettingsPresenter::AddThumbnails(const std::vector<std::wstring>& Titles)
{
    bool CharacterAdded = false;
    for (const std::wstring& Title : Titles)
    {
        ClientsPage.AddThumbnail(Title, Configuration.IsThumbnailDisabled(Title));
        HotkeysPage.AddClient(Title);
        OrganizerPage.AddOpenClient(Title);
        CharacterAdded = Configuration.RememberCharacter(Title) == true || CharacterAdded == true;
    }

    if (CharacterAdded == true)
    {
        CharactersChanged.Emit(Configuration.GetSelectableCharacters());
        Storage.Save();
    }

    OpenClients.insert(Titles.begin(), Titles.end());
    NotifyClientsOpen();
    OrganizerPage.SetDetectedCount(ClientsPage.GetEnabledThumbnailCount());
}

void SettingsPresenter::RemoveThumbnails(const std::vector<std::wstring>& Titles)
{
    for (const std::wstring& Title : Titles)
    {
        ClientsPage.RemoveThumbnail(Title);
        HotkeysPage.RemoveClient(Title);
        OrganizerPage.RemoveOpenClient(Title);
    }

    for (const std::wstring& Title : Titles)
    {
        OpenClients.erase(Title);
    }

    NotifyClientsOpen();
    OrganizerPage.SetDetectedCount(ClientsPage.GetEnabledThumbnailCount());
}

bool SettingsPresenter::ForgetCharacter(const std::string& Name)
{
    const bool SelectionCleared = Configuration.ForgetCharacter(Name);
    Storage.Save();
    CharactersChanged.Emit(Configuration.GetSelectableCharacters());
    return SelectionCleared;
}

void SettingsPresenter::UpdateThumbnailSize(const Size NewSize)
{
    ThumbnailPage.SetThumbnailSize(NewSize);
    Configuration.ThumbnailSize = NewSize;
}

void SettingsPresenter::SaveSettings()
{
    const bool PreviousFrames = Configuration.ShowThumbnailFrames;
    const std::string PreviousHotkeys = Configuration.GetHotkeySignature();

    for (const ITabPage* const Page : Pages)
    {
        Page->StoreToConfiguration(Configuration);
    }

    Storage.Save();

    if (PreviousFrames != Configuration.ShowThumbnailFrames)
    {
        FrameSettingsChanged.Emit();
    }

    if (PreviousHotkeys != Configuration.GetHotkeySignature())
    {
        HotkeysChanged.Emit();
    }
}

void SettingsPresenter::NotifyClientsOpen()
{
    const bool IsOpen = OpenClients.empty() == false;
    if (IsOpen == ClientsOpen)
    {
        return;
    }

    ClientsOpen = IsOpen;
    ClientsOpenChanged.Emit(IsOpen);
}

void SettingsPresenter::OnThumbnailSizeChanged()
{
    SaveSettings();
    ThumbnailSizeChanged.Emit();
}

void SettingsPresenter::OnThumbnailStateChanged(const std::wstring& Title, const bool IsDisabled)
{
    Configuration.ToggleThumbnail(Title, IsDisabled);
    Storage.Save();
    OrganizerPage.SetDetectedCount(ClientsPage.GetEnabledThumbnailCount());
}
