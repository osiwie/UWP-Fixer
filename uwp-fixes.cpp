#include <windows.h>
#include <shellapi.h>
#include <iostream>

#define ID_TRAY_APP_ICON 1
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM 2

bool mouseSaved = false;
POINT savedPoint;
NOTIFYICONDATA nid = {};
HWND robloxWindow = NULL;

bool IsRobloxActive()
{
    HWND foreground = GetForegroundWindow();
    if (foreground)
    {
        char windowTitle[256];
        GetWindowText(foreground, windowTitle, sizeof(windowTitle));
        if (strcmp(windowTitle, "Roblox") == 0)
        {
            robloxWindow = foreground;
            return true;
        }
    }
    robloxWindow = NULL;
    return false;
}

int GetScaledTitleBarHeight() {
    const int referenceHeight = 1080;
    const int referenceTitleBarHeight = 40; 

    HDC screen = GetDC(0);
    int currentHeight = GetDeviceCaps(screen, VERTRES);
    ReleaseDC(0, screen);

    float scaleFactor = (float)currentHeight / referenceHeight;

    return (int)(referenceTitleBarHeight * scaleFactor);
}

void ConstrainCursorToRoblox()
{
    if (!robloxWindow)
        return;

    RECT clientRect;
    if (GetClientRect(robloxWindow, &clientRect))
    {
        POINT topLeft = {clientRect.left, clientRect.top};
        POINT bottomRight = {clientRect.right, clientRect.bottom};

        ClientToScreen(robloxWindow, &topLeft);
        ClientToScreen(robloxWindow, &bottomRight);

        int titleBarHeight = GetScaledTitleBarHeight();
        topLeft.y += titleBarHeight;

        RECT screenRect = {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
        ClipCursor(&screenRect);
    }
}

void FreeCursor()
{
    ClipCursor(NULL);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    std::cout << "Mouse Hook triggered. Event: " << wParam << std::endl;
    if (nCode == HC_ACTION)
    {
        if (IsRobloxActive())
        {
            switch (wParam)
            {
            case WM_RBUTTONDOWN:
                mouseSaved = true;
                GetCursorPos(&savedPoint);
                std::cout << "Mouse position saved: (" << savedPoint.x << ", " << savedPoint.y << ")\n";
                break;

            case WM_RBUTTONUP:
                if (mouseSaved)
                {
                    SetCursorPos(savedPoint.x, savedPoint.y);
                    std::cout << "Mouse position reset to saved position.\n";
                    mouseSaved = false;
                }
                break;

            case WM_MOUSEMOVE:
                ConstrainCursorToRoblox();
                break;
            }
        }
        else
        {
            FreeCursor();
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP)
        {
            POINT curPoint;
            GetCursorPos(&curPoint);
            SetForegroundWindow(hwnd);

            HMENU hMenu = CreatePopupMenu();
            if (hMenu)
            {
                InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_EXIT_CONTEXT_MENU_ITEM, "Quit");

                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, curPoint.x, curPoint.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
            }
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TRAY_EXIT_CONTEXT_MENU_ITEM)
        {
            PostQuitMessage(0);
        }
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int cmdShow)
{
    const char CLASS_NAME[] = "Sample Window Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Sample Window Name",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hwnd == NULL)
    {
        return 0;
    }

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy(nid.szTip, "UWPFixer - by Osiris");

    Shell_NotifyIcon(NIM_ADD, &nid);

    HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
    if (!mouseHook)
    {
        return 1;
    }

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIcon(NIM_DELETE, &nid);
    UnhookWindowsHookEx(mouseHook);

    return 0;
}
