#pragma once

#include <dxcapi.h>
#include <wrl/client.h>
#include <string>
#include <vector>

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

		IDxcBlob* Compile(const std::wstring& shaderPath, const std::wstring& entryPoint, const std::wstring& targetProfile)
		{
		
			IDxcBlobEncoding* source = nullptr;
			utils->LoadFile(shaderPath.c_str(), nullptr, &source);

			
			IDxcIncludeHandler* includeHandler = nullptr;
			utils->CreateDefaultIncludeHandler(&includeHandler);

			// Argumentos básicos
			LPCWSTR args[] =
			{
				shaderPath.c_str(),
				L"-E", entryPoint.c_str(),
				L"-T", targetProfile.c_str(),
				L"-Zi", L"-Qembed_debug", // Debug info embebida
				L"-Od" // not optimization
			};


			// Compilar
			ComPtr<IDxcOperationResult> result;
			HRESULT hr = compiler->Compile(
				source,
				shaderPath.c_str(),
				entryPoint.c_str(),
				targetProfile.c_str(),
				args, _countof(args),
				nullptr, 0,
				includeHandler,
				&result
			);



			HRESULT status;
			result->GetStatus(&status);
			if (FAILED(status))
			{
				IDxcBlobEncoding* errors = nullptr;
				result->GetErrorBuffer(&errors);
				OutputDebugStringA((char*)errors->GetBufferPointer());
				return nullptr;
			}

			IDxcBlob* shaderBlob = nullptr;
			result->GetResult((IDxcBlob**)&shaderBlob);
			return shaderBlob;
		}

	private:
		IDxcCompiler* compiler = nullptr;
		IDxcLibrary* library = nullptr;
		IDxcUtils* utils = nullptr;
	};


}
