#include <cstdio>
#include <cmath>
#include <d3d11.h>
#include <dwmapi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <windows.h>
#include "xorstr.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

static void Log(const char *fmt, ...) {
    char buf[512];
    va_list a;
    va_start(a, fmt);
    vsnprintf(buf, sizeof(buf), fmt, a);
    va_end(a);
    printf(XS("[DBG] %s\n"), buf);
    fflush(stdout);
}

static void LogHR(const char *label, HRESULT hr) {
    if (SUCCEEDED(hr)) { Log("%s -> OK", label); return; }
    char msg[256] = {};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, hr, 0, msg, sizeof(msg), nullptr);
    for (int i = (int)strlen(msg) - 1; i >= 0 && (msg[i] == '\n' || msg[i] == '\r'); --i)
        msg[i] = 0;
    Log("%s -> FAILED 0x%08X: %s", label, (unsigned)hr, msg);
}

static ID3D11Device *g_dev = nullptr;
static ID3D11DeviceContext *g_ctx = nullptr;
static IDXGISwapChain *g_sc = nullptr;
static ID3D11RenderTargetView *g_rtv = nullptr;
static HWND g_hwnd = nullptr;
static bool g_splash_done = false;
static float g_splash_elapsed = 0.0f;

static void CreateRTV() {
    ID3D11Texture2D *bb = nullptr;
    if (SUCCEEDED(g_sc->GetBuffer(0, IID_PPV_ARGS(&bb)))) {
        g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
        bb->Release();
    }
}

static bool CreateD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL lvls[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
    D3D_FEATURE_LEVEL fl;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, lvls, 2, D3D11_SDK_VERSION, &sd, &g_sc, &g_dev, &fl, &g_ctx);
    LogHR(XS("D3D11CreateDeviceAndSwapChain"), hr);
    if (hr == DXGI_ERROR_UNSUPPORTED) {
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP,
            nullptr, 0, lvls, 2, D3D11_SDK_VERSION, &sd, &g_sc, &g_dev, &fl, &g_ctx);
        LogHR(XS("D3D11CreateDeviceAndSwapChain (WARP)"), hr);
    }
    if (FAILED(hr)) return false;
    g_ctx->Release();
    g_sc->Release();
    return true;
}

