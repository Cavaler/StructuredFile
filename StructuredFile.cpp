#define DEFINE_VARS
#include "StructuredFile.h"

#include <FarDlg.h>
#include <UTF8.h>
#include <Directory.h>

#include <initguid.h>
DEFINE_GUID(GUID_StructuredFile, 0xec26a161, 0xf889, 0x45e3, 0xa8, 0xa1, 0x9, 0x96, 0xe, 0xb4, 0x8a, 0x50);
DEFINE_GUID(GUID_StructuredFileConfig, 0x904560e, 0x424f, 0x49ca, 0xb6, 0x2, 0xdb, 0x46, 0xac, 0x78, 0x98, 0xf2);
DEFINE_GUID(GUID_StructuredFileRun, 0x251b81b4, 0xc0c5, 0x4422, 0xae, 0x73, 0xc, 0x5, 0xd2, 0xfb, 0xe2, 0x1d);

#include "version.h"

#include <algorithm>
#include <charconv>
#include <functional>
#include <format>
#include <regex>

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info) {
    Info->StructSize = sizeof(GlobalInfo);
    Info->MinFarVersion = FARMANAGERVERSION;
    Info->Version = MAKEFARVERSION(PLUGIN_VERSION_MAJOR, PLUGIN_VERSION_MINOR, PLUGIN_VERSION_REVISION, 0, VS_RELEASE);
    Info->Guid = GUID_StructuredFile;
    Info->Title = _T(PLUGIN_NAME);
    Info->Description = _T(PLUGIN_DESCRIPTION);
    Info->Author = _T(PLUGIN_AUTHOR);
}

void WINAPI FAR_EXPORT(GetPluginInfo)(PluginInfo *Info) {
    Info->StructSize = sizeof(PluginInfo);
    Info->Flags = 0;

    static const GUID ConfigGuids[] = {GUID_StructuredFileConfig};
    static const TCHAR *ConfigStrings[] = {GetMsg(MStructuredFile)};

    Info->PluginConfig.Count = 1;
    Info->PluginConfig.Guids = ConfigGuids;
    Info->PluginConfig.Strings = ConfigStrings;

    Info->PluginMenu.Count = 0;
    Info->DiskMenu.Count = 0;

    Info->CommandPrefix = NULL;
}

void WINAPI FAR_EXPORT(SetStartupInfo)(const PluginStartupInfo *Info) {
    StartupInfo = *Info;
    StartupInfo.m_GUID = GUID_StructuredFile;

    LoadSettings();

    g_pszOKButton = GetMsg(MOk);
    g_pszErrorTitle = GetMsg(MError);
    CFarIntegerRangeValidator::s_szErrorMsg = GetMsg(MInvalidNumber);
    CFarIntegerRangeValidator::s_szHelpTopic = _T("REInvalidNumber");
    CFarDialog::AutoHotkeys = true;
    CFarDialog::SetDefaultCancelID(MCancel);
}

void ShowError(const std::wstring &title, const std::wstring &error) {
    std::vector<std::wstring> items;
    items.push_back(title);

    size_t pos = 0;
    while (pos < error.size()) {
        auto nl = error.find('\n', pos);
        if (nl == wstring::npos)
            nl = error.size();

        if (nl - pos > 70) {
            auto sp = error.substr(pos, 70).rfind(' ');
            if (sp != wstring::npos)
                nl = pos + sp;
        }

        items.push_back(error.substr(pos, nl - pos));
        pos = nl + 1;
    }

    Message(FMSG_WARNING | FMSG_LEFTALIGN | FMSG_MB_OK, NULL, 0, items);
}

template <typename rettype, rettype on_error = (rettype)0>
rettype TryCall(std::function<rettype()> callback) {
    try {
        return callback();
    } catch (const exception &ex) {
        ShowError(GetMsg(MStructuredFile), DecodeUTF8(ex.what()));
        return on_error;
    }
}

void TryCall(std::function<void()> callback) {
    try {
        callback();
    } catch (const exception &ex) {
        ShowError(GetMsg(MStructuredFile), DecodeUTF8(ex.what()));
    }
}

void ShowParseError(const struct AnalyseInfo *Info, const std::wstring &error) {
    if (!(Info->OpMode & OPM_SILENT) && !error.empty()) {
        ShowError(std::vformat(GetMsg(MParseError), std::make_wformat_args(Info->FileName)), error);
    }
}

