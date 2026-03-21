#pragma once


#include <Windows.h>
#include <d3d12.h>
#include "../Dta/imgui.h"
#include "../Dta/imgui_impl_win32.h"
#include "../Dta/imgui_impl_dx12.h"

#include "DescriptorHeap.h"


namespace Graphics
{
	class DescriptorHeap;

	class GUI
	{
	public:
		GUI() = default;
		~GUI() = default;
		void Initialize(ID3D12Device* device, ID3D12CommandQueue* queue, uint32_t frameCount, HWND hwnd);
		void Render(ID3D12GraphicsCommandList* cmd);
		void NewFrame();
		void Begin(const char* name = "Main Window", bool* p_open = nullptr, ImGuiWindowFlags flags = 0) { ImGui::Begin(name, p_open, flags); }
		void End() { ImGui::End(); }
		void Destroy();


	private:
		DescriptorHeap m_descriptorHeap {};
		ImGuiContext* m_context = nullptr;
	};

}