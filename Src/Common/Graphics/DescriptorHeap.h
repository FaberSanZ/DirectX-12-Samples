#pragma once
#include <d3d12.h>
#include <cstdint>

namespace Graphics
{
    class DescriptorHeap
    {
    public:

        DescriptorHeap() = default;

        ID3D12DescriptorHeap* m_Heap = nullptr;
        uint32_t m_DescriptorSize = 0;


        void Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool isShaderVisible);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index) const
        {
            D3D12_CPU_DESCRIPTOR_HANDLE handle = GetCPUDescriptorHandleForHeapStart();
            handle.ptr += index * m_DescriptorSize;
            return handle;
        }


        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index) const
        {
            D3D12_GPU_DESCRIPTOR_HANDLE handle = GetGPUDescriptorHandleForHeapStart();
            handle.ptr += index * m_DescriptorSize;
            return handle;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() const { return m_Heap->GetCPUDescriptorHandleForHeapStart(); }
        D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() const { return m_Heap->GetGPUDescriptorHandleForHeapStart(); }
        uint32_t GetDescriptorSize() const { return m_DescriptorSize; };


        void Destroy();
    };
}

