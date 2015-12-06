#include "Icon.h"

Icon::Icon(UINT width, UINT height, float scale)
{
    CoInitializeEx(0, COINIT_SPEED_OVER_MEMORY | COINIT_MULTITHREADED);

    CheckHR(CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wic_factory_)
        ));

    CheckHR(wic_factory_->CreateBitmap(
        width,
        height,
        GUID_WICPixelFormat32bppPBGRA,
        WICBitmapCacheOnDemand,
        &wic_bitmap_
        ));

    CheckHR(D2D1CreateFactory(
        D2D1_FACTORY_TYPE_MULTI_THREADED,
        &d2d1_factory_
        ));

    CheckHR(d2d1_factory_->CreateWicBitmapRenderTarget(
        wic_bitmap_,
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(),
            96.f * scale, 96.f * scale
            ),
        &d2d1_target_
        ));

    icon_mask_ = CreateBitmap(width, height, 1, 1, nullptr);
    if (!icon_mask_) CheckLastError();
}

Icon::~Icon()
{
    if (icon_)
        DestroyIcon(icon_);
    if (icon_bitmap_)
        DeleteObject(icon_bitmap_);
    if (icon_mask_)
        DeleteObject(icon_mask_);
    d2d1_target_.Release();
    d2d1_factory_.Release();
    wic_bitmap_.Release();
    wic_factory_.Release();

    CoUninitialize();
}

void Icon::Lock(ID2D1RenderTarget **target)
{
    AcquireSRWLockExclusive(&lock_);
    *target = d2d1_target_;
    (*target)->AddRef();
}

void Icon::Unlock()
{
    // Free old stuff
    if (icon_)
        DestroyIcon(icon_);
    if (icon_bitmap_)
        DeleteObject(icon_bitmap_);

    // Get the dimensions
    UINT width, height;
    CheckHR(wic_bitmap_->GetSize(&width, &height));

    // Lock the bitmap for reading
    ComPtr<IWICBitmapLock> lock;
    WICRect rect = { 0, 0, (INT)width, (INT)height };
    CheckHR(wic_bitmap_->Lock(&rect, WICBitmapLockRead, &lock));

    // Create a bitmap from the data
    UINT bufsize;
    WICInProcPointer buf;
    lock->GetDataPointer(&bufsize, &buf);
    icon_bitmap_ = CreateBitmap(width, height, 1, 32, buf);
    if (!icon_bitmap_) CheckLastError();

    // Create an icon from the bitmap
    ICONINFO info = { 0 };
    info.fIcon = true;
    info.hbmMask = icon_mask_;
    info.hbmColor = icon_bitmap_;
    icon_ = CreateIconIndirect(&info);
    if (!icon_) CheckLastError();

    // Finish unlocking
    ReleaseSRWLockExclusive(&lock_);
}

HICON Icon::GetIcon()
{
    return icon_;
}
