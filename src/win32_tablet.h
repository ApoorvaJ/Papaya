#pragma once

#include <wintab.h>

#define PACKETDATA PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE
#define PACKETMODE PK_BUTTONS
#include <pktdef.h>

typedef UINT (API * WTINFOA) (UINT, UINT, LPVOID);
typedef HCTX (API * WTOPENA)(HWND, LPLOGCONTEXTA, BOOL);
typedef BOOL (API * WTGETA) (HCTX, LPLOGCONTEXT);
typedef BOOL (API * WTSETA) (HCTX, LPLOGCONTEXT);
typedef BOOL (API * WTCLOSE) (HCTX);
typedef BOOL (API * WTENABLE) (HCTX, BOOL);
typedef BOOL (API * WTPACKET) (HCTX, UINT, LPVOID);
typedef BOOL (API * WTOVERLAP) (HCTX, BOOL);
typedef BOOL (API * WTSAVE) (HCTX, LPVOID);
typedef BOOL (API * WTCONFIG) (HCTX, HWND);
typedef HCTX (API * WTRESTORE) (HWND, LPVOID, BOOL);
typedef BOOL (API * WTEXTSET) (HCTX, UINT, LPVOID);
typedef BOOL (API * WTEXTGET) (HCTX, UINT, LPVOID);
typedef BOOL (API * WTQUEUESIZESET) (HCTX, int);
typedef int  (API * WTDATAPEEK) (HCTX, UINT, UINT, int, LPVOID, LPINT);
typedef int  (API * WTPACKETSGET) (HCTX, int, LPVOID);
typedef HMGR (API * WTMGROPEN) (HWND, UINT);
typedef BOOL (API * WTMGRCLOSE) (HMGR);
typedef HCTX (API * WTMGRDEFCONTEXT) (HMGR, BOOL);
typedef HCTX (API * WTMGRDEFCONTEXTEX) (HMGR, UINT, BOOL);

struct WintabInfo
{
    HINSTANCE Dll;
    HCTX      Context;

    LONG  PosX, PosY;
    float Pressure;

    LONG RangeX, RangeY;
    UINT MaxPressure;

    WTINFOA           WTInfoA;
    WTOPENA           WTOpenA;
    WTGETA            WTGetA;
    WTSETA            WTSetA;
    WTCLOSE           WTClose;
    WTPACKET          WTPacket;
    WTENABLE          WTEnable;
    WTOVERLAP         WTOverlap;
    WTSAVE            WTSave;
    WTCONFIG          WTConfig;
    WTRESTORE         WTRestore;
    WTEXTSET          WTExtSet;
    WTEXTGET          WTExtGet;
    WTQUEUESIZESET    WTQueueSizeSet;
    WTDATAPEEK        WTDataPeek;
    WTPACKETSGET      WTPacketsGet;
    WTMGROPEN         WTMgrOpen;
    WTMGRCLOSE        WTMgrClose;
    WTMGRDEFCONTEXT   WTMgrDefContext;
    WTMGRDEFCONTEXTEX WTMgrDefContextEx;
};

#define GETPROCADDRESS(type, func)                                              \
    Wintab->func = (type)GetProcAddress(Wintab->Dll, #func);                    \
    if (!Wintab->func)                                                          \
    {                                                                           \
        OutputDebugStringA("Function " #func " not found in Wintab32.dll.\n");  \
        return false;                                                           \
    }

bool Wintab_Load(WintabInfo* Wintab, HWND Window)
{
    // Load Wintab DLL and get function addresses
    {
        Wintab->Dll = LoadLibraryA("Wintab32.dll");
        if (!Wintab->Dll)
        {
            OutputDebugStringA("Wintab32.dll not found.\n");
            return false;
        }

        GETPROCADDRESS(WTINFOA           , WTInfoA);
        GETPROCADDRESS(WTOPENA           , WTOpenA);
        GETPROCADDRESS(WTGETA            , WTGetA);
        GETPROCADDRESS(WTSETA            , WTSetA);
        GETPROCADDRESS(WTCLOSE           , WTClose);
        GETPROCADDRESS(WTPACKET          , WTPacket);
        GETPROCADDRESS(WTENABLE          , WTEnable);
        GETPROCADDRESS(WTOVERLAP         , WTOverlap);
        GETPROCADDRESS(WTSAVE            , WTSave);
        GETPROCADDRESS(WTCONFIG          , WTConfig);
        GETPROCADDRESS(WTRESTORE         , WTRestore);
        GETPROCADDRESS(WTEXTSET          , WTExtSet);
        GETPROCADDRESS(WTEXTGET          , WTExtGet);
        GETPROCADDRESS(WTQUEUESIZESET    , WTQueueSizeSet);
        GETPROCADDRESS(WTDATAPEEK        , WTDataPeek);
        GETPROCADDRESS(WTPACKETSGET      , WTPacketsGet);
        GETPROCADDRESS(WTMGROPEN         , WTMgrOpen);
        GETPROCADDRESS(WTMGRCLOSE        , WTMgrClose);
        GETPROCADDRESS(WTMGRDEFCONTEXT   , WTMgrDefContext);
        GETPROCADDRESS(WTMGRDEFCONTEXTEX , WTMgrDefContextEx);
    }

    if (!Wintab->WTInfoA(0, 0, NULL))
    {
        OutputDebugStringA("WinTab services not available.\n");
        return false;
    }

    // Open context
    {
        LOGCONTEXT LogContext = {0};
        AXIS       RangeX     = {0};
        AXIS       RangeY     = {0};
        AXIS       Pressure   = {0};

        Wintab->WTInfoA(WTI_DDCTXS, 0, &LogContext);
        Wintab->WTInfoA(WTI_DEVICES, DVC_X, &RangeX);
        Wintab->WTInfoA(WTI_DEVICES, DVC_Y, &RangeY);
        Wintab->WTInfoA(WTI_DEVICES, DVC_NPRESSURE, &Pressure);

        LogContext.lcPktData = PACKETDATA; // ??
        LogContext.lcOptions |= CXO_SYSTEM;
        LogContext.lcOptions |= CXO_MESSAGES;
        LogContext.lcPktMode = PACKETMODE;
        LogContext.lcMoveMask = PACKETDATA;
        LogContext.lcBtnUpMask = LogContext.lcBtnDnMask;

        LogContext.lcInOrgX = 0;
        LogContext.lcInOrgY = 0;
        LogContext.lcInExtX = RangeX.axMax;
        LogContext.lcInExtY = RangeY.axMax;

        LogContext.lcOutOrgX = 0;
        LogContext.lcOutOrgY = 0;
        LogContext.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
        LogContext.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);

        LogContext.lcSysOrgX = 0;
        LogContext.lcSysOrgY = 0;
        LogContext.lcSysExtX = GetSystemMetrics(SM_CXSCREEN);
        LogContext.lcSysExtY = GetSystemMetrics(SM_CYSCREEN);

        Wintab->Context = Wintab->WTOpenA(Window, &LogContext, TRUE);
        
        if (!Wintab->Context)
        {
            OutputDebugStringA("WinTab context couldn't be opened.\n");
            return false;
        }

        // Get tablet capabilites
        {
            Wintab->MaxPressure = Pressure.axMax;
            Wintab->RangeX      = RangeX.axMax;
            Wintab->RangeY      = RangeY.axMax;
        }
    }

    return true;
}
