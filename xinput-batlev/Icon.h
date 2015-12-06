#pragma once

#include "WindowsAPIs.h"

class Icon
{
public:
    Icon(UINT width, UINT height, float scale);
    ~Icon();

    void Lock(ID2D1RenderTarget **target);
    void Unlock();

    HICON GetIcon();

private:
    ComPtr<IWICImagingFactory2> wic_factory_;
    ComPtr<IWICBitmap> wic_bitmap_;
    ComPtr<ID2D1Factory1> d2d1_factory_;
    ComPtr<ID2D1RenderTarget> d2d1_target_;

    HBITMAP icon_mask_ = nullptr;
    HBITMAP icon_bitmap_ = nullptr;
    HICON icon_ = nullptr;
    SRWLOCK lock_ = { SRWLOCK_INIT };
};
