#include "pch.h"
#include "utils.h"

#include "window.h"
#include "main_window.h"

#include "resource.h"

extern "C" IMAGE_DOS_HEADER __ImageBase; // for hinstance

static float const DefWindowWidth = 600.0f;
static float const DefWindowHeight = 400.0f;

MainWindow::MainWindow()
{
    SetupMessageHandlers(); // important to set up message handlers before creating the window

    CreateDesktopWindow();
}

void MainWindow::CreateDesktopWindow()
{
    WNDCLASS wc = {};
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);
    wc.lpszClassName = L"App.MainWindow";
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_APP));
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));

    RegisterClass(&wc);

    ASSERT(!m_win);
    VERIFY(CreateWindowEx(0,
        wc.lpszClassName,
        L"App Main Window",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        wc.hInstance,
        this));
    ASSERT(m_win);
}

void MainWindow::SetupMessageHandlers() {
    on_msg(WM_CREATE, [this](WPARAM, LPARAM) -> LRESULT {
        HMONITOR const monitor = MonitorFromWindow(m_win, MONITOR_DEFAULTTONEAREST);

        unsigned dpiX = 0;
        unsigned dpiY = 0;

        check_hresult(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY));

        m_dpiX = static_cast<float>(dpiX);
        m_dpiY = static_cast<float>(dpiY);

        TRACE(L"DPI %.2f %.2f\n", m_dpiX, m_dpiY);

        RECT rect =
        {
            0, 0,
            static_cast<LONG>(LogicalToPhysical(DefWindowWidth, m_dpiX)),
            static_cast<LONG>(LogicalToPhysical(DefWindowHeight, m_dpiY))
        };

        VERIFY(AdjustWindowRect(&rect, GetWindowLong(m_win, GWL_STYLE), false));

        VERIFY(SetWindowPos(m_win, nullptr, 0, 0,
            rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER));

        TRACE(L"Adjusted %d %d %d %d\n",
            rect.left, rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top);

        return 0;
    });

    on_msg(WM_DESTROY, [this](WPARAM, LPARAM) -> LRESULT {
        PostQuitMessage(0);
        return 0;
    });

    on_msg(WM_PAINT, [this](WPARAM, LPARAM) -> LRESULT {
        RECT rect = {};
        VERIFY(GetClientRect(m_win, &rect));

        TRACE(L"Client size %.2f %.2f\n",
            PhysicalToLogical(rect.right, m_dpiX),
            PhysicalToLogical(rect.bottom, m_dpiY));

        VERIFY(ValidateRect(m_win, nullptr));
        return 0;
    });

    on_msg(WM_DPICHANGED, [this](WPARAM w, LPARAM l) -> LRESULT {
        m_dpiX = LOWORD(w);
        m_dpiY = HIWORD(w);

        TRACE(L"DPI %.2f %.2f\n", m_dpiX, m_dpiY);

        RECT const* suggested = reinterpret_cast<RECT const*>(l);
        VERIFY(SetWindowPos(m_win,
            nullptr,
            suggested->left,
            suggested->top,
            suggested->right - suggested->left,
            suggested->bottom - suggested->top,
            SWP_NOACTIVATE | SWP_NOZORDER));
        return 0;
    });
}