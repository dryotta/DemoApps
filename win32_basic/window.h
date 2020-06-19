#pragma once

// Lambda based message handling inspired by https://github.com/rodrigocfd/winlamb

template <typename T>
struct Window
{
    HWND m_win = nullptr;
    std::map<UINT, std::function<LRESULT(WPARAM wparam, LPARAM )>> msg_map;

    // Important to set up message handlers before message loop starts!
    void on_msg(UINT msg, std::function<LRESULT(WPARAM wparam, LPARAM lparam)> func) {
        msg_map.insert(std::make_pair(msg, std::move(func)));
    }

    static T * GetThisFromHandle(HWND const window) {
        return reinterpret_cast<T *>(GetWindowLongPtr(window, GWLP_USERDATA));
    }

    static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) {
        if (WM_NCCREATE == message) {
            CREATESTRUCT * cs = reinterpret_cast<CREATESTRUCT *>(lparam);
            T * that = static_cast<T *>(cs->lpCreateParams);
            that->m_win = window;

            SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
        } else if (T * that = GetThisFromHandle(window))
        {
            auto res = that->msg_map.find(message);
            if (res != that->msg_map.end()) {
                return res->second(wparam, lparam);
            }
        }

        return DefWindowProc(window, message, wparam, lparam);
    }
};
