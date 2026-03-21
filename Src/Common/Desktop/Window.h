#pragma once

#include <Windows.h>
#include <tchar.h>

#include <functional>
#include "../Dta/imgui.h"
#include "../Dta/imgui_impl_win32.h"




extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Desktop
{

    class WindowApp
    {

    public:


        WindowApp(uint32_t width, uint32_t height, const wchar_t* title);

        bool Initialize(HINSTANCE hInstance);
        int Run();


        void SetOnUpdate(std::function<void()> func);
        void SetOnRender(std::function<void()> func);
        void SetOnResize(std::function<void(UINT, UINT)> func);

        std::function<void()> onUpdate;
        std::function<void()> onRender;
        std::function<void(UINT, UINT)> onResize;

        HWND GetHWND() const { return m_hWnd; }
        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }

    private:

        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void HandleResize(UINT width, UINT height);

        HWND m_hWnd = nullptr;
        HINSTANCE m_hInstance = nullptr;
        uint32_t m_Width = 1280;
        uint32_t m_Height = 720;
        const wchar_t* m_Title = L"DX12 App";
    };
}

