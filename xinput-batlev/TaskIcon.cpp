#include "TaskIcon.h"
#include <cmath>

const UINT icon_id = 100;
const UINT id_exit = 101;

static const wchar_t *const icon_label = L"Xbox Controller Battery Level";
static const wchar_t *const task_class = L"xinput-batlev_task-icon";
static const wchar_t icon_emoji[] = L"🎮";
static std::atomic<size_t> task_refcount = 0;

TaskIcon::TaskIcon()
    : icon_(128, 128, 2)
{
    RegClass();
    hwnd_ = CreateWindowExW(
        0, task_class, icon_label,
        0, 0, 0, 0, 0, nullptr,
        nullptr, hinstance, this
        );

    menu_ = CreatePopupMenu();
    if (!menu_) CheckLastError();

    MENUINFO menuinfo = { sizeof(menuinfo) };
    menuinfo.fMask = MIM_STYLE;
    menuinfo.dwStyle = MNS_AUTODISMISS;
    SetMenuInfo(menu_, &menuinfo);

    InsertMenuW(menu_, 0, MF_BYPOSITION, id_exit, L"Exit");

    CheckHR(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(*dwrite_),
        (IUnknown **)&dwrite_)
        );

    CheckHR(dwrite_->CreateTextFormat(
        L"Segoe UI Emoji",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        42,
        L"en-US",
        &font_
        ));

    NOTIFYICONDATAW idata = { sizeof(idata) };
    idata.hWnd = hwnd_;
    idata.uID = icon_id;
    idata.uVersion = NOTIFYICON_VERSION_4;
    idata.uCallbackMessage = WM_TASKICON;
    idata.hIcon = MakeIcon(false);
    idata.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    wcscpy_s(idata.szTip, icon_label);
    Shell_NotifyIconW(NIM_ADD, &idata);
}

TaskIcon::~TaskIcon()
{
    NOTIFYICONDATAW idata = { sizeof(idata) };
    idata.hWnd = hwnd_;
    idata.uID = icon_id;
    Shell_NotifyIconW(NIM_DELETE, &idata);

    if (menu_)
        DestroyMenu(menu_);
    if (hwnd_)
        DestroyWindow(hwnd_);
    UnregClass();
}

void TaskIcon::MessageLoop()
{
    // Recheck once every 5 seconds
    SetTimer(hwnd_, icon_id, 5 * 1000, nullptr);

    // Initial update
    UpdateIcon();

    MSG msg;
    while (GetMessageW(&msg, hwnd_, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (stop_)
        {
            break;
        }
    }
}

void TaskIcon::Stop()
{
    stop_ = true;
}

LRESULT TaskIcon::StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_CREATE)
    {
        auto cdata = (CREATESTRUCTW *)lparam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cdata->lpCreateParams);
        return 0;
    }

    auto task_ptr = (TaskIcon *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (task_ptr)
    {
        return task_ptr->WndProc(msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT TaskIcon::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_TIMER:
        {
            UpdateIcon();
            return 0;
        }
        case WM_TASKICON:
        {
            if (LOWORD(lparam) == WM_RBUTTONUP)
            {
                POINT cpos;
                GetCursorPos(&cpos);

                SetForegroundWindow(hwnd_);
                TrackPopupMenuEx(
                    menu_,
                    TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                    cpos.x, cpos.y,
                    hwnd_,
                    nullptr
                    );
            }
            return 0;
        }
        case WM_COMMAND:
        {
            if (wparam == id_exit)
            {
                stop_ = true;
            }
            return 0;
        }
    }

    return DefWindowProcW(hwnd_, msg, wparam, lparam);
}

void TaskIcon::UpdateIcon()
{
    XINPUT_BATTERY_INFORMATION info;
    if (XInputGetBatteryInformation(0, BATTERY_DEVTYPE_GAMEPAD, &info) != ERROR_SUCCESS)
    {
        info.BatteryType = BATTERY_TYPE_DISCONNECTED;
    }

    switch (info.BatteryType)
    {
        case BATTERY_TYPE_WIRED:
            batlevel_ = 1;
            nocontroller_ = false;
            unknown_ = false;
            break;

        case BATTERY_TYPE_ALKALINE:
        case BATTERY_TYPE_NIMH:
            switch (info.BatteryLevel)
            {
                case BATTERY_LEVEL_EMPTY:
                    batlevel_ = 0;
                    break;
                case BATTERY_LEVEL_LOW:
                    batlevel_ = 0.3f;
                    break;
                case BATTERY_LEVEL_MEDIUM:
                    batlevel_ = 0.6f;
                    break;
                case BATTERY_LEVEL_FULL:
                    batlevel_ = 1;
                    break;
            }
            nocontroller_ = false;
            unknown_ = false;
            break;

        case BATTERY_TYPE_UNKNOWN:
            batlevel_ = 0;
            nocontroller_ = false;
            unknown_ = true;

        case BATTERY_TYPE_DISCONNECTED:
        default:
            batlevel_ = 0;
            nocontroller_ = true;
            unknown_ = false;
            break;
    }

    NOTIFYICONDATAW idata = { sizeof(idata) };
    idata.hWnd = hwnd_;
    idata.uID = icon_id;
    idata.hIcon = MakeIcon(true);
    idata.uFlags = NIF_ICON;
    Shell_NotifyIconW(NIM_MODIFY, &idata);
}

HICON TaskIcon::MakeIcon(bool redraw)
{
    auto icon = icon_.GetIcon();
    if (!icon || redraw)
    {
        ComPtr<ID2D1RenderTarget> target;
        icon_.Lock(&target);

        target->BeginDraw();
        target->Clear();

        auto size = target->GetSize();
        auto batrect = D2D1::RectF(5, size.height / 2 - 16, size.width - 8, size.height / 2 + 16);
        auto posend = D2D1::RectF(size.width - 8, size.height / 2 - 8, size.width, size.height / 2 + 8);
        auto fillrect = D2D1::RectF(batrect.left + 5, batrect.top + 5, batrect.right - 5, batrect.bottom - 5);
        fillrect.right = (1 - batlevel_) * fillrect.left + batlevel_ * fillrect.right;

        ComPtr<ID2D1SolidColorBrush> brush;
        CheckHR(target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush));

        target->DrawRectangle(batrect, brush, 5);
        target->FillRectangle(posend, brush);
        target->FillRectangle(fillrect, brush);

        target->DrawText(
            icon_emoji,
            ARRAYSIZE(icon_emoji) - 1,
            font_,
            D2D1::RectF(
                size.width / 3,
                size.height / 4.f,
                size.width,
                size.height
                ),
            brush,
            D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT
            );

        if (nocontroller_)
        {
            ComPtr<ID2D1SolidColorBrush> errbrush;
            CheckHR(target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &errbrush));
            auto center = D2D1::Point2F(16, size.height / 2 + 16);
            auto radiusx = std::sqrtf(2) * 7;
            auto tl = D2D1::Point2F(center.x - radiusx, center.y - radiusx);
            auto tr = D2D1::Point2F(center.x + radiusx, center.y - radiusx);
            auto bl = D2D1::Point2F(center.x - radiusx, center.y + radiusx);
            auto br = D2D1::Point2F(center.x + radiusx, center.y + radiusx);

            target->FillEllipse(
                D2D1::Ellipse(
                    center,
                    16, 16
                    ),
                errbrush
                );
            target->DrawLine(tl, br, brush, 5);
            target->DrawLine(tr, bl, brush, 5);
        }

        CheckHR(target->EndDraw());

        icon_.Unlock();
        icon = icon_.GetIcon();
    }
    return icon;
}

void TaskIcon::RegClass()
{
    if (task_refcount++)
        return;

    WNDCLASSEXW wndc = { sizeof(wndc) };
    wndc.cbWndExtra = sizeof(TaskIcon *);
    wndc.hInstance = hinstance;
    wndc.lpfnWndProc = TaskIcon::StaticWndProc;
    wndc.lpszClassName = task_class;
    RegisterClassExW(&wndc);
}

void TaskIcon::UnregClass()
{
    if (--task_refcount)
        return;

    UnregisterClassW(task_class, hinstance);
}
