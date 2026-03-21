#include "GUI.h"


namespace Graphics
{

	void GUI::Initialize(ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t frameCount, HWND hwnd)
	{

		// Create ImGui context

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		ImGui::StyleColorsDark();

		// Backend
		ImGui_ImplWin32_Init(hwnd);

		m_descriptorHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024, true);

		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = device;
		init_info.CommandQueue = queue;
		init_info.NumFramesInFlight = frameCount;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;

		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
		// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
		init_info.SrvDescriptorHeap = m_descriptorHeap.m_Heap;


		init_info.LegacySingleSrvCpuDescriptor = m_descriptorHeap.GetCPUDescriptorHandleForHeapStart();
		init_info.LegacySingleSrvGpuDescriptor = m_descriptorHeap.GetGPUDescriptorHandleForHeapStart();

		ImGui_ImplDX12_Init(&init_info);


	}
	void GUI::Render(ID3D12GraphicsCommandList* cmd)
	{
		if(cmd)
			ImGui::Render();



		ID3D12DescriptorHeap* guiheaps[] = { m_descriptorHeap.m_Heap };
		cmd->SetDescriptorHeaps(_countof(guiheaps), guiheaps);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmd);
	}


	void GUI::NewFrame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}
	

	void GUI::Destroy()
	{
		if (m_descriptorHeap.m_Heap)
			m_descriptorHeap.Destroy();

		if (m_context)
		{
			ImGui::DestroyContext(m_context);
			m_context = nullptr;
		}
	}

	
}