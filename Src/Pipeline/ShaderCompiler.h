#pragma once

#include <wrl/client.h>
#include <string>
#include <vector>
#include <dxcapi.h>

using Microsoft::WRL::ComPtr;
namespace Core
{
	class ShaderCompilerDXC
	{
	public:
		ShaderCompilerDXC()
		{
			DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
			DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
			DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
		}

		IDxcBlob* Compile(const std::wstring& shaderPath, const std::wstring& entryPoint, const std::wstring& targetProfile);

	private:
		IDxcCompiler* compiler = nullptr;
		IDxcLibrary* library = nullptr;
		IDxcUtils* utils = nullptr;
		//IDxcOperationResult* result = nullptr;

	};


}


