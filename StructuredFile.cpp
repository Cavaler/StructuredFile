#include "StructuredFile.h"
#include <initguid.h>
DEFINE_GUID(GUID_StructuredFile, 0xec26a161, 0xf889, 0x45e3, 0xa8, 0xa1, 0x9, 0x96, 0xe, 0xb4, 0x8a, 0x50);
DEFINE_GUID(GUID_StructuredFileConfig, 0x904560e, 0x424f, 0x49ca, 0xb6, 0x2, 0xdb, 0x46, 0xac, 0x78, 0x98, 0xf2);
DEFINE_GUID(GUID_StructuredFileRun, 0x251b81b4, 0xc0c5, 0x4422, 0xae, 0x73, 0xc, 0x5, 0xd2, 0xfb, 0xe2, 0x1d);

#include "version.h"

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

    Info->PluginConfig.Count = 0;
    Info->PluginMenu.Count = 0;
    Info->DiskMenu.Count = 0;

    Info->CommandPrefix = NULL;
}

void WINAPI FAR_EXPORT(SetStartupInfo)(const PluginStartupInfo *Info) {
    StartupInfo = *Info;
    StartupInfo.m_GUID = GUID_StructuredFile;

    g_pszOKButton = GetMsg(MOk);
    g_pszErrorTitle = GetMsg(MError);
    CFarIntegerRangeValidator::s_szErrorMsg = GetMsg(MInvalidNumber);
    CFarIntegerRangeValidator::s_szHelpTopic = _T("REInvalidNumber");
    CFarDialog::AutoHotkeys = true;
    CFarDialog::SetDefaultCancelID(MCancel);
}

HANDLE WINAPI AnalyseW(const struct AnalyseInfo *Info) {
    return NO_PANEL_HANDLE;
}

HANDLE WINAPI OpenW(const struct OpenInfo *Info) {
    switch (Info->OpenFrom) {
    case OPEN_ANALYSE:
        auto AInfo = (const struct OpenAnalyseInfo *)Info->Data;
        return NO_PANEL_HANDLE;
    }

    return NO_PANEL_HANDLE;
}

void WINAPI GetOpenPanelInfoW(struct OpenPanelInfo *Info) {}

intptr_t WINAPI GetFindDataW(struct GetFindDataInfo *Info) {
    return 0;
}

intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo *Info) {
    return 0;
}

intptr_t WINAPI PutFilesW(const struct PutFilesInfo *Info) {
    return 0;
}

intptr_t WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo *Info) {
    return 0;
}

void WINAPI ClosePanelW(const struct ClosePanelInfo *Info) {}

void WINAPI ExitFARW(const ExitInfo *Info) {}
