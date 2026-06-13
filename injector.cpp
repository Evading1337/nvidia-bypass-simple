#include <cstdio>
#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include "xorstr.h"

static bool inject(DWORD pid, const std::wstring &dll) {
    HANDLE proc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!proc) {
        printf(XS("  OpenProcess(%lu) failed\n"), pid);
        return false;
    }

    size_t nb = (dll.size() + 1) * sizeof(wchar_t);
    void *mem = VirtualAllocEx(proc, nullptr, nb, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) {
        CloseHandle(proc);
        return false;
    }

    WriteProcessMemory(proc, mem, dll.c_str(), nb, nullptr);

    auto loadlib = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleW(XWS(L"kernel32.dll")), XS("LoadLibraryW"));

    HANDLE t = CreateRemoteThread(proc, nullptr, 0, loadlib, mem, 0, nullptr);
    if (!t) {
        VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
        CloseHandle(proc);
        return false;
    }

    WaitForSingleObject(t, 5000);
    CloseHandle(t);
    VirtualFreeEx(proc, mem, 0, MEM_RELEASE);
    CloseHandle(proc);
    return true;
}

int main() {
    wchar_t dir[MAX_PATH];
    GetModuleFileNameW(nullptr, dir, MAX_PATH);
    wchar_t *slash = wcsrchr(dir, L'\\');
    if (slash) *(slash + 1) = 0;
    std::wstring dll = std::wstring(dir) + XWS(L"nvhook.dll");

    if (GetFileAttributesW(dll.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return 1;
    }

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return 1;
    }

    PROCESSENTRY32W pe = {sizeof(pe)};
    int n = 0;

    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, XWS(L"nvcontainer.exe")) == 0) {
                if (inject(pe.th32ProcessID, dll))
                    n++;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);

    return n > 0 ? 0 : 1;
}
