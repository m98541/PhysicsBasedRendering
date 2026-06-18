#include "MJ_D3D11_ShaderFile.h"
#include <cassert>


ShaderFile::ShaderFile()
{
	path = L"";
	entryPoint = "main";
	version = "vs_5_0";
	buffer = nullptr;
}


ShaderFile::~ShaderFile()
{

}

LPVOID ShaderFile::GetBufferPointer()
{
	return buffer->GetBufferPointer();

}

size_t ShaderFile::GetBlobSize()
{
	return buffer->GetBufferSize();
}

ShaderFile::ShaderFile(
	std::wstring path, 
	std::string entryPoint, 
	std::string version,
	unsigned char shaderFileType
)
{
	this->path = path;
	this->entryPoint = entryPoint;
	this->version = version;
	type = shaderFileType;
	buffer = nullptr;
}

ID3D11VertexShader* ShaderFile::VertexShaderCompile(ID3D11Device *dev)
{

	if (type != VERTEX_SHADER_FILE) return nullptr;

	ID3D11VertexShader* pVertexShader;

	LPCWSTR widePath = path.c_str();

	
	std::string vsVersion = version;
	LPCSTR vsVersionCSTR = vsVersion.c_str();

	LPCSTR entryPointCSTR = entryPoint.c_str();

	HRESULT hr = D3DCompileFromFile(widePath, 0, 0, entryPointCSTR, vsVersionCSTR, 0, 0, &buffer, 0);

	if (SUCCEEDED(hr))
	{

		if ( SUCCEEDED(dev->CreateVertexShader(buffer->GetBufferPointer(), buffer->GetBufferSize(), NULL, &pVertexShader)) )
		{
			return pVertexShader;
		}
		else
		{
			assert(false && "create vert shader failed");
			return nullptr;
		}


	}
	else
	{
		printf("\n error code : %d \n", HRESULT_CODE(hr));
		assert(false && "vert shader compile failed");


		return nullptr;
	}



}

ID3D11PixelShader* ShaderFile::PixelShaderCompile(ID3D11Device* dev)
{
	if (type != PIXEL_SHADER_FILE) return nullptr;

	ID3D11PixelShader* pPixelShader;

	LPCWSTR widePath = path.c_str();

	
	std::string psVersion = version;
	LPCSTR psVersionCSTR = psVersion.c_str();

	LPCSTR entryPointCSTR = entryPoint.c_str();


	if (SUCCEEDED(D3DCompileFromFile(widePath, 0, 0, entryPointCSTR, psVersionCSTR, 0, 0, &buffer, 0)))
	{

		if ( SUCCEEDED(dev->CreatePixelShader(buffer->GetBufferPointer(), buffer->GetBufferSize(), NULL, &pPixelShader)) )
		{
			return pPixelShader;
		}
		else
		{
			assert(false && "create pixel shader failed");
			return nullptr;
		}


	}
	else
	{
		assert(false && "pixel shader compile failed");
		return nullptr;
	}


}