HANDLE WINAPI AnalyseW(const struct AnalyseInfo *Info) {
    std::wstring error;

    if (FormatFactory<JsonFormat>::ApplicableFileName(Info->FileName)) {
        if (FormatFactory<JsonFormat>::ApplicableFile(Info->FileName, error))
            return (HANDLE)FormatFactory<JsonFormat>::FORMAT;

        ShowParseError(Info, error);
        return NO_PANEL_HANDLE;
    }

    if (FormatFactory<YamlFormat>::ApplicableFileName(Info->FileName)) {
        if (FormatFactory<YamlFormat>::ApplicableFile(Info->FileName, error))
            return (HANDLE)FormatFactory<YamlFormat>::FORMAT;

        ShowParseError(Info, error);
        return NO_PANEL_HANDLE;
    }

    if (FormatFactory<TomlFormat>::ApplicableFileName(Info->FileName)) {
        if (FormatFactory<TomlFormat>::ApplicableFile(Info->FileName, error))
            return (HANDLE)FormatFactory<TomlFormat>::FORMAT;

        ShowParseError(Info, error);
        return NO_PANEL_HANDLE;
    }

    if (FormatFactory<JsonFormat>::ApplicableFile(Info->FileName, error))
        return (HANDLE)FormatFactory<JsonFormat>::FORMAT;

    if (FormatFactory<YamlFormat>::ApplicableFile(Info->FileName, error))
        return (HANDLE)FormatFactory<YamlFormat>::FORMAT;

    if (FormatFactory<TomlFormat>::ApplicableFile(Info->FileName, error))
        return (HANDLE)FormatFactory<TomlFormat>::FORMAT;

    return NO_PANEL_HANDLE;
}

HANDLE WINAPI OpenW(const struct OpenInfo *Info) {
    switch (Info->OpenFrom) {
    case OPEN_ANALYSE:
        auto AInfo = (const struct OpenAnalyseInfo *)Info->Data;

        return TryCall<HANDLE, NO_PANEL_HANDLE>([AInfo]() {
            switch ((intptr_t)AInfo->Handle) {
            case FormatFactory<JsonFormat>::FORMAT:
                return (HANDLE)FormatFactory<JsonFormat>::CreateFromFile(AInfo->Info->FileName);
            case FormatFactory<YamlFormat>::FORMAT:
                return (HANDLE)FormatFactory<YamlFormat>::CreateFromFile(AInfo->Info->FileName);
            case FormatFactory<TomlFormat>::FORMAT:
                return (HANDLE)FormatFactory<TomlFormat>::CreateFromFile(AInfo->Info->FileName);
            }
            return NO_PANEL_HANDLE;
        });
    }

    return NO_PANEL_HANDLE;
}

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo *Info) {
    auto fmt = (IFormat *)Info->hPanel;

    Info->StructSize = sizeof(*Info);
    Info->Flags = OPIF_ADDDOTS | OPIF_SHOWPRESERVECASE;
    Info->HostFile = fmt->HostFileName();
    Info->CurDir = fmt->CurrentDirectory();

    Info->Format = fmt->FormatName();
    Info->PanelTitle = Info->HostFile;
    Info->InfoLines = NULL;
    Info->InfoLinesNumber = 0;
    Info->DescrFiles = NULL;
    Info->DescrFilesNumber = 0;

    Info->PanelModesArray = NULL;
    Info->PanelModesNumber = 0;

    Info->StartPanelMode = '6';
    Info->StartSortMode = SM_DEFAULT;
    Info->StartSortOrder = 0;

    static KeyBarTitles KeyBar;
    static KeyBarLabel KeyBarLabels[1];

    KeyBar.CountLabels = 1;
    KeyBar.Labels = KeyBarLabels;
    KeyBarLabels[0].Key.VirtualKeyCode = (WORD)VK_F4;
    KeyBarLabels[0].Key.ControlKeyState = SHIFT_PRESSED;
    KeyBarLabels[0].Text = (LPTSTR)GetMsg(MRawEdit);
    Info->KeyBar = &KeyBar;

    Info->ShortcutData = Info->CurDir;
}

intptr_t WINAPI GetFindDataW(struct GetFindDataInfo *Info) {
    if (Info->OpMode & OPM_FIND)
        return 0;

    auto fmt = (IFormat *)Info->hPanel;
    auto &data = fmt->GetFindData();

    auto find_data = (CPluginPanelItem *)malloc(data.size() * sizeof(CPluginPanelItem));
    std::copy(data.begin(), data.end(), find_data);

    Info->StructSize = sizeof(*Info);
    Info->PanelItem = find_data;
    Info->ItemsNumber = data.size();

    return TRUE;
}

