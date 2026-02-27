#ifndef MJ_D3D11_SHADER_FILE_H
#define MJ_D3D11_SHADER_FILE_H

#include <string>
#include <d3d11.h>
#include <d3dcompiler.h>

#define VERTEX_SHADER_FILE 0
#define PIXEL_SHADER_FILE 1


class ShaderFile
{
	public:
		ShaderFile();
		ShaderFile(
			std::wstring path,
			std::string entryPoint,
			std::string version,
			unsigned char shaderFileType);
		~ShaderFile();

		ID3D11VertexShader* VertexShaderCompile(ID3D11Device* dev);
		ID3D11PixelShader* PixelShaderCompile(ID3D11Device* dev);
		LPVOID GetBufferPointer();
		size_t GetBlobSize();
	private:
		
		std::wstring path;
		std::string entryPoint;
		std::string version;
		ID3D10Blob* buffer;

		unsigned char type;
};




#endif // MJ_D3D11_SHADER_FILE_H
