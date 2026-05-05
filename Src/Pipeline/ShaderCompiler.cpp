#include "ShaderCompiler.h"
//#include <dxcapi.h>
#include <dxcapi.h>

namespace Core
{
	IDxcBlob* ShaderCompilerDXC::Compile(const std::wstring& shaderPath, const std::wstring& entryPoint, const std::wstring& targetProfile)
	{

		// Cargar archivo
		IDxcBlobEncoding* source = nullptr;
		HRESULT hr = utils->LoadFile(shaderPath.c_str(), nullptr, &source);
		if (FAILED(hr) || source == nullptr)
		{
			OutputDebugStringA("Failed to load shader file.\n");
			return nullptr;
		}

		// Crear include handler
		IDxcIncludeHandler* includeHandler = nullptr;
		hr = utils->CreateDefaultIncludeHandler(&includeHandler);
		if (FAILED(hr) || includeHandler == nullptr)
		{
			OutputDebugStringA("Failed to create shader include handler.\n");
			return nullptr;
		}

		// Argumentos básicos
		LPCWSTR args[] =
		{
			shaderPath.c_str(),
			L"-E", entryPoint.c_str(),
			L"-T", targetProfile.c_str(),
			L"-Zi", L"-Qembed_debug", // Debug info embebida
			L"-Od" // Sin optimización
		};


		// Compilar
		IDxcOperationResult* result;
		hr = compiler->Compile(
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
		hr = result->GetStatus(&status);
		if (FAILED(hr))
		{
			OutputDebugStringA("Failed to query shader compile status.\n");
			return nullptr;
		}
		if (FAILED(status))
		{
			IDxcBlobEncoding* errors = nullptr;
			if (SUCCEEDED(result->GetErrorBuffer(&errors)) && errors != nullptr)
			{
				OutputDebugStringA((char*)errors->GetBufferPointer());
			}
			else
			{
				OutputDebugStringA("Shader compilation failed with no error buffer.\n");
			}
			return nullptr;
		}

		IDxcBlob* shaderBlob = nullptr;
		result->GetResult((IDxcBlob**)&shaderBlob);
		return shaderBlob;
	}
}
