#pragma once

#include "window.h"
#include "custom_child.h"

struct MainWindow : Window<MainWindow>
{
private:
    inline static const std::wstring class_name{ L"DemoApp.Main.Window" };

    HWND button = nullptr;
    CustomChild custom;

    void SetupMessageHandlers();

    HWND Create(LPCWSTR lpWindowName);
    void ResizeChildWindows();
public:
    MainWindow();
};