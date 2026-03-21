// ImGui.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <iostream>
#include <d3dcompiler.h>
#include "ShaderCompiler.h"
#include "Desktop/Window.h"
#include "Graphics/DescriptorHeap.h"
#include <DirectXMath.h>
#include "Dta/imgui.h"
#include "Dta/imgui_impl_win32.h"
#include "Dta/imgui_impl_dx12.h"



#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace Graphics;
using namespace Desktop;
using namespace DirectX;

class Render
{
public:

    class VertexBuffer
    {
    public:
        VertexBuffer() = default;

        ID3D12Resource* m_vertexBuffer = nullptr;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

        uint32_t stride = { };
        uint32_t size = { };

        void Destroy()
        {
            if (m_vertexBuffer)
            {
                m_vertexBuffer->Release();
                m_vertexBuffer = nullptr;
            }
        }
    } vertexBuffer;

    class IndexBuffer
    {
    public:
        IndexBuffer() = default;
        ID3D12Resource* m_indexBuffer = nullptr;
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
        uint32_t stride = { };
        uint32_t size = { };
        uint32_t m_indexCount { };

        void Destroy()
        {
            if (m_indexBuffer)
            {
                m_indexBuffer->Release();
                m_indexBuffer = nullptr;
            }
        }

    } indexBuffer;

    struct ConstantBuffer
    {
        ID3D12Resource* m_buffer = nullptr;
        D3D12_CONSTANT_BUFFER_VIEW_DESC m_desc = { };
        uint32_t m_size = 0;

        void Destroy()
        {
            if (m_buffer)
            {
                m_buffer->Release();
                m_buffer = nullptr;
            }
        }
    } constBuffer, constBuffer2;



    struct CameraBuffer
    {
        XMMATRIX word;
        XMMATRIX view;
        XMMATRIX projection;
    } cameraData, cameraData2;


public:
    Render() = default;

    uint32_t m_Width { };
    uint32_t m_Height { };
    uint32_t m_FrameCount { 2 };

    // Render device and resources
    ID3D12Device* device = nullptr;
    ID3D12CommandQueue* commandQueue = nullptr;
    IDXGISwapChain3* swapChain = nullptr;
    ID3D12Resource* renderTargets[2];
    ID3D12CommandAllocator* commandAlloc = nullptr;
    ID3D12GraphicsCommandList* commandList = nullptr;

    ID3D12Resource* depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial

    // Pipeline state and root signature
    ID3D12PipelineState* pipelineState = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;
    Core::ShaderCompilerDXC shaderCompiler {};

    // Set the viewport and scissor rect
    D3D12_VIEWPORT viewport = { };
    D3D12_RECT scissorRect = { };

    DescriptorHeap rtvDescriptorHeap {};  // This is a heap for our render target view descriptor
    DescriptorHeap dpvDescriptorHeap {};  // This is a heap for our depth/stencil buffer descriptor
	DescriptorHeap guiDescriptorHeap {}; // This is a heap for our ImGui descriptor heap



    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ID3D12Fence* m_fence;
    UINT64 m_fenceValue;



    bool Initialize(HWND hwnd, uint32_t width, uint32_t Heigh)
    {
        m_Width = width;
        m_Height = Heigh;


        viewport = { 0, 0, (float)m_Width, (float)m_Height, 0.0f, 1.0f };
        scissorRect = { 0, 0, (long)m_Width, (long)m_Height };


        IDXGIFactory4* factory = nullptr;
        CreateDXGIFactory1(IID_PPV_ARGS(&factory));

        D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));



        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));


        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = m_FrameCount;
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        IDXGISwapChain1* tempSwapChain = nullptr;
        factory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);

        // 
        tempSwapChain->QueryInterface(IID_PPV_ARGS(&swapChain));
        factory->Release();

        // Create command allocator and command list
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAlloc));
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc, nullptr, IID_PPV_ARGS(&commandList));

        commandList->Close();


        CreateSynchronizationObjects();
        CreateRenderTargetViews();
        CreateDepthBuffer();
        CreatePipeline();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateConstantBuffer();




		// Create ImGui context

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::StyleColorsDark();

        // Backend
        ImGui_ImplWin32_Init(hwnd);

		guiDescriptorHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, true);

        ImGui_ImplDX12_InitInfo init_info = {};
        init_info.Device = device;
        init_info.CommandQueue = commandQueue;
        init_info.NumFramesInFlight = m_FrameCount;
        init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;

        // Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
        // (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
        init_info.SrvDescriptorHeap = guiDescriptorHeap.m_Heap;


        init_info.LegacySingleSrvCpuDescriptor = guiDescriptorHeap.GetCPUDescriptorHandleForHeapStart();
        init_info.LegacySingleSrvGpuDescriptor = guiDescriptorHeap.GetGPUDescriptorHandleForHeapStart();

        ImGui_ImplDX12_Init(&init_info);

        return true;
    }

    void CreateSynchronizationObjects()
    {
        m_frameIndex = swapChain->GetCurrentBackBufferIndex();
        m_fenceValue = 0;
        // Create a fence for synchronization
        device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
        // Create an event handle to use for frame synchronization
        m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);

        if (m_fenceEvent == nullptr)
        {
            throw std::runtime_error("Failed to create fence event.");
        }

        WaitForPreviousFrame();
    }

    void WaitForPreviousFrame()
    {
        // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
        // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
        // sample illustrates how to use fences for efficient resource usage and to
        // maximize GPU utilization.

        // Signal and increment the fence value.
        const UINT64 fence = m_fenceValue;
        commandQueue->Signal(m_fence, fence);
        m_fenceValue++;

        // Wait until the previous frame is finished.
        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_frameIndex = swapChain->GetCurrentBackBufferIndex();
    }


    void CreateRenderTargetViews()
    {
        // Create RTV descriptor heap
        rtvDescriptorHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_FrameCount, false);

        for (uint32_t i = 0; i < m_FrameCount; ++i)
        {
            ID3D12Resource* backBuffer = nullptr;
            swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap.GetCPUHandle(i);
            device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);
            renderTargets[i] = backBuffer;
        }

    }


    void CreateDepthBuffer()
    {
        // Create depth stencil buffer
        D3D12_RESOURCE_DESC depthStencilDesc = {};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        depthStencilDesc.Width = m_Width;
        depthStencilDesc.Height = m_Height;
        depthStencilDesc.DepthOrArraySize = 1;
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        // Clear value for the depth stencil buffer
        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;


        // Create heap properties for the depth stencil buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;


        // Create the depth stencil buffer resource
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthStencilBuffer));

        // Create descriptor heap for depth stencil view (DSV)
        dpvDescriptorHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);


        // Create the depth stencil view (DSV)
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        dsvDesc.Texture2D.MipSlice = 0;

        device->CreateDepthStencilView(depthStencilBuffer, &dsvDesc, dpvDescriptorHeap.GetCPUHandle(0));
    }

    void CreatePipeline()
    {
        auto vertexShaderBlob = shaderCompiler.Compile(L"../../../../Assets/Shaders/ImGui/VertexShader.hlsl", L"VS", L"vs_6_0");
        auto pixelShaderBlob = shaderCompiler.Compile(L"../../../../Assets/Shaders/ImGui/PixelShader.hlsl", L"PS", L"ps_6_0");



        D3D12_ROOT_PARAMETER cbvRootParam = {};
        cbvRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		cbvRootParam.Descriptor.ShaderRegister = 0; // b0
        cbvRootParam.Descriptor.RegisterSpace = 0;
        cbvRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;


        D3D12_ROOT_PARAMETER rootParams[] = { cbvRootParam };

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = _countof(rootParams);
        rootSigDesc.pParameters = rootParams;
        rootSigDesc.NumStaticSamplers = 0;
        rootSigDesc.pStaticSamplers = nullptr;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;




        ID3DBlob* sigBlob = nullptr;
        ID3DBlob* errorBlob = nullptr;

        D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob);
        device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));


        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };


        // Rasterizer state
        D3D12_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise = false;
        rasterizerDesc.DepthClipEnable = true;


        // Blend state
        // This is the blend state for the pipeline. It defines how blending is done.
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = false;
        blendDesc.IndependentBlendEnable = false;
        blendDesc.RenderTarget[0].BlendEnable = true;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


        // Depth stencil
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilDesc.StencilEnable = false;



        // --- PIPELINE STATE [PSO]---
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
        psoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
        psoDesc.InputLayout.NumElements = _countof(inputElementDescs);
        psoDesc.InputLayout.pInputElementDescs = inputElementDescs;
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.BlendState = blendDesc;
        psoDesc.DepthStencilState = depthStencilDesc;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;  // Set the render target format
        psoDesc.SampleDesc.Count = 1;

        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState));
    }


    void CreateVertexBuffer()
    {
        // Define vertices for a triangle
        struct Vertex
        {
            float position[4];
            float color[4];
        };

        Vertex vertices[] =
        {
            // Front face
            {{-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Right side face
            {{ 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Left side face
            {{-0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Back face
            {{ 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Top face
            {{-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Bottom face
            {{ 0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        };

        vertexBuffer.size = sizeof(vertices);
        vertexBuffer.stride = sizeof(Vertex);

        // Create vertex buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = vertexBuffer.size;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;


        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer.m_vertexBuffer));


        // Copy vertex data to the vertex buffer
        void* pData;
        vertexBuffer.m_vertexBuffer->Map(0, nullptr, &pData);
        memcpy(pData, vertices, sizeof(vertices));
        vertexBuffer.m_vertexBuffer->Unmap(0, nullptr);


        // Initialize the vertex buffer view.
        vertexBuffer.m_vertexBufferView.BufferLocation = vertexBuffer.m_vertexBuffer->GetGPUVirtualAddress();
        vertexBuffer.m_vertexBufferView.StrideInBytes = vertexBuffer.stride;
        vertexBuffer.m_vertexBufferView.SizeInBytes = vertexBuffer.size;

    }

    void CreateIndexBuffer()
    {
        // Define indices for a triangle
        uint32_t indices[] =
        {
            // front face
            0, 1, 2, // first triangle
            0, 3, 1, // second triangle

            // left face
            4, 5, 6, // first triangle
            4, 7, 5, // second triangle

            // right face
            8, 9, 10, // first triangle
            8, 11, 9, // second triangle

            // back face
            12, 13, 14, // first triangle
            12, 15, 13, // second triangle

            // top face
            16, 17, 18, // first triangle
            16, 19, 17, // second triangle

            // bottom face
            20, 21, 22, // first triangle
            20, 23, 21, // second triangle
        };

        indexBuffer.size = sizeof(indices);
        indexBuffer.stride = sizeof(uint32_t);
        indexBuffer.m_indexCount = _countof(indices);

        // Create index buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = indexBuffer.size;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer.m_indexBuffer));


        // Copy index data to the index buffer
        void* pData;
        indexBuffer.m_indexBuffer->Map(0, nullptr, &pData);
        memcpy(pData, indices, sizeof(indices));
        indexBuffer.m_indexBuffer->Unmap(0, nullptr);


        // Initialize the index buffer view.
        indexBuffer.m_indexBufferView.BufferLocation = indexBuffer.m_indexBuffer->GetGPUVirtualAddress();
        indexBuffer.m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        indexBuffer.m_indexBufferView.SizeInBytes = indexBuffer.size;
    }


    void CreateConstantBuffer()
    {
        constBuffer.m_size = (sizeof(CameraBuffer) + 255) & ~255;

        // Create constant buffer
        constBuffer.m_size = 256; // Size of the constant buffer in bytes
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = constBuffer.m_size;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuffer.m_buffer));

        
		// Create second constant buffer
        device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&constBuffer2.m_buffer));


        CreateCamera();
    }

    void CreateCamera()
    {
        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH({ 0, 0, -3 }, { 0, 0, 0 }, { 0, 1, 0 });

        // Set up projection matrix (perspective)
        float fov = 45.0f * (3.14f / 180.0f);
        float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
        float nearZ = 0.1f;
        float farZ = 1000.0f;
        DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);

        // Transpose matrices for HLSL (row-major in C++, column-major in HLSL)
        cameraData.word = DirectX::XMMatrixIdentity();
        cameraData.view = DirectX::XMMatrixTranspose(view);
        cameraData.projection = DirectX::XMMatrixTranspose(projection);


        // Transpose matrices for HLSL (row-major in C++, column-major in HLSL)
        cameraData2.word = DirectX::XMMatrixIdentity();
        cameraData2.view = DirectX::XMMatrixTranspose(view);
        cameraData2.projection = DirectX::XMMatrixTranspose(projection);

    }


    // Cubo 1
    float cube1RotationSpeed[3] = { 0.0f, 0.0f, 0.0f }; // pitch, yaw, roll
    float cube1Position[3] = { 0.2f, 0.0f, 0.0f };

    // Cubo 2
    float cube2RotationSpeed[3] = { 0.0f, 0.0f, 0.0f };
    float cube2Position[3] = { -0.2f, 0.0f, 0.0f };

    // Ángulos acumulados
    float cube1Angles[3] = { 0.0f, 0.0f, 0.0f };
    float cube2Angles[3] = { 0.0f, 0.0f, 0.0f };



    void OnUpdate()
    {
        for (int i = 0; i < 3; ++i)
        {
            cube1Angles[i] += cube1RotationSpeed[i];
            cube2Angles[i] += cube2RotationSpeed[i];
        }


		// Cube 1
        XMMATRIX rot1 = XMMatrixRotationRollPitchYaw(cube1Angles[0], cube1Angles[1], cube1Angles[2]);
        XMMATRIX trans1 = XMMatrixTranslation(cube1Position[0], cube1Position[1], cube1Position[2]);
        cameraData.word = XMMatrixTranspose(rot1 * trans1);

        void* mappedData = nullptr;
        constBuffer.m_buffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, &cameraData, sizeof(CameraBuffer));
        constBuffer.m_buffer->Unmap(0, nullptr);





        // Cube 2
        XMMATRIX rot2 = XMMatrixRotationRollPitchYaw(cube2Angles[0], cube2Angles[1], cube2Angles[2]);
        XMMATRIX trans2 = XMMatrixTranslation(cube2Position[0], cube2Position[1], cube2Position[2]);
        cameraData2.word = XMMatrixTranspose(rot2 * trans2);

        void* mappedData2 = nullptr;
        constBuffer2.m_buffer->Map(0, nullptr, &mappedData2);
        memcpy(mappedData2, &cameraData2, sizeof(CameraBuffer));
        constBuffer2.m_buffer->Unmap(0, nullptr);
    }

    void OnRenderGui()
    {
        ImGui::Begin("Cubes Transform Control");

        if (ImGui::CollapsingHeader("Cube 1"))
        {
            ImGui::SliderFloat3("Pos 1", cube1Position, -5.0f, 5.0f);
            ImGui::SliderFloat3("Rot Speed 1", cube1RotationSpeed, -0.1f, 0.1f);
        }

        if (ImGui::CollapsingHeader("Cube 2"))
        {
            ImGui::SliderFloat3("Pos 2", cube2Position, -5.0f, 5.0f);
            ImGui::SliderFloat3("Rot Speed 2", cube2RotationSpeed, -0.1f, 0.1f);
        }

        ImGui::End();
    }





    void OnRender()
    {
        // get the current back buffer index
        uint32_t backBufferIndex = swapChain->GetCurrentBackBufferIndex();

        // Reset the command allocator and command list for the current frame
        commandAlloc->Reset();
        commandList->Reset(commandAlloc, nullptr);




        // get a handle to the depth/stencil buffer
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dpvDescriptorHeap.GetCPUHandle(0);

        // get a handle to the render target view (RTV) for the current back buffer
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvDescriptorHeap.GetCPUHandle(backBufferIndex);

        // set the render target for the output merger stage (the output of the pipeline)
        // Set the render target view (RTV) for the current back buffer
        // Set the depth/stencil view (DSV) for the current frame
        commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


        // Clear the render target
        float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);



        commandList->SetGraphicsRootSignature(rootSignature);

        commandList->SetPipelineState(pipelineState);
        commandList->IASetVertexBuffers(0, 1, &vertexBuffer.m_vertexBufferView);
        commandList->IASetIndexBuffer(&indexBuffer.m_indexBufferView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->SetGraphicsRootConstantBufferView(0, constBuffer.m_buffer->GetGPUVirtualAddress());

        commandList->DrawIndexedInstanced(indexBuffer.m_indexCount, 1, 0, 0, 0);




		//  second cube
        commandList->SetGraphicsRootSignature(rootSignature);
        commandList->SetPipelineState(pipelineState);
        commandList->IASetVertexBuffers(0, 1, &vertexBuffer.m_vertexBufferView);
        commandList->IASetIndexBuffer(&indexBuffer.m_indexBufferView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetGraphicsRootConstantBufferView(0, constBuffer2.m_buffer->GetGPUVirtualAddress());
        commandList->DrawIndexedInstanced(indexBuffer.m_indexCount, 1, 0, 0, 0);


        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        OnRenderGui();
        // Render ImGui
        ImGui::Render();


        ID3D12DescriptorHeap* guiheaps[] = { guiDescriptorHeap.m_Heap };
        commandList->SetDescriptorHeaps(_countof(guiheaps), guiheaps);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);


        commandList->Close();

        // Execute the command list
        ID3D12CommandList* ppCommandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, ppCommandLists);

        // Present the frame
        swapChain->Present(1, 0);

        // Signal and increment the fence value.
        // This will be used to synchronize the GPU and CPU.
        WaitForPreviousFrame();
    }





    void OnResize(uint32_t newWidth, uint32_t newHeight)
    {
        m_Width = newWidth;
        m_Height = newHeight;

        commandQueue->Signal(m_fence, ++m_fenceValue);
        WaitForPreviousFrame();

        for (int i = 0; i < m_FrameCount; ++i)
        {
            if (renderTargets[i]) renderTargets[i]->Release();
        }
        if (depthStencilBuffer) depthStencilBuffer->Release();

        // Resize swap chain
        swapChain->ResizeBuffers(m_FrameCount, m_Width, m_Height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

        CreateRenderTargetViews();

        CreateDepthBuffer();

        // Viewport & Scissor Rect
        viewport = { 0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, 1.0f };
        scissorRect = { 0, 0, static_cast<long>(m_Width), static_cast<long>(m_Height) };

        CreateCamera();
    }


    void Cleanup()
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        if (indexBuffer.m_indexBuffer)
            indexBuffer.Destroy();

        if (vertexBuffer.m_vertexBuffer)
            vertexBuffer.Destroy();

        if (depthStencilBuffer)
            depthStencilBuffer->Release();


        if (pipelineState)
            pipelineState->Release();

        for (uint32_t i = 0; i < 2; ++i)
            if (renderTargets[i])
                renderTargets[i]->Release();

        rtvDescriptorHeap.Destroy();
        dpvDescriptorHeap.Destroy();

        if (swapChain)
            swapChain->Release();

        if (commandQueue)
            commandQueue->Release();

        if (device)
            device->Release();

        if (commandAlloc)
            commandAlloc->Release();

        if (commandList)
            commandList->Release();
    }
};







int main()
{
    WindowApp win = { 1280u, 720u, L"DX12 ImGui" };
    win.Initialize(GetModuleHandle(nullptr));

    Render render {};
    render.Initialize(win.GetHWND(), win.GetWidth(), win.GetHeight());


    win.SetOnUpdate([&render]
        {
        });




    win.SetOnRender([&render]
        {
            render.OnUpdate();
            render.OnRender();
        });


    win.SetOnResize([&render](UINT w, UINT h)
        {
            render.OnResize(w, h);
        });


    win.Run();

    return 0;
}
