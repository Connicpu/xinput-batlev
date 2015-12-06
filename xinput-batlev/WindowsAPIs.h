#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <shellapi.h>
#include <dxgi1_2.h>
#include <d2d1_2.h>
#include <d2d1_2helper.h>
#include <dwrite.h>
#include <Xinput.h>
#include <atlbase.h>
#include <wincodec.h>

template <typename T>
using ComPtr = ATL::CComPtr<T>;
using Bstr = ATL::CComBSTR;

extern HINSTANCE hinstance;

inline void CheckHR(HRESULT hr)
{
    if (FAILED(hr))
    {
        if (IsDebuggerPresent())
        {
            DebugBreak();
        }
        else
        {
            abort();
        }
    }
}

inline void CheckLastError()
{
    CheckHR(HRESULT_FROM_WIN32(GetLastError()));
}
