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

#include "stdafx.h"
#include "D3D12HelloWindow.h"

D3D12HelloWindow::D3D12HelloWindow(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    _frameIndex(0),
    _rtvDescriptorSize(0),
    _fenceValue(0),
    _fenceEvent(nullptr)
{
}

void D3D12HelloWindow::OnInit()
{
    LoadPipeline();
    LoadAssets();
}

// Load the rendering pipeline dependencies.
void D3D12HelloWindow::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0; // 0x01 : DXGI_CREATE_FACTORY_DEBUG

    // DXGIfactory를 만든다.
    ComPtr<IDXGIFactory4> factory;
    // IDXGIFactory2 오브젝트를 생성한다. IDXGIFactory2 <- IDXGIFactory3 <- IDXGIFactory4
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    // only gpu renderer
    // DX12에 맞는 어댑터가 있는지 조회하고 생성한다.
    // 해당 어댑터의
    // D3D12Device를 생성한다. 
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0, // 최소 버전
            IID_PPV_ARGS(&_device)
        ));
    }
    
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};    
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;            // GPU 타임아웃(시간초과) 제한 활성 여부
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;            // GPU각 직접 실행할 수 있는 커멘드 버퍼로 지정한다. GPU State를 상속할 수 없다.
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;   // 커맨드 큐 우선순위
    queueDesc.NodeMask = 0;                                     // single gpu operation 또는 multip gpu 노드의 경우 해당 노드의 식별자를 넣는다.

    ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount; // 이중 버퍼의 경우 2개 사용
    swapChainDesc.Width = _width;
    swapChainDesc.Height = _height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        _commandQueue.Get(),            // Swap chain needs the queue so that it can force a flush on it.
        Win64Application::GetHwnd(),    // window 핸들 :: swap chaing의 출력창과 연결된 핸들
        &swapChainDesc,                 // 
        nullptr,                        // DXGI_SWAP_CHAIN_FULLSCREEN_DESC;
        nullptr,
        &swapChain                      // IDXGISwapChain1;
    ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win64Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&_swapChain)); // IDXGISwapChain1 -> IDXGISwapChain3

    _frameIndex = _swapChain->GetCurrentBackBufferIndex();

    LoadPipelineRTV();
    //LoadPipelineDSV();
    
    

    ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)));
}

void D3D12HelloWindow::LoadPipelineRTV()
{
    // Create descriptor heaps.
    // desciptor heap은 gpu가 리소스에 접근하는 방법을 제공한다.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0; // only one gpu
        ThrowIfFailed(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

        _rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            // swap_chain의 버퍼를 가지고 온다.
            ThrowIfFailed(_swapChain->GetBuffer(n, IID_PPV_ARGS(&_renderTargets[n])));
            // 해당 버퍼를 가지고 렌더 타겟 뷰를 만든다ㅣ
            _device->CreateRenderTargetView(
                _renderTargets[n].Get(),
                nullptr,
                rtvHandle);
            rtvHandle.Offset(1, _rtvDescriptorSize);
        }
    }
}

void D3D12HelloWindow::LoadPipelineDSV()
{
    // Create descripter heaps.
    {
        // Describe and create a depth stencil view (DSV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;
        ThrowIfFailed(_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap)));
    }

    // Create frame resource.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a DSV
        D3D12_RESOURCE_DESC dsvDesc = {};
        dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        dsvDesc.Alignment = 1;
        dsvDesc.DepthOrArraySize = 1;
        dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.MipLevels = 0;
        dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        dsvDesc.SampleDesc.Count = 1;
        dsvDesc.Width = _width;
        dsvDesc.Height = _height;

    }
}

// Load the sample assets.
void D3D12HelloWindow::LoadAssets()
{
    // Create an empty Root Signature
    {
        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(
            0u,                                                                 // num parameter
            nullptr,                                                            // pParameters
            0u,                                                                 // numStaticSamplers
            nullptr,                                                            // pStaticSamplers
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        ThrowIfFailed(D3D12SerializeVersionedRootSignature(
            &rootSignatureDesc,
            signature.GetAddressOf(),
            error.GetAddressOf()
        ));

        ThrowIfFailed(_device->CreateRootSignature(
            0,
            signature.Get(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(_rootSignature.GetAddressOf())
        ));
    }

    // Create the Pipeline State, whic includes compiling and loading shaders.
    {
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;

#endif
        ThrowIfFailed(D3DCompileFromFile(
            GetAssetFullPath(L"shaders.hlsl").c_str(),
            nullptr,
            nullptr, 
            "VSMain",
            "vs_5_0", 
            compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(
            GetAssetFullPath(L"shaders.hlsl").c_str(),
            nullptr,
            nullptr,
            "PSMain",
            "ps_5_0",
            compileFlags, 0, &pixelShader, nullptr));
    }

    // Create the command list.
    ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), nullptr, IID_PPV_ARGS(&_commandList)));
    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(_commandList->Close());

    {
        /*
        virtual HRESULT STDMETHODCALLTYPE CreateCommittedResource( 
            _In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
            D3D12_HEAP_FLAGS HeapFlags,
            _In_  const D3D12_RESOURCE_DESC *pDesc,
            D3D12_RESOURCE_STATES InitialResourceState,
            _In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
            REFIID riidResource,
            _COM_Outptr_opt_  void **ppvResource) = 0;
        */

        /*
          {
            D3D12_HEAP_TYPE Type;
            D3D12_CPU_PAGE_PROPERTY CPUPageProperty;
            D3D12_MEMORY_POOL MemoryPoolPreference;
            UINT CreationNodeMask;
            UINT VisibleNodeMask;
           } 	D3D12_HEAP_PROPERTIES;
        */
      

        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProperties.CreationNodeMask = 0; // 0 ==> 1
        heapProperties.VisibleNodeMask = 0; //  0 ==> 1

        // TODO
        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;// 
        resourceDesc.DepthOrArraySize = 3;

        _device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
            _vertexBuffer.GetAddressOf(),

        )
        // Create the vertex buffer
        {
            // Define t
        }

    }



    // Create synchronization objects.
    {
        ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
        _fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        _fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
}

// Update frame-based values.
void D3D12HelloWindow::OnUpdate()
{
}

// Render the scene.
void D3D12HelloWindow::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
    _commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(_swapChain->Present(1, 0));

    WaitForPreviousFrame();
}

void D3D12HelloWindow::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForPreviousFrame();

    CloseHandle(_fenceEvent);
}

void D3D12HelloWindow::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(_commandList->Reset(_commandAllocator.Get(), _pipelineState.Get()));


    // Indicate that the back buffer will be used as a render target.
    _commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));


    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), _frameIndex, _rtvDescriptorSize);

    // Record commands.
    const float clearColor[] = { 1.0f, 0.2f, 0.4f, 1.0f };
    _commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // Indicate that the back buffer will now be used to present.
    _commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


    ThrowIfFailed(_commandList->Close());
}

void D3D12HelloWindow::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = _fenceValue;
    ThrowIfFailed(_commandQueue->Signal(_fence.Get(), fence));
    _fenceValue++;

    // Wait until the previous frame is finished.
    if (_fence->GetCompletedValue() < fence)
    {
        ThrowIfFailed(
            _fence->SetEventOnCompletion(
                fence,
                _fenceEvent));
        WaitForSingleObject(_fenceEvent, INFINITE);
    }

    _frameIndex = _swapChain->GetCurrentBackBufferIndex();
}
