// ClearScreen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <iostream>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

class DescriptorHeap
{
public:
    DescriptorHeap() = default;

    void Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = type;
        heapDesc.NumDescriptors = numDescriptors;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;
        device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));
        m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandleForHeapStart();
        handle.ptr += index * m_DescriptorSize;
        return handle;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const
    {
        return m_Heap->GetCPUDescriptorHandleForHeapStart();
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const
    {
        return m_Heap->GetGPUDescriptorHandleForHeapStart();
    }

    uint32_t GetDescriptorSize() const
    {
        return m_DescriptorSize;
    }

    void Destroy()
    {
        if (m_Heap)
        {
            m_Heap->Release();
            m_Heap = nullptr;
        }
    }


private:
    ID3D12DescriptorHeap* m_Heap = nullptr;
    uint32_t m_DescriptorSize = 0;
};

class RenderSystem
{


public:
    RenderSystem() = default;


    bool Initialize(HWND hwnd, uint32_t width, uint32_t Heigh)
    {
        m_Width = width;
        m_Height = Heigh;
        IDXGIFactory4* factory = nullptr;
        CreateDXGIFactory1(IID_PPV_ARGS(&factory));

        D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_Device));


		// Create command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));

		// Create swap chain
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
        factory->CreateSwapChainForHwnd(m_CommandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &tempSwapChain);


        tempSwapChain->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
        factory->Release();


        // Create RTV descriptor heap
        m_RenderTargetViewHeap.Initialize(m_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_FrameCount);

        for (uint32_t i = 0; i < m_FrameCount; ++i)
        {
            ID3D12Resource* backBuffer = nullptr;
            m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));


            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RenderTargetViewHeap.GetCPUHandle(i);
            m_Device->CreateRenderTargetView(backBuffer, nullptr, rtvHandle);

            m_RenderTargets[i] = backBuffer;
        }


        m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAlloc));
        m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_CommandAlloc, nullptr, IID_PPV_ARGS(&m_CommandList));

        m_CommandList->Close();

        return true;
    }



    void Loop()
    {
        // get the current back buffer index
        uint32_t backBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

        // Reset the command allocator and command list for the current frame
        m_CommandAlloc->Reset();
        m_CommandList->Reset(m_CommandAlloc, nullptr);



        // Set the render target view (RTV) for the current back bufferq
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_RenderTargetViewHeap.GetCPUHandle(backBufferIndex);
        m_CommandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

        // Clear the render target
        float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);


        m_CommandList->Close();

        // Execute the command list
        ID3D12CommandList* ppCommandLists[] = { m_CommandList };
        m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);

        // Present the frame
        m_SwapChain->Present(1, 0);
    }


    void Cleanup()
    {
        for (uint32_t i = 0; i < 2; ++i)
            if (m_RenderTargets[i])
                m_RenderTargets[i]->Release();

        m_RenderTargetViewHeap.Destroy();

        if (m_SwapChain)
            m_SwapChain->Release();

        if (m_CommandQueue)
            m_CommandQueue->Release();

        if (m_Device)
            m_Device->Release();

        if (m_CommandAlloc)
            m_CommandAlloc->Release();

        if (m_CommandList)
            m_CommandList->Release();
    }

    private:
        uint32_t m_Width{ };
        uint32_t m_Height{ };
        uint32_t m_FrameCount{ 2 };


        // Render device and resources
        ID3D12Device* m_Device = nullptr;
        ID3D12CommandQueue* m_CommandQueue = nullptr;
        IDXGISwapChain3* m_SwapChain = nullptr;
        ID3D12Resource* m_RenderTargets[2];
        ID3D12CommandAllocator* m_CommandAlloc = nullptr;
        ID3D12GraphicsCommandList* m_CommandList = nullptr;
        DescriptorHeap m_RenderTargetViewHeap {};


};

int main()
{
	uint32_t width = 1280;
	uint32_t height = 820;
    const wchar_t title[] = L"DX12 ClearScreen";

    RenderSystem render {};



    WNDCLASS wc = { };
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        if (msg == WM_DESTROY)
            PostQuitMessage(0);

        return DefWindowProc(hwnd, msg, wparam, lparam);
    };

    wc.lpszClassName = title;
    wc.hInstance = GetModuleHandle(NULL);
    RegisterClass(&wc);
    HWND hwnd = CreateWindow(title, title, WS_OVERLAPPEDWINDOW, 100, 100, width, height, NULL, NULL, wc.hInstance, NULL);
    ShowWindow(hwnd, SW_SHOW);


    if (!render.Initialize(hwnd, width, height))
    {
        std::cerr << "Error DirectX 12" << std::endl;
        return 1;
    }

    MSG msg = { 0 };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            render.Loop();
        }
    }


	render.Cleanup();

    return 0;
}
