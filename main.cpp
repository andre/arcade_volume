#define UNICODE
#define _UNICODE
#define IDR_VOLUME_WAV 101

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Winmm.lib")

#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <psapi.h>
#include <wrl/client.h>

#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>

using Microsoft::WRL::ComPtr;

HINSTANCE g_hInst;
HWND g_hwnd = nullptr;
ComPtr<IAudioEndpointVolume> g_endpointVolume;
float g_volumeLevel = 0.0f;
BOOL g_muted = FALSE;

bool g_visible = false;
bool g_fading = false;
float g_opacity = 0.8f;
std::chrono::steady_clock::time_point g_lastShown;
const int FADE_TIMER_ID = 1;
const int HOTKEY_SHUTDOWN = 2;

/* We play a little tick sound when the volume changes. The .wav file is embedded in the resource section of the executable. */
void PlayEmbeddedSound() {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_VOLUME_WAV), RT_RCDATA);
    HGLOBAL hData = LoadResource(NULL, hRes);
    DWORD size = SizeofResource(NULL, hRes);
    void* pData = LockResource(hData);
    if (!pData) return;

    PlaySound((LPCWSTR)pData, NULL, SND_MEMORY | SND_ASYNC);
}

/* This is here because we ONLY adjust volume or display our little window when the windows explorer shell is NOT active.
In other words, this whole program is only functional when windows explorer shell is not running. */
bool isExplorerShellActive() {
    HWND shellWnd = GetShellWindow();
    if (!shellWnd) return false;

    DWORD shellPid = 0;
    GetWindowThreadProcessId(shellWnd, &shellPid);
    if (shellPid == 0) return false;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, shellPid);
    if (!hProc) return false;

    WCHAR name[MAX_PATH] = {};
    DWORD len = GetProcessImageFileNameW(hProc, name, MAX_PATH);
    std::wstring path(name);
    size_t lastSlash = path.find_last_of(L"\\/");
    std::wstring exeName = (lastSlash != std::wstring::npos) ? path.substr(lastSlash + 1) : path;

    CloseHandle(hProc);

    return (len > 0 && _wcsicmp(exeName.c_str(), L"explorer.exe") == 0);
}


/* Display our window, and set a timestamp for when it will go away.*/
void ShowOverlay() {
    g_lastShown = std::chrono::steady_clock::now();
    g_fading = false;
    g_opacity = 0.8f;
    SetLayeredWindowAttributes(g_hwnd, 0, BYTE(g_opacity * 255), LWA_ALPHA);
    ShowWindow(g_hwnd, SW_SHOWNOACTIVATE);
    InvalidateRect(g_hwnd, NULL, TRUE);
    g_visible = true;
    SetTimer(g_hwnd, FADE_TIMER_ID, 16, NULL);  // ~30fps
}

/* Adjust the volume by a delta value. Volume range is between 0 and one, so a reasonable delta is 0.02.  */
void AdjustVolume(float delta) {
    if (isExplorerShellActive()) return; // do nothing if explorer is active
    if (!g_endpointVolume) return;
    if (delta > 0) g_endpointVolume->SetMute(false, NULL);;
    float current = 0.0f;
    g_endpointVolume->GetMasterVolumeLevelScalar(&current);
    float newVolume = max(0.0f, min(current + delta , 1.0f));
    g_endpointVolume->SetMasterVolumeLevelScalar(newVolume, NULL);
    PlayEmbeddedSound();
    ShowOverlay();
}