void WINAPI FreeFindDataW(const struct FreeFindDataInfo *Info) {
    StdFreeFindData(Info->PanelItem, Info->ItemsNumber);
}

intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo *Info) {
    auto fmt = (IFormat *)Info->hPanel;
    path curpath = fmt->GetPath();

    std::wstring dir = Info->Dir;
    if (dir.empty())
        return FALSE;

    if (dir[0] == L'\\') {
        curpath.clear();
        dir.erase(0, 1);
    }

    static std::wregex re(L"[\\\\/]");
    std::wsregex_token_iterator iter(dir.begin(), dir.end(), re, -1);
    std::wsregex_token_iterator end;
    for (; iter != end; ++iter) {
        std::wstring subdir = *iter;

        if (subdir.empty())
            continue;
        else if (subdir == L"..") {
            if (!curpath.empty())
                curpath.pop_back();
            else
                return 0;
        } else {
            path_element elem;
            elem.name = subdir;

            std::string chars = EncodeUTF8(subdir);
            unsigned int index;
            if (std::from_chars(chars.data(), chars.data() + chars.size(), index).ec == std::errc())
                elem.index = index;

            curpath.push_back(elem);
        }
    }

    return TryCall<bool>([&]() { return fmt->SetPath(curpath); });
}

std::wstring TemporaryFile(IFormat *fmt) {
    wchar_t szBuffer[MAX_PATH], szName[MAX_PATH];
    GetTempPath(MAX_PATH, szBuffer);
    GetTempFileName(szBuffer, fmt->FormatName(), 0, szName);

    return szName + L"."s + fmt->DefaultExtension();
}

void ViewItem(IFormat *fmt, const CPluginPanelItem &Item) {
    path_element elem(Item);

    auto path = TemporaryFile(fmt);

    if (TryCall<bool>([&]() { return fmt->GetFile(elem, path, g_bEditPrettyPrint ? g_nIndent : -1); }))
        StartupInfo.Viewer(path.c_str(), NULL, 0, 0, -1, -1, VF_DELETEONLYFILEONCLOSE | VF_DISABLEHISTORY,
                           CP_AUTODETECT);
}

void EditItem(IFormat *fmt, const CPluginPanelItem &Item) {
    path_element elem(Item);

    auto path = TemporaryFile(fmt);

    if (!TryCall<bool>([&]() { return fmt->GetFile(elem, path, g_bEditPrettyPrint ? g_nIndent : -1); }))
        return;

    if (StartupInfo.Editor(path.c_str(), NULL, 0, 0, -1, -1, VF_DISABLEHISTORY | EF_DISABLESAVEPOS, 0, 0,
                           CP_AUTODETECT) != EEC_MODIFIED)
        return;

    if (TryCall<bool>([&]() { return fmt->SetFile(elem, path); })) {
        DeleteFile(path.c_str());
        StartupInfo.PanelControl(PANEL_ACTIVE, FCTL_UPDATEPANEL, 0, NULL);
        StartupInfo.PanelControl(PANEL_ACTIVE, FCTL_REDRAWPANEL, 0, NULL);
    }
}

intptr_t WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo *Info) {
    if (Info->Rec.EventType != KEY_EVENT)
        return FALSE;

    CPanelInfo PInfo(false);
    if (PInfo.SelectedItems.empty())
        return FALSE;

    auto &Current = PInfo.PanelItems[PInfo.CurrentItem];

    auto fmt = (IFormat *)Info->hPanel;

    if (Info->Rec.Event.KeyEvent.wVirtualKeyCode == VK_F3 && Info->Rec.Event.KeyEvent.dwControlKeyState == 0) {
        ViewItem(fmt, Current);
        return TRUE;
    }

    if (Info->Rec.Event.KeyEvent.wVirtualKeyCode == VK_F4 && Info->Rec.Event.KeyEvent.dwControlKeyState == 0) {
        EditItem(fmt, Current);
        return TRUE;
    }

    return FALSE;
}

