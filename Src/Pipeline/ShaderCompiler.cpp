#include "ShaderCompiler.h"
//#include <dxcapi.h>
#include <dxcapi.h>

namespace Core
{
	IDxcBlob* ShaderCompilerDXC::Compile(const std::wstring& shaderPath, const std::wstring& entryPoint, const std::wstring& targetProfile)
	{

		// Cargar archivo
		IDxcBlobEncoding* source = nullptr;
		utils->LoadFile(shaderPath.c_str(), nullptr, &source);

		// Crear include handler
		IDxcIncludeHandler* includeHandler = nullptr;
		utils->CreateDefaultIncludeHandler(&includeHandler);

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
}