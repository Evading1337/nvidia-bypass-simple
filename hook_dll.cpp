#include <windows.h>
#include "xorstr.h"
#include <MinHook.h>

typedef BOOL(WINAPI *GetWindowDisplayAffinity_t)(HWND, LPDWORD);
static GetWindowDisplayAffinity_t g_original = nullptr;

static BOOL WINAPI hook_GetWindowDisplayAffinity(HWND hWnd, LPDWORD pdwAffinity) {
    BOOL result = g_original(hWnd, pdwAffinity);
    if (result && pdwAffinity)
        *pdwAffinity = WDA_NONE;
    return result;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(GetModuleHandleW(XWS(L"hook_dll.dll")));
        MH_Initialize();
        HMODULE mod = GetModuleHandleW(XWS(L"user32.dll"));
        auto fn = reinterpret_cast<GetWindowDisplayAffinity_t>(
            GetProcAddress(mod, XS("GetWindowDisplayAffinity")));
        if (fn) {
            MH_CreateHook(fn, (LPVOID)hook_GetWindowDisplayAffinity, (LPVOID *)&g_original);
            MH_EnableHook(MH_ALL_HOOKS);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    return TRUE;
}
