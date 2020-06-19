#pragma once

struct MainWindow : Window<MainWindow>
{
    float m_dpiX = 0.0f;
    float m_dpiY = 0.0f;

    MainWindow();

    void SetupMessageHandlers();

    void CreateDesktopWindow();
    void CreateBrowserWindow();
    
};