intptr_t WINAPI GetFilesW(struct GetFilesInfo *Info) {
    auto fmt = (IFormat *)Info->hPanel;

    CFarDialog Dialog(64, 10, L"CopyFiles");
    Dialog.SetUseID(true);
    Dialog.AddFrame(MCopy);

    auto prompt = Info->ItemsNumber > 1
                      ? std::vformat(GetMsg(MCopyManyTo), std::make_wformat_args(Info->ItemsNumber))
                      : std::vformat(GetMsg(MCopyOneTo), std::make_wformat_args(Info->PanelItem[0].FileName));

    std::wstring dest_path = Info->DestPath;

    Dialog.Add(new CFarTextItem(5, 3, 0, prompt));
    Dialog.Add(new CFarEditItem(5, 4, 58, 0, NULL, dest_path));

    Dialog.AddButtons(MOk, MCancel);

    if (Dialog.Display() != MOk)
        return FALSE;

    for (auto item = 0; item < Info->ItemsNumber; ++item) {
        path_element elem(CPluginPanelItem(Info->PanelItem[item]));

        auto path = CatFile(dest_path, elem.name + L"." + fmt->DefaultExtension());

        if (!TryCall<bool>([&]() { return fmt->GetFile(elem, path.c_str(), g_bExportPrettyPrint ? g_nIndent : -1); }))
            break;
    }

    return TRUE;
}

intptr_t WINAPI DeleteFilesW(const struct DeleteFilesInfo *Info) {
    auto fmt = (IFormat *)Info->hPanel;

    CFarDialog Dialog(64, 10, L"DeleteFiles");
    Dialog.SetUseID(true);
    Dialog.AddFrame(MDelete);

    auto prompt = Info->ItemsNumber > 1
                      ? std::vformat(GetMsg(MDeleteMany), std::make_wformat_args(Info->ItemsNumber))
                      : std::vformat(GetMsg(MDeleteOne), std::make_wformat_args(Info->PanelItem[0].FileName));

    Dialog.Add(new CFarTextItem(5, 3, 0, prompt));

    Dialog.AddButtons(MOk, MCancel);

    if (Dialog.Display() != MOk)
        return FALSE;

    std::set<path_element, std::greater<> > items;

    for (auto item = 0; item < Info->ItemsNumber; ++item)
        items.insert(CPluginPanelItem(Info->PanelItem[item]));

    for (const auto &elem : items) {
        if (!TryCall<bool>([&]() { return fmt->DeleteFile(elem); }))
            break;
    }

    return TRUE;
}

void WINAPI ClosePanelW(const struct ClosePanelInfo *Info) {
    auto fmt = (IFormat *)Info->hPanel;

    if (fmt->Changed()) {
        std::vector<std::wstring> items = {
            GetMsg(MStructuredFile), std::vformat(GetMsg(MSaveChanges), std::make_wformat_args(fmt->HostFileName()))};
        auto res = Message(FMSG_MB_OKCANCEL, NULL, 0, items);
        if (res == MB_OK)
            TryCall([&]() { fmt->ApplyChanges(g_bExportPrettyPrint ? g_nIndent : -1); });
    }

    delete fmt;
}

int WINAPI ConfigureW(int ItemNumber) {
    CFarDialog Dialog(64, 13, L"Configuration");
    Dialog.SetUseID(true);
    Dialog.AddFrame(MStructuredFile);

    Dialog.Add(new CFarCheckBoxItem(5, 3, 0, MExportPrettyPrint, &g_bExportPrettyPrint));
    Dialog.Add(new CFarCheckBoxItem(5, 4, 0, MEditPrettyPrint, &g_bEditPrettyPrint));
    Dialog.Add(new CFarTextItem(5, 5, 0, MIndent));
    Dialog.Add(new CFarEditItem(15, 5, 22, 0, NULL, g_nIndent, new CFarIntegerRangeValidator(0, 32)));
    Dialog.Add(new CFarCheckBoxItem(5, 7, 0, MSaveUnixEOL, &g_bSaveUnixEOL));

    Dialog.AddButtons(MOk, MCancel);

    Dialog.Display();

    SaveSettings();

    return TRUE;
}

void LoadSettings() {
    auto settings = CFarSettingsKey(L"StructuredFile");

#define DECLARE_PERSIST_LOAD settings
#include "PersistVars.h"
}

void SaveSettings() {
    auto settings = CFarSettingsKey(L"StructuredFile");

#define DECLARE_PERSIST_SAVE settings
#include "PersistVars.h"
}

void WINAPI ExitFARW() {
    SaveSettings();
}
