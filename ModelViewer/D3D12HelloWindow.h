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

#include "DXSample.h"

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class D3D12HelloWindow : public DXSample
{
public: D3D12HelloWindow(UINT width, UINT height, std::wstring name);

public: virtual void OnInit();
public: virtual void OnUpdate();
public: virtual void OnRender();
public: virtual void OnDestroy();

private: static const UINT FrameCount = 2;

    // Pipeline objects.
private: ComPtr<IDXGISwapChain3> _swapChain;
private: ComPtr<ID3D12Device> _device;

private: ComPtr<ID3D12Resource> _renderTargets[FrameCount];
private: ComPtr<ID3D12Resource> _depthStencil;

private: ComPtr<ID3D12CommandAllocator> _commandAllocator;
private: ComPtr<ID3D12CommandQueue> _commandQueue;

private: ComPtr<ID3D12DescriptorHeap> _rtvHeap;
private: ComPtr<ID3D12DescriptorHeap> _dsvHeap;

private: ComPtr<ID3D12PipelineState> _pipelineState;
private: ComPtr<ID3D12GraphicsCommandList> _commandList;

private: ComPtr<ID3D12RootSignature> _rootSignature;

private: UINT _rtvDescriptorSize;
private: UINT _dsvDescriptorSize;

    // Synchronization objects.
private: UINT _frameIndex;
private: HANDLE _fenceEvent;
private: ComPtr<ID3D12Fence> _fence;
private: UINT64 _fenceValue;


private: void LoadPipeline();

private: void LoadPipelineRTV();
private: void LoadPipelineDSV();

private: void LoadAssets();


private: void PopulateCommandList();
private: void WaitForPreviousFrame();
};