void ToggleMute() {
    if (isExplorerShellActive()) return; // do nothing if explorer is active
    if (!g_endpointVolume) return;
    BOOL muted = FALSE;
    g_endpointVolume->GetMute(&muted);
    muted = !muted;
    g_endpointVolume->SetMute(muted, NULL);
    if(!muted) PlayEmbeddedSound();
    ShowOverlay();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        // Register for raw input
        {
            RAWINPUTDEVICE rid;
            rid.usUsagePage = 0x0C;
            rid.usUsage = 0x01;
            rid.dwFlags = RIDEV_INPUTSINK;
            rid.hwndTarget = hwnd;
            RegisterRawInputDevices(&rid, 1, sizeof(rid));
        }

        // Register hotkey Ctrl+Alt+Y
        RegisterHotKey(hwnd, HOTKEY_SHUTDOWN, MOD_CONTROL | MOD_ALT, 'Y');        
        break;

    case WM_INPUT:        
        {
            UINT size = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
            BYTE* buffer = new BYTE[size];
            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size) {
                RAWINPUT* raw = (RAWINPUT*)buffer;
                if (raw->header.dwType == RIM_TYPEHID && raw->data.hid.dwSizeHid >= 2) {
                    USHORT usage = raw->data.hid.bRawData[1];
                    if (usage == 0xE9) AdjustVolume(0.02f);       // Volume Up
                    else if (usage == 0xEA) AdjustVolume(-0.02f);  // Volume Down
                    else if (usage == 0xE2) ToggleMute();      // Mute
                }
            }
            delete[] buffer;
        }
        break;

    case WM_HOTKEY:
        if (wParam == HOTKEY_SHUTDOWN) {                        
            // MessageBox(NULL, L"Ctrl+Alt+Y pressed!", L"Hotkey", MB_OK);
            // Attempt to shut down the system
            HANDLE hToken;
            TOKEN_PRIVILEGES tkp;

            // Get a token for this process
            if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
                // Get the LUID for the shutdown privilege
                LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

                tkp.PrivilegeCount = 1;  // One privilege to set
                tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

                // Adjust the token's privileges
                AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

                if (GetLastError() == ERROR_SUCCESS) {
                    // Shut down the system and force close applications
                    ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
                }

                CloseHandle(hToken);
            }
        }       
        break;

    case WM_TIMER:
        if (g_visible) {
            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float, std::milli>(now - g_lastShown).count();

            if (!g_fading && elapsed > 1000) {
                g_fading = true;
                g_lastShown = now;
            }

            if (g_fading) {
                g_opacity -= 0.066f;  // Approx 30 steps over 500ms
                if (g_opacity <= 0.0f) {
                    g_opacity = 0.0f;
                    KillTimer(hwnd, FADE_TIMER_ID);
                    ShowWindow(hwnd, SW_HIDE);
                    g_visible = false;
                }
                SetLayeredWindowAttributes(hwnd, 0, BYTE(g_opacity * 255), LWA_ALPHA);
            }
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);

            HFONT hFont = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            SelectObject(hdc, hFont);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));

            // get the current volume level and mute state for display
            g_endpointVolume->GetMasterVolumeLevelScalar(&g_volumeLevel);
            g_endpointVolume->GetMute(&g_muted);


            // Calculate bar dimensions
            int barMaxWidth = rc.right - 100;
            int barWidth = int(barMaxWidth * g_volumeLevel); 

            RECT bar = { 20, 20, 20+barWidth, rc.bottom - 20 }; //left,top,right,bottom
            HBRUSH fill = CreateSolidBrush(g_muted ? RGB(200, 50, 50) : RGB(50, 200, 50));            
            FillRect(hdc, &bar, fill);            
            DeleteObject(fill);

            RECT outline = { bar.left-1, bar.top-1, 20+barMaxWidth+1, bar.bottom+1}; // 1 pixel larger
            HBRUSH outlineBrush = CreateSolidBrush(RGB(255, 255, 255));
            FrameRect(hdc, &outline, outlineBrush);
            DeleteObject(outlineBrush);
    
            // Text representation of the volume, either a percentage like "20%" or "MUTE"
            RECT textRect = { rc.right - 120, rc.bottom - 40, rc.right - 20, rc.bottom - 20 };
            std::wstring text = g_muted ? L"MUTE": std::to_wstring(int(g_volumeLevel * 100)) + L"%";
            DrawText(hdc, text.c_str(), -1, &textRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

            DeleteObject(hFont);
            EndPaint(hwnd, &ps);
        }
        break;

    case WM_DESTROY:
        UnregisterHotKey(hwnd, HOTKEY_SHUTDOWN);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Entry point, set up the window, audio endpoint, and the main event loop */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
    g_hInst = hInstance;
    CoInitialize(NULL);

    // Init endpoint volume
    ComPtr<IMMDeviceEnumerator> pEnumerator;
    ComPtr<IMMDevice> pDevice;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, IID_PPV_ARGS(&pEnumerator));
    pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, reinterpret_cast<void**>(g_endpointVolume.GetAddressOf()));

    // Register window class
    WNDCLASS wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"VolumeOSD";
    wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    RegisterClass(&wc);

    // Create layered, tool-style window
    int width = 400, height = 60;
    int x = GetSystemMetrics(SM_CXSCREEN) - width - 20;
    int y = GetSystemMetrics(SM_CYSCREEN) - height - 40;

    g_hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName, L"",
        WS_POPUP,
        x, y, width, height,
        NULL, NULL, hInstance, NULL);

    SetLayeredWindowAttributes(g_hwnd, 0, BYTE(255 * g_opacity), LWA_ALPHA);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CoUninitialize();
    return 0;
}
