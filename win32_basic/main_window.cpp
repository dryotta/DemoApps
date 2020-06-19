#include "pch.h"
#include "utils.h"

#include "main_window.h"
#include "resource.h"

static LONG const DefWindowWidth = 600;
static LONG const DefWindowHeight = 400;
static int Margin = 20;
static int ButtonHeight = 80;

MainWindow::MainWindow() {
    WNDCLASS wc = {};
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = class_name.c_str();
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));

    RegisterClass(&wc);

    SetupMessageHandlers(); // important to set up message handlers before creating the window

    VERIFY(Create(L"App Main Window"));
}

void MainWindow::SetupMessageHandlers() {
    On(WM_CREATE, [this](WPARAM, LPARAM) -> LRESULT {
        RECT rect = {0, 0, DefWindowWidth, DefWindowHeight};
        VERIFY(AdjustWindowRect(&rect, GetWindowLong(hwnd, GWL_STYLE), false));
        VERIFY(SetWindowPos(hwnd, nullptr, 0, 0,
            rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER));

        TRACE(L"Adjusted %d %d %d %d\n", rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top);

        VERIFY(GetClientRect(hwnd, &rect));
        button = CreateWindow(L"BUTTON", L"Click me", 
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_NOTIFY,
            0,0,0,0, hwnd, (HMENU)ID_BUTTON, GetModuleHandle(nullptr), nullptr);

        custom.Create(L"Hello", hwnd);

        ResizeChildWindows();

        return 0;
    });

    On(ID_BUTTON, [this]() {
        custom.SetText(L"The button was clicked");
    });

    On(ID_BUTTON, BN_KILLFOCUS, [this]() {
        custom.SetText(L"The button lost focus");
    });

    On(WM_DESTROY, [this](WPARAM, LPARAM) -> LRESULT {
        PostQuitMessage(0);
        return 0;
    });

    On(WM_PAINT, [this](WPARAM, LPARAM) -> LRESULT {
        RECT rect = {};
        VERIFY(GetClientRect(hwnd, &rect));

        
        VERIFY(ValidateRect(hwnd, nullptr));
        return 0;
    });

    On(WM_SIZE, [this](WPARAM, LPARAM) -> LRESULT {
        ResizeChildWindows();
        return 0;
    });
}

HWND MainWindow::Create(LPCWSTR lpWindowName) {
    hwnd = CreateWindowEx(0, class_name.c_str(), lpWindowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, GetModuleHandle(nullptr), this);
    VERIFY(hwnd);

    return hwnd;
}

void MainWindow::ResizeChildWindows() {
    RECT rect = {};
    VERIFY(GetClientRect(hwnd, &rect));

    if (button) {
        VERIFY(SetWindowPos(button, nullptr, Margin, Margin, ButtonHeight * 2, ButtonHeight, SWP_NOZORDER));
    }
    
    if (custom.hwnd) {
        VERIFY(SetWindowPos(custom.hwnd, nullptr, Margin, Margin * 2 + ButtonHeight, (rect.right - rect.left) - Margin * 2, (rect.bottom - rect.top) - Margin * 3 - ButtonHeight, SWP_NOZORDER));
    }
}
