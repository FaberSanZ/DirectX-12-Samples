// RootConstants.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <iostream>
#include <random> // std::random_device, std::mt19937, std::uniform_real_distribution
#include <d3dcompiler.h>
#include "ShaderCompiler.h"
#include "Desktop/Window.h"
#include "Graphics/DescriptorHeap.h"
#include <DirectXMath.h>
#include <Graphics/GUI.h>

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")


// Using namespaces for convenience.
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
    } constBuffer;



    struct CameraBuffer
    {
        XMMATRIX view;
        XMMATRIX projection;
    } cameraBuffer;

    struct Camera
    {
        float m_positionX, m_positionY, m_positionZ;
        float m_rotationX, m_rotationY, m_rotationZ;

        XMMATRIX m_viewMatrix;



        Camera()
        {
            m_positionX = 0.0f;
            m_positionY = 0.0f;
            m_positionZ = 0.0f;

            m_rotationX = 0.0f;
            m_rotationY = 0.0f;
            m_rotationZ = 0.0f;

            m_viewMatrix = XMMatrixIdentity();
        }

        void SetPosition(float x, float y, float z)
        {
            m_positionX = x;
            m_positionY = y;
            m_positionZ = z;
            return;
        }


        void SetRotation(float x, float y, float z)
        {
            m_rotationX = x;
            m_rotationY = y;
            m_rotationZ = z;
            return;
        }


        XMFLOAT3 GetPosition()
        {
            return XMFLOAT3(m_positionX, m_positionY, m_positionZ);
        }


        XMFLOAT3 GetRotation()
        {
            return XMFLOAT3(m_rotationX, m_rotationY, m_rotationZ);
        }


        void Render()
        {
            XMFLOAT3 up, position, lookAt;
            XMVECTOR upVector, positionVector, lookAtVector;
            float yaw, pitch, roll;
            XMMATRIX rotationMatrix;


            // Setup the vector that points upwards.
            up.x = 0.0f;
            up.y = 1.0f;
            up.z = 0.0f;

            // Load it into a XMVECTOR structure.
            upVector = XMLoadFloat3(&up);

            // Setup the position of the camera in the world.
            position.x = m_positionX;
            position.y = m_positionY;
            position.z = m_positionZ;

            // Load it into a XMVECTOR structure.
            positionVector = XMLoadFloat3(&position);

            // Setup where the camera is looking by default.
            lookAt.x = 0.0f;
            lookAt.y = 0.0f;
            lookAt.z = 1.0f;

            // Load it into a XMVECTOR structure.
            lookAtVector = XMLoadFloat3(&lookAt);

            // Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
            pitch = m_rotationX * 0.0174532925f;
            yaw = m_rotationY * 0.0174532925f;
            roll = m_rotationZ * 0.0174532925f;

            // Create the rotation matrix from the yaw, pitch, and roll values.
            rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

            // Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
            lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
            upVector = XMVector3TransformCoord(upVector, rotationMatrix);

            // Translate the rotated camera position to the location of the viewer.
            lookAtVector = XMVectorAdd(positionVector, lookAtVector);

            // Finally create the view matrix from the three updated vectors.
            m_viewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);

            return;
        }


        XMMATRIX GetViewMatrix() const { return m_viewMatrix; }


    };


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

    // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
    ID3D12Resource* depthStencilBuffer; 

    // Pipeline state and root signature
    ID3D12PipelineState* pipelineState = nullptr;
    ID3D12RootSignature* rootSignature = nullptr;
    Core::ShaderCompilerDXC shaderCompiler {};

	// using viewport and scissor rect to define the area we will render to
    D3D12_VIEWPORT viewport = { };
    D3D12_RECT scissorRect = { };

    // This is a heap for our render target view descriptor
    DescriptorHeap rtvDescriptorHeap {}; 

    // This is a heap for our depth/stencil buffer descriptor
    DescriptorHeap dpvDescriptorHeap {};  



    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ID3D12Fence* m_fence;
    UINT64 m_fenceValue;

    // GUI object for ImGui integration
    GUI gui;


    // Camera
    Camera camera {};
    float SCREEN_DEPTH = 1000.0f;
    float SCREEN_NEAR = 0.1f;


    // Cube positions and rotation speeds, 
    // these will be used to control the cubes in the scene
    const uint32_t OBJECT_INSTANCES = 255;
    DirectX::XMFLOAT3 rotationSpeeds[256] = {};
    float rotation = 0.0f;
    float dimension = 0;
    uint32_t drawCallCount = 0;



    bool Initialize(HWND hwnd, uint32_t width, uint32_t Heigh)
    {
		// Set screen width and height
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


        // Create ImGui context
        gui.Initialize(device, commandQueue, m_FrameCount, hwnd);


        CreateSynchronizationObjects();
        CreateRenderTargetViews();
        CreateDepthBuffer();
        CreatePipeline();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateConstantBuffer();
        CreateCamera();
        InitializeRotationSpeeds();

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
        auto vertexShaderBlob = shaderCompiler.Compile(L"../../../../Assets/Shaders/RootConstants/VertexShader.hlsl", L"VS", L"vs_6_0");
        auto pixelShaderBlob = shaderCompiler.Compile(L"../../../../Assets/Shaders/RootConstants/PixelShader.hlsl", L"PS", L"ps_6_0");



        D3D12_ROOT_PARAMETER cbvRootParam = {};
        cbvRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        cbvRootParam.Descriptor.ShaderRegister = 0; // b0
        cbvRootParam.Descriptor.RegisterSpace = 0;
        cbvRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        // 1. Define el root parameter para las constantes
        D3D12_ROOT_PARAMETER rootParam = {};
        rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParam.Constants.Num32BitValues = 8;
        rootParam.Constants.ShaderRegister = 1;                               // Register b1
        rootParam.Constants.RegisterSpace = 0;
        rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;


        D3D12_ROOT_PARAMETER rootParams[] = { cbvRootParam, rootParam };

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


    }

    void CreateCamera()
    {
        camera.SetPosition(0.0f, 0.0f, -5.0f);
        camera.SetRotation(0.0f, 0.0f, 0.0f);
        camera.Render();


        // Setup the projection matrix.
        auto fieldOfView = 3.141592654f / 4.0f;
        float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

        // Create the projection matrix for 3D rendering.
        cameraBuffer.projection = DirectX::XMMatrixTranspose(XMMatrixPerspectiveFovLH(fieldOfView, aspect, SCREEN_NEAR, SCREEN_DEPTH));
        cameraBuffer.view = DirectX::XMMatrixTranspose(camera.GetViewMatrix());
    }

    void OnUpdate()
    {
        camera.Render();
        cameraBuffer.view = DirectX::XMMatrixTranspose(camera.GetViewMatrix());


        void* mappedData = nullptr;
        constBuffer.m_buffer->Map(0, nullptr, &mappedData);
        memcpy(mappedData, &cameraBuffer, sizeof(CameraBuffer));
        constBuffer.m_buffer->Unmap(0, nullptr);
    }



    void OnRenderGui()
    {
        uint32_t totaltriangles = drawCallCount * (indexBuffer.m_indexCount / 3);
        ImGui::Begin("Camera Control");
		ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
		ImGui::Text("Screen Size: %ux%u", m_Width, m_Height);
        ImGui::Text("Draw Calls: %u", drawCallCount);
		ImGui::Text("Total Triangles: %u", totaltriangles);

        drawCallCount = 0;
		totaltriangles = 0;

        ImGui::NewLine();

		// Sliders for camera position and rotation
		ImGui::Text("x: %.2f,     y: %.2f,     z: %.2f", camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
		ImGui::SliderFloat3("Camera Position", &camera.m_positionX, -30.0f, 30.0f);
        ImGui::NewLine();
        ImGui::Text("x: %.2f,     y: %.2f,     z: %.2f", camera.GetRotation().x, camera.GetRotation().y, camera.GetRotation().z);
		ImGui::SliderFloat3("Camera Rotation", &camera.m_rotationX, -180.0f, 180.0f);

        ImGui::NewLine();
        // Slider for dimension
		ImGui::Text("Dimension: %.2f", dimension);
        ImGui::SliderFloat("Dimension", &dimension, -10.0f, 10.0f);

		// Button to update camera
        if (ImGui::Button("Update Camera"))
        {
            camera.SetPosition(0.0f, 0, -25.0f);
			camera.SetRotation(0.0f, 0.0f, 0.0f);
        }

		// Button to reset rotation
        if (ImGui::Button("Reset"))
        {
			rotation = 0.0f;
        }

        ImGui::End();
    }


    void InitializeRotationSpeeds()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

        for (int i = 0; i < OBJECT_INSTANCES; i++)
        {
            rotationSpeeds[i] = { dist(gen), dist(gen), dist(gen) };
        }
    }

	// Generate cubes in a grid pattern
    void GenerateCubes(ID3D12GraphicsCommandList* cmd)
    {
        rotation += 0.02f;

        uint32_t dim = static_cast<uint32_t>(std::cbrt(OBJECT_INSTANCES)); // using cube root to determine the dimension of the grid
        XMFLOAT3 offset = { dimension, dimension, dimension };

        float halfDimOffsetX = (dim * offset.x) / 2.0f;
        float halfDimOffsetY = (dim * offset.y) / 2.0f;
        float halfDimOffsetZ = (dim * offset.z) / 2.0f;

        for (uint32_t x = 0; x < dim; ++x)
        {
            for (uint32_t y = 0; y < dim; ++y)
            {
                for (uint32_t z = 0; z < dim; ++z)
                {
                    uint32_t index = x * dim * dim + y * dim + z;


                    XMFLOAT3 position =
                    {
                        -halfDimOffsetX + offset.x / 2.0f + x * offset.x,
                        -halfDimOffsetY + offset.y / 2.0f + y * offset.y,
                        -halfDimOffsetZ + offset.z / 2.0f + z * offset.z
                    };


                    XMFLOAT3 cubeRotation =
                    {
                        rotation * rotationSpeeds[index].x,
                        rotation * rotationSpeeds[index].y,
                        rotation * rotationSpeeds[index].z
                    };

                    AddCube(cmd, position, cubeRotation);
                }
            }
        }
    }

	// Add a cube to the scene at a specific position and rotation
    void AddCube(ID3D12GraphicsCommandList* cmd, XMFLOAT3 position, XMFLOAT3 rotation)
    {
        XMMATRIX trans = XMMatrixTranslation(position.x, position.y, position.z);
        XMMATRIX rot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        XMMATRIX world = XMMatrixTranspose(rot * trans);

        cmd->SetGraphicsRoot32BitConstants(1, 16, &world, 0);
        cmd->DrawIndexedInstanced(indexBuffer.m_indexCount, 1, 0, 0, 0);
        drawCallCount++;
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


        // Clear the render target
        float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// Clear the depth/stencil buffer
        commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);


        // Tender the scene
        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);
        commandList->SetGraphicsRootSignature(rootSignature);
        commandList->SetPipelineState(pipelineState);
        commandList->IASetVertexBuffers(0, 1, &vertexBuffer.m_vertexBufferView);
        commandList->IASetIndexBuffer(&indexBuffer.m_indexBufferView);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetGraphicsRootConstantBufferView(0, constBuffer.m_buffer->GetGPUVirtualAddress());
        GenerateCubes(commandList);



        // Start the Dear ImGui frame
        gui.NewFrame();
        // Call the function to render the GUI
        OnRenderGui();
        // Render ImGui
        gui.Render(commandList);



        // Close the command list
        commandList->Close();

        // Execute the command list
        ID3D12CommandList* ppCommandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, ppCommandLists);

        // Present the frame
        swapChain->Present(0, 0);

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
        if (depthStencilBuffer) 
            depthStencilBuffer->Release();

        // Resize swap chain
        swapChain->ResizeBuffers(m_FrameCount, m_Width, m_Height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

        CreateRenderTargetViews();

        CreateDepthBuffer();

        // Viewport & Scissor Rect
        viewport = { 0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height), 0.0f, 1.0f };
        scissorRect = { 0, 0, static_cast<long>(m_Width), static_cast<long>(m_Height) };


        // Setup the projection matrix.
        auto fieldOfView = 3.141592654f / 4.0f;
        float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);

        // Create the projection matrix for 3D rendering.
        cameraBuffer.projection = DirectX::XMMatrixTranspose(XMMatrixPerspectiveFovLH(fieldOfView, aspect, SCREEN_NEAR, SCREEN_DEPTH));

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
    WindowApp win = { 1280u, 720u, L"DX12 RootConstants" };
    win.Initialize(GetModuleHandle(nullptr));

    Render render {};
    render.Initialize(win.GetHWND(), win.GetWidth(), win.GetHeight());

	// Set the update and render callbacks
    win.SetOnUpdate([&render] { render.OnUpdate(); });
    win.SetOnRender([&render] { render.OnRender(); });

	// Set the resize callback
    win.SetOnResize([&render](UINT w, UINT h) { render.OnResize(w, h); });


    win.Run();

    return 0;
}
