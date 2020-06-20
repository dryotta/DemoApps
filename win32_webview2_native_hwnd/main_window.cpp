#include "pch.h"
#include "utils.h"

#include <wrl.h>
#include "WebView2.h"
using namespace Microsoft::WRL;

#include "window.h"
#include "main_window.h"

#include "resource.h"

extern "C" IMAGE_DOS_HEADER __ImageBase; // for hinstance

static LONG const DefWindowWidth = 600;
static LONG const DefWindowHeight = 400;
static int Margin = 20;
static int ButtonHeight = 80;

static com_ptr<ICoreWebView2Controller> webviewController;
static com_ptr<ICoreWebView2> webviewWindow;

MainWindow::MainWindow()
{
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

HWND MainWindow::Create(LPCWSTR lpWindowName) {
    hwnd = CreateWindowEx(0, class_name.c_str(), lpWindowName, WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr, nullptr, GetModuleHandle(nullptr), this);
    VERIFY(hwnd);

    return hwnd;
}

void MainWindow::ResizeChildWindows() {

    RECT rect = {};
    VERIFY(GetClientRect(hwnd, &rect));

    if (webviewController != nullptr) {
        webviewController->put_Bounds(rect);
    };

    HWND hwndChrome = FindWindowEx(hwnd, NULL, L"Chrome_WidgetWin_0", NULL);
    if (hwndChrome) {
        // ensure it is at bottom
        VERIFY(SetWindowPos(hwndChrome, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE));
    }
    

    if (button) {
        VERIFY(SetWindowPos(button, nullptr, Margin, Margin, ButtonHeight * 2, ButtonHeight, SWP_NOZORDER));
    }

    if (custom.hwnd) {
        VERIFY(SetWindowPos(custom.hwnd, nullptr, Margin, Margin * 2 + ButtonHeight, (rect.right - rect.left) - Margin * 2, (rect.bottom - rect.top) - Margin * 3 - ButtonHeight, SWP_NOZORDER));
    }
}

void MainWindow::CreateBrowserWindow() {
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

        // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
        env->CreateCoreWebView2Controller(hwnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
            if (controller != nullptr) {
                webviewController.copy_from(controller);
                webviewController->get_CoreWebView2(webviewWindow.put());
            }

            // Add a few settings for the webview
            // this is a redundant demo step as they are the default settings values
            ICoreWebView2Settings* Settings;
            webviewWindow->get_Settings(&Settings);
            Settings->put_IsScriptEnabled(TRUE);
            Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            Settings->put_IsWebMessageEnabled(TRUE);

            // Resize WebView to fit the bounds of the parent window
            RECT bounds;
            GetClientRect(this->hwnd, &bounds);
            webviewController->put_Bounds(bounds);

            // Schedule an async task to navigate to Bing
            webviewWindow->Navigate(L"https://www.bing.com");

            // Step 4 - Navigation events
            // register an ICoreWebView2NavigationStartingEventHandler to cancel any non-https navigation
            EventRegistrationToken token;
            webviewWindow->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                [](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                PWSTR uri;
                args->get_Uri(&uri);
                std::wstring source(uri);
                if (source.substr(0, 5) != L"https") {
                    //args->put_Cancel(true);
                }
                CoTaskMemFree(uri);
                return S_OK;
            }).Get(), &token);

            // Step 5 - Scripting

            // Schedule an async task to add initialization script that freezes the Object object
            webviewWindow->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
            // Schedule an async task to get the document URL
            webviewWindow->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
                LPCWSTR URL = resultObjectAsJson;
                TRACE(L"URL is %s\n", URL);
                return S_OK;
            }).Get());


            // Step 6 - Communication between host and web content
            // Set an event handler for the host to return received message back to the web content
            webviewWindow->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                PWSTR message;
                args->TryGetWebMessageAsString(&message);
                // processMessage(&message);
                TRACE(L"Web Message received %s\n", message);

                // add WS_CLIPSIBLINGS style to ensure native controls to be visible
                HWND hwndChrome = FindWindowEx(hwnd, NULL, L"Chrome_WidgetWin_0", NULL);
                if (hwndChrome) {
                    LONG style = GetWindowLong(hwndChrome, GWL_STYLE);
                    style |= WS_CLIPSIBLINGS;
                    SetWindowLong(hwndChrome, GWL_STYLE, style);
                    VERIFY(SetWindowPos(hwndChrome, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED));
                }

                // Page have been loaded, create child controls
                button = CreateWindow(L"BUTTON", L"Click me",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_NOTIFY,
                    0, 0, 0, 0, hwnd, (HMENU)ID_BUTTON, GetModuleHandle(nullptr), nullptr);

                custom.Create(L"Hello", hwnd);

                ResizeChildWindows();

                webview->PostWebMessageAsString(message);
                CoTaskMemFree(message);
                return S_OK;
            }).Get(), &token);

            // Schedule an async task to add initialization script that
            // 1) Add an listener to print message from the host
            // 2) Post document URL to the host
            webviewWindow->AddScriptToExecuteOnDocumentCreated(
                L"window.chrome.webview.postMessage(window.document.URL);",
                nullptr);


            return S_OK;
        }).Get());
        return S_OK;
    }).Get());

}

void MainWindow::SetupMessageHandlers() {
    On(WM_CREATE, [this](WPARAM, LPARAM) -> LRESULT {
        RECT rect = { 0, 0, DefWindowWidth, DefWindowHeight };
        VERIFY(AdjustWindowRect(&rect, GetWindowLong(hwnd, GWL_STYLE), false));
        VERIFY(SetWindowPos(hwnd, nullptr, 0, 0,
            rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER));

        TRACE(L"Adjusted %d %d %d %d\n", rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top);
        VERIFY(GetClientRect(hwnd, &rect));

        CreateBrowserWindow();

        return 0;
    });

    On(ID_BUTTON, [this]() {
        custom.SetText(L"The button was clicked");
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
