#include "pch.h"
#include "utils.h"

#include <wrl.h>
#include "WebView2.h"
using namespace Microsoft::WRL;

#include "window.h"
#include "main_window.h"

#include "resource.h"

extern "C" IMAGE_DOS_HEADER __ImageBase; // for hinstance

static float const DefWindowWidth = 600.0f;
static float const DefWindowHeight = 400.0f;

static com_ptr<ICoreWebView2Controller> webviewController;
static com_ptr<ICoreWebView2> webviewWindow;

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

void MainWindow::CreateBrowserWindow() {
    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

        // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
        env->CreateCoreWebView2Controller(this->m_win, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
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
            GetClientRect(this->m_win, &bounds);
            webviewController->put_Bounds(bounds);

            // Schedule an async task to navigate to Bing
            webviewWindow->Navigate(L"https://www.youtube.com/");

            // Step 4 - Navigation events
            // register an ICoreWebView2NavigationStartingEventHandler to cancel any non-https navigation
            EventRegistrationToken token;
            webviewWindow->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                [](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                PWSTR uri;
                args->get_Uri(&uri);
                std::wstring source(uri);
                if (source.substr(0, 5) != L"https") {
                    args->put_Cancel(true);
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
                [](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                PWSTR message;
                args->TryGetWebMessageAsString(&message);
                // processMessage(&message);
                TRACE(L"Web Message received %s\n", message);
                webview->PostWebMessageAsString(message);
                CoTaskMemFree(message);
                return S_OK;
            }).Get(), &token);

            // Schedule an async task to add initialization script that
            // 1) Add an listener to print message from the host
            // 2) Post document URL to the host
            webviewWindow->AddScriptToExecuteOnDocumentCreated(
                L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
                L"window.chrome.webview.postMessage(window.document.URL);",
                nullptr);


            return S_OK;
        }).Get());
        return S_OK;
    }).Get());

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

        CreateBrowserWindow();

        return 0;
    });

    on_msg(WM_DESTROY, [this](WPARAM, LPARAM) -> LRESULT {
        PostQuitMessage(0);
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

    on_msg(WM_PAINT, [this](WPARAM, LPARAM) -> LRESULT {
        RECT rect = {};
        VERIFY(GetClientRect(m_win, &rect));

        TRACE(L"Client size %.2f %.2f\n",
            PhysicalToLogical(rect.right, m_dpiX),
            PhysicalToLogical(rect.bottom, m_dpiY));

        VERIFY(ValidateRect(m_win, nullptr));
        return 0;
    });

    on_msg(WM_SIZE, [this](WPARAM, LPARAM) -> LRESULT {
        if (webviewController != nullptr) {
            RECT bounds;
            GetClientRect(this->m_win, &bounds);
            webviewController->put_Bounds(bounds);
        };

        return 0;
    });
}