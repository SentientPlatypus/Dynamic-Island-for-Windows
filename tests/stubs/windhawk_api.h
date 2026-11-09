// Stand-in for the header Windhawk force-includes when it compiles a mod.
// Only used by the local mingw compile check. Not part of the mod.
#pragma once
#include <windows.h>
#include <cmath>
// MSVC provides _isnan. mingw does not. Compile-check only.
#ifndef _isnan
#define _isnan std::isnan
#endif

#ifndef WH_MOD_ID
#define WH_MOD_ID L"dynamic-island-for-windows-gcal"
#endif
#ifndef WH_MOD_VERSION
#define WH_MOD_VERSION L"1.4.0-gcal.3"
#endif

inline int Wh_GetIntSetting(PCWSTR) { return 0; }
inline int Wh_GetIntValue(PCWSTR, int defaultValue) { return defaultValue; }
inline BOOL Wh_SetIntValue(PCWSTR, int) { return TRUE; }
inline PCWSTR Wh_GetStringSetting(PCWSTR) { return L""; }
inline void Wh_FreeStringSetting(PCWSTR) {}
inline BOOL Wh_GetModStoragePath(WCHAR* path, size_t cch) {
    if (path && cch) path[0] = 0;
    return FALSE;
}
inline void Wh_Log(PCWSTR, ...) {}
inline BOOL Wh_SetFunctionHook(void*, void*, void*) { return TRUE; }
