//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "DXSampleHelper.h"
#include "Win64Application.h"

class DXSample
{
public:
    DXSample(UINT width, UINT height, std::wstring name);
    virtual ~DXSample();

    virtual void OnInit() = 0;
    virtual void OnUpdate() = 0;
    virtual void OnRender() = 0;
    virtual void OnDestroy() = 0;

    // Samples override the event handlers to handle specific messages.
    virtual void OnKeyDown(UINT8 /*key*/) {}
    virtual void OnKeyUp(UINT8 /*key*/) {}

    // Accessors.
    UINT GetWidth() const { return _width; }
    UINT GetHeight() const { return _height; }
    const WCHAR* GetTitle() const { return _title.c_str(); }

protected:
    std::wstring GetAssetFullPath(LPCWSTR assetName);

    void GetHardwareAdapter(
        _In_ IDXGIFactory1* pFactory,
        _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter = false);

    void SetCustomWindowText(LPCWSTR text);

    // Viewport dimensions.
    UINT _width;
    UINT _height;
    float _aspectRatio;

private:
    // Root assets path.
    std::wstring _assetsPath;

    // Window title.
    std::wstring _title;
};
