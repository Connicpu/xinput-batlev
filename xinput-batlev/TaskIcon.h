#pragma once

#include "WindowsAPIs.h"
#include "Icon.h"
#include <atomic>

#define WM_TASKICON (WM_USER + 1)

class TaskIcon
{
public:
    TaskIcon();
    ~TaskIcon();

    void MessageLoop();
    void Stop();

private:
    static LRESULT WINAPI StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

    void UpdateIcon();
    HICON MakeIcon(bool redraw);

    void RegClass();
    void UnregClass();

    ComPtr<IDWriteFactory> dwrite_;
    ComPtr<IDWriteTextFormat> font_;

    HWND hwnd_ = nullptr;
    HMENU menu_ = nullptr;
    Icon icon_;
    float batlevel_ = 0;
    bool nocontroller_ = true;
    bool unknown_ = false;
    std::atomic<bool> stop_ = false;
};
