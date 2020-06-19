#pragma once

#include "window.h"

struct CustomChild : Window<CustomChild> {
private:
    inline static const std::wstring class_name{ L"App.Custom.Child" };
    std::wstring text{L"Click the button to change the text."};

public:
    CustomChild();

    void SetText(std::wstring t);

    HWND Create(LPCWSTR lpWindowName, HWND parent, int X = 0, int Y = 0,
        int nWidth = 0, int nHeight = 0, DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        DWORD dwExStyle = 0);
};