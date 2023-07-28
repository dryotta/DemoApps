#pragma once

#include "window.h"
#include "custom_child.h"

struct ICoreWebView2;

struct MainWindow : Window<MainWindow>
{
private:
    inline static const std::wstring class_name{ L"DemoApp.Main.Window" };

    LONGLONG tic;

    HWND button = nullptr;
    CustomChild custom;

    void SetupMessageHandlers();

    HWND Create(LPCWSTR lpWindowName);
    void CreateBrowserWindow();
    void ResizeChildWindows();

    void ProcessWebMessage(ICoreWebView2* sender, std::wstring);

    std::wstring GetLocalPath(std::wstring path);
    std::wstring GetLocalUri(std::wstring path);
public:
    MainWindow(LONGLONG tic);
};