#include "pch.h"
#include "utils.h"
#include "custom_child.h"

CustomChild::CustomChild() {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));
    wc.lpszClassName = CustomChild::class_name.c_str();

    VERIFY(RegisterClassEx(&wc));

    On(WM_PAINT, [this](WPARAM, LPARAM) -> LRESULT {
        HDC hdc;
        PAINTSTRUCT ps;

        hdc = BeginPaint(hwnd, &ps);

        RECT rcClient;
        VERIFY(GetClientRect(hwnd, &rcClient));

        // Drawtext on the control window.
        HFONT font = CreateFontW(40, 0, 0, 0, 0, FALSE, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI Light");    // Instruction Font and Size (Same as Description's)
        if (font) {
            HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, font));

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));

            DrawText(hdc, text.c_str(), -1, &rcClient, DT_WORDBREAK | DT_CENTER);

            SelectObject(hdc, hOldFont);

            DeleteObject(font);
        }
        EndPaint(hwnd, &ps);

        return 0;
    });
}

void CustomChild::SetText(std::wstring t)
{
    text = t;

    InvalidateRect(hwnd, nullptr, true);
}

HWND CustomChild::Create(LPCWSTR lpWindowName, HWND parent, int X, int Y, int nWidth, int nHeight, DWORD dwStyle, DWORD dwExStyle) {
    hwnd = CreateWindowExW(dwExStyle, CustomChild::class_name.c_str(), lpWindowName, dwStyle, X, Y, nWidth, nHeight,
        parent, nullptr, (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE), this);

    VERIFY(hwnd);

    return hwnd;
}
