#include "DescriptorHeap.h"
#include "GUI.h"


namespace Graphics
{


	void DescriptorHeap::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors, bool isShaderVisible)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = type;
		heapDesc.NumDescriptors = numDescriptors;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		heapDesc.NodeMask = 0;
		if (isShaderVisible)
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));
		m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);
	}




	void DescriptorHeap::Destroy()
	{
		if (m_Heap)
		{
			m_Heap->Release();
			m_Heap = nullptr;
		}
	}
}