static void CleanupD3D() {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    if (g_sc)  { g_sc->Release();  g_sc  = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (g_dev && wParam != SIZE_MINIMIZED) {
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            g_sc->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRTV();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void ApplyBypass(HWND hwnd) {
    auto fn = reinterpret_cast<BOOL(WINAPI*)(HWND, DWORD)>(
        GetProcAddress(GetModuleHandleW(XWS(L"user32.dll")), XS("SetWindowDisplayAffinity")));
    if (fn) {
        BOOL ok = fn(hwnd, 0x11);
        Log(XS("affinity set -> %s"), ok ? XS("OK") : XS("FAILED"));
    }
    MARGINS m = {-1, -1, -1, -1};
    DwmExtendFrameIntoClientArea(hwnd, &m);
    BOOL noT = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &noT, sizeof(noT));
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

int main() {
    SetConsoleTitleA(XS("NVIDIA Display Container"));
    Log(XS("starting"));

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    auto clsName = XWS(L"NVIDIA_DisplayContainer_Window");
    wc.lpszClassName = clsName;
    RegisterClassExW(&wc);

    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);

    auto winTitle = XWS(L"NVIDIA Display Container");
    g_hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        clsName, winTitle,
        WS_POPUP,
        0, 0, W, H,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!g_hwnd) { Log(XS("CreateWindowExW failed")); return 1; }

    SetLayeredWindowAttributes(g_hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ApplyBypass(g_hwnd);

    if (!CreateD3D(g_hwnd)) {
        Log(XS("CreateD3D failed"));
        CleanupD3D();
        UnregisterClassW(clsName, wc.hInstance);
        return 1;
    }

    ShowWindow(g_hwnd, SW_SHOWNA);
    UpdateWindow(g_hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    bool running = true;
    bool click_through = false;
    int frames = 0;
    bool injected = false;
    LARGE_INTEGER freq, last;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);

    while (running) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        float dt = (float)(now.QuadPart - last.QuadPart) / (float)freq.QuadPart;
        last = now;

        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        if (!g_splash_done) {
            g_splash_elapsed += dt;
            if (g_splash_elapsed >= 10.0f) {
                g_splash_done = true;
                if (!injected) {
                    wchar_t path[MAX_PATH];
                    GetModuleFileNameW(nullptr, path, MAX_PATH);
                    wchar_t *s = wcsrchr(path, L'\\');
                    if (s) {
                        wcscpy_s(s + 1, MAX_PATH - (s - path + 1), XWS(L"nvinjector.exe"));
                        STARTUPINFOW si = {sizeof(si)};
                        PROCESS_INFORMATION pi;
                        if (CreateProcessW(path, nullptr, nullptr, nullptr, FALSE,
                            CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                            WaitForSingleObject(pi.hProcess, 5000);
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                            injected = true;
                        }
                    }
                }
            }
        }

        if (GetAsyncKeyState(VK_INSERT) & 1) {
            click_through = !click_through;
            LONG ex = GetWindowLongW(g_hwnd, GWL_EXSTYLE);
            if (click_through) ex |= WS_EX_TRANSPARENT;
            else               ex &= ~WS_EX_TRANSPARENT;
            SetWindowLongW(g_hwnd, GWL_EXSTYLE, ex);
        }

        if (GetAsyncKeyState(VK_END) & 1) running = false;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        float splash_alpha = 1.0f;
        if (g_splash_elapsed > 9.0f)
            splash_alpha = 1.0f - (g_splash_elapsed - 9.0f);

        ImDrawList *dl = ImGui::GetForegroundDrawList();
        ImVec2 ss = ImGui::GetIO().DisplaySize;

        if (!g_splash_done) {
            dl->AddRectFilled(ImVec2(0,0), ss, IM_COL32(0,0,0,(int)(220 * splash_alpha)));
            for (int i = 0; i < 30; i++) {
                float y = (float)(rand() % (int)ss.y);
                float x = (float)(rand() % 200);
                dl->AddLine(ImVec2(x, y), ImVec2(x + 200 + rand()%400, y),
                    IM_COL32(0, 255, (int)(100 + rand()%155), (int)(60 * splash_alpha)));
            }
            float scan = fmodf(g_splash_elapsed * 30.0f, ss.y);
            dl->AddRectFilled(ImVec2(0, scan), ImVec2(ss.x, scan + 3),
                IM_COL32(0, 255, 0, (int)(40 * splash_alpha)));

            auto title = XS("NVIDIA Display Container v2.1.0.8");
            float tw = ImGui::CalcTextSize(title).x;
            float tx = (ss.x - tw) / 2.0f;
            float ty = ss.y / 2.0f - 60;
            dl->AddText(ImVec2(tx+2, ty+2), IM_COL32(0,255,0,(int)(180 * splash_alpha)), title);
            dl->AddText(ImVec2(tx, ty), IM_COL32(0,255,0,(int)(255 * splash_alpha)), title);

            auto sub = XS(">> initializing display services <<");
            float sw = ImGui::CalcTextSize(sub).x;
            dl->AddText(ImVec2((ss.x-sw)/2, ty+50), IM_COL32(0,200,0,(int)(200 * splash_alpha)), sub);

            float prog = fminf(g_splash_elapsed / 10.0f, 1.0f);
            float bar_w = 300;
            float bar_h = 4;
            float bx = (ss.x - bar_w) / 2.0f;
            float by = ty + 90;
            dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx+bar_w, by+bar_h),
                IM_COL32(0,60,0,(int)(150 * splash_alpha)));
            dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx+bar_w*prog, by+bar_h),
                IM_COL32(0,255,0,(int)(200 * splash_alpha)));
            char pct[16];
            sprintf_s(pct, "%d%%", (int)(prog * 100));
            dl->AddText(ImVec2(bx+bar_w+10, by-4), IM_COL32(0,255,0,(int)(180 * splash_alpha)), pct);
        }

        if (g_splash_done) {
            ImGui::GetStyle().Alpha = 0.92f;
            ImGui::Begin(XS("NVIDIA Display Container v2.1.0.8"));
            ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), XS("container active | frame %d"), frames);
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                click_through ? XS("[INSERT] passthrough: on") : XS("[INSERT] passthrough: off"));
            ImGui::Text(XS("[END] shutdown"));
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), XS("protection: active"));
            ImGui::End();
        }

        ImGui::Render();

        float cc[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, cc);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_sc->Present(1, 0);

        if (frames == 0) ;
        ++frames;
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupD3D();
    DestroyWindow(g_hwnd);
    UnregisterClassW(clsName, wc.hInstance);
    return 0;
}
