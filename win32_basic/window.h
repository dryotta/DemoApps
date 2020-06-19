#pragma once

// Lambda based message handling inspired by https://github.com/rodrigocfd/winlamb

template <typename T>
struct Window {
private:
    std::map<UINT, std::function<LRESULT(WPARAM wparam, LPARAM)>> msg_map; // map of message handlers
    std::map<UINT, std::function<void()>> cmd_map; // map of WM_COMMAND handlers
    std::map<std::pair<UINT_PTR, UINT>, std::function<void()>> ntf_map; // map of WM_NOTIFY handlers, pair of idFrom and code

public:
    HWND hwnd = nullptr;    

    // Important to set up message handlers before message loop starts!
    void On(UINT msg, std::function<LRESULT(WPARAM wparam, LPARAM lparam)> func) {
        msg_map.insert(std::make_pair(msg, std::move(func)));
    }

    void On(WORD cmd, std::function<void()> func) {
        cmd_map.insert(std::make_pair(cmd, std::move(func)));
    }

    void On(UINT idfrom, UINT code, std::function<void()> func) {
        ntf_map.insert(std::make_pair(std::make_pair(idfrom, code), std::move(func)));
    }
    
    static T * GetThisFromHwnd(HWND const window) {
        return reinterpret_cast<T *>(GetWindowLongPtr(window, GWLP_USERDATA));
    }

    static LRESULT __stdcall WndProc(HWND const window, UINT const message, WPARAM const wparam, LPARAM const lparam) {
        if (WM_NCCREATE == message) {
            CREATESTRUCT * cs = reinterpret_cast<CREATESTRUCT *>(lparam);
            T * that = static_cast<T *>(cs->lpCreateParams);
            that->hwnd = window;

            SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
        } else if (T* that = GetThisFromHwnd(window))
        {
			if (WM_COMMAND == message) {
				auto res = that->cmd_map.find(LOWORD(wparam));
				if (res != that->cmd_map.end()) {
					res->second();
                    return 0;
				}
			} else if (WM_NOTIFY == message) {
                NMHDR* mnh = reinterpret_cast<NMHDR*>(lparam);
				auto res = that->ntf_map.find(std::make_pair(mnh->idFrom, mnh->code));
				if (res != that->ntf_map.end()) {
					res->second();
                    return 0;
				}
            } else {
                auto res = that->msg_map.find(message);
                if (res != that->msg_map.end()) {
                    return res->second(wparam, lparam);
                }
            }

            
        }

        return DefWindowProc(window, message, wparam, lparam);
    }
};