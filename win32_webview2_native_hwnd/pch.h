#pragma once

#include <map>
#include <functional>
#include <format>

#include <windows.h>

#include <wil/com.h>

#include <winrt/Windows.Foundation.h>
#pragma comment(lib, "windowsapp")
using namespace winrt;

#include <wrl.h>
#include "WebView2.h"
using namespace Microsoft::WRL;

#include <shellscalingapi.h> // for DPI awareness
#pragma comment(lib, "shcore")