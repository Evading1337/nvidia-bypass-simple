#include <windows.h>
#include <tlhelp32.h>
#include "xorstr.h"

#pragma comment(lib, "advapi32.lib")

static SERVICE_STATUS g_status = {};
static SERVICE_STATUS_HANDLE g_hStatus = nullptr;

static bool inject(DWORD pid) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    wchar_t *s = wcsrchr(path, L'\\');
    if (!s) return false;
    wcscpy_s(s + 1, MAX_PATH - (s - path + 1), XWS(L"nvhook.dll"));

    HANDLE proc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!proc) return false;

    size_t nb = (wcslen(path) + 1) * sizeof(wchar_t);
    void *mem = VirtualAllocEx(proc, nullptr, nb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) { CloseHandle(proc); return false; }

    WriteProcessMemory(proc, mem, path, nb, nullptr);
    auto loadlib = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleW(XWS(L"kernel32.dll")), XS("LoadLibraryW"));

    HANDLE t = CreateRemoteThread(proc, nullptr, 0, loadlib, mem, 0, nullptr);
    if (!t) { VirtualFreeEx(proc, mem, 0, MEM_RELEASE); CloseHandle(proc); return false; }
    WaitForSingleObject(t, 5000);
    CloseHandle(t);
    VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
    CloseHandle(proc);
    return true;
}

static void inject_nvcontainer() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe = {sizeof(pe)};
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, XWS(L"nvcontainer.exe")) == 0)
                inject(pe.th32ProcessID);
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
}

static void ensure_registry() {
    HKEY hKey;
    LSTATUS r = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
        XWS(L"SOFTWARE\\NVIDIA Corporation\\Global\\NvApp\\ShadowPlay\\FTS"),
        0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hKey, nullptr);
    if (r == ERROR_SUCCESS) {
        DWORD val = 36;
        RegSetValueExW(hKey, XWS(L"{497B8458-4244-4EE6-BFEA-F3D2BA294F21}"),
            0, REG_DWORD, (BYTE*)&val, sizeof(val));
        RegCloseKey(hKey);
    }
}

static DWORD WINAPI worker(LPVOID) {
    ensure_registry();
    inject_nvcontainer();
    return 0;
}

static void WINAPI ctrl(DWORD code) {
    if (code == SERVICE_CONTROL_STOP) {
        g_status.dwCurrentState = SERVICE_STOPPED;
        g_status.dwWin32ExitCode = 0;
        SetServiceStatus(g_hStatus, &g_status);
        ExitProcess(0);
    }
    SetServiceStatus(g_hStatus, &g_status);
}

static void WINAPI start(DWORD, wchar_t **) {
    g_hStatus = RegisterServiceCtrlHandlerW(XWS(L"NVIDIA_DisplayContainer"), ctrl);
    if (!g_hStatus) return;

    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_status.dwCurrentState = SERVICE_RUNNING;
    g_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(g_hStatus, &g_status);

    HANDLE h = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    if (h) CloseHandle(h);

    while (g_status.dwCurrentState == SERVICE_RUNNING) {
        Sleep(15000);
        inject_nvcontainer();
    }
}

static void install_service() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);

    SC_HANDLE mgr = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!mgr) return;

    SC_HANDLE svc = CreateServiceW(mgr,
        XWS(L"NVIDIA_DisplayContainer"),
        XWS(L"NVIDIA Display Container"),
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
        path, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (svc) CloseServiceHandle(svc);
    CloseServiceHandle(mgr);
}

static void remove_service() {
    SC_HANDLE mgr = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!mgr) return;

    SC_HANDLE svc = OpenServiceW(mgr,
        XWS(L"NVIDIA_DisplayContainer"), SERVICE_ALL_ACCESS);
    if (svc) {
        DeleteService(svc);
        CloseServiceHandle(svc);
    }
    CloseServiceHandle(mgr);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], XS("--install")) == 0) {
            install_service();
            return 0;
        }
        if (strcmp(argv[1], XS("--remove")) == 0) {
            remove_service();
            return 0;
        }
    }

    SERVICE_TABLE_ENTRYW tbl[] = {
        { (LPWSTR)XWS(L"NVIDIA_DisplayContainer"), start },
        { nullptr, nullptr }
    };
    if (!StartServiceCtrlDispatcherW(tbl))
        CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    WaitForSingleObject(GetCurrentThread(), INFINITE);
    return 0;
}
