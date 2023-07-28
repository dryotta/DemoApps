#include "pch.h"
#include "window.h"
#include "main_window.h"
#include "tictoc.h"

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    
    MainWindow window(Tic());
    
    MSG message;
    while (GetMessage(&message, nullptr, 0, 0)) {
        DispatchMessage(&message);
    }
}
