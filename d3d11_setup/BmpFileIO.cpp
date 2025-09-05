#include <windows.h>
#include <stdio.h>
#include "BmpFileIO.h"
//file
bool LoadBmpFile(const char* fileName, BMPFILE_T* ImageBuffer, unsigned int BMP_FORMAT)
{
	bool result = 1;

	HANDLE hFile;
	DWORD fileSize, dwRead;
	BITMAPFILEHEADER* fh = NULL;
	BITMAPINFOHEADER* ih = NULL;
	BYTE* pRaster;

	hFile = CreateFileA(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("파일 읽기 실패");
		return 0;
	}

	fileSize = GetFileSize(hFile, NULL);
	fh = (BITMAPFILEHEADER*)malloc(fileSize);
	result = result && ReadFile(hFile, fh, fileSize, &dwRead, NULL);
	CloseHandle(hFile);

	int len = fileSize - fh->bfOffBits;
	pRaster = (BYTE*)malloc(len);
	memcpy(pRaster, (BYTE*)fh + fh->bfOffBits, len);

	ih = (BITMAPINFOHEADER*)((PBYTE)fh + sizeof(BITMAPFILEHEADER));

	
	ImageBuffer->format = BMP_FORMAT;
	ImageBuffer->data = pRaster;
	ImageBuffer->size = len;
	ImageBuffer->width = ih->biWidth;
	ImageBuffer->height = ih->biHeight;
	ImageBuffer->widthPadding = (4-(ImageBuffer->width * ImageBuffer->format) % 4) % 4;


	free(fh);

	return result;
}


bool SaveBmpFile(const char* fileName, BMPFILE_T* ImageBuffer)
{
	bool result = 1;

	BITMAPFILEHEADER fh;
	BITMAPINFOHEADER ih;
	fh.bfType = 'MB';

	BYTE padding =(ImageBuffer->width * ImageBuffer->format) % 2;
	fh.bfSize = (sizeof(BYTE) * ImageBuffer->width * ImageBuffer->format + padding) * ImageBuffer->height + 54;
	printf("size : %d", fh.bfSize);
	fh.bfReserved1 = NULL;
	fh.bfReserved2 = NULL;

	ih.biSize = 40;
	ih.biWidth = ImageBuffer->width;
	ih.biHeight = ImageBuffer->height;
	ih.biPlanes = 1;
	ih.biBitCount = 8 * ImageBuffer->format; //bgr 24bit bgra 32bit
	ih.biCompression = 0;
	ih.biSizeImage = 0;
	ih.biXPelsPerMeter = ImageBuffer->width;
	ih.biYPelsPerMeter = ImageBuffer->height;
	ih.biClrUsed = 0;
	ih.biClrImportant = 0;
	fh.bfOffBits = sizeof(fh) + sizeof(ih);

	HANDLE hFile;
	DWORD dwByte = 0;

	hFile = CreateFileA(fileName, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return 0;
	}

	result = result && WriteFile(hFile, &fh, sizeof(fh), &dwByte, NULL);
	printf("write filehead : %d \n", dwByte);
	result = result && WriteFile(hFile, &ih, sizeof(ih), &dwByte, NULL);
	printf("write infohead : %d \n", dwByte);


	result = result && WriteFile(hFile, ImageBuffer->data, (sizeof(BYTE) * ImageBuffer->width * ImageBuffer->format + padding) * ImageBuffer->height, &dwByte, NULL);
	printf("write image : %d \n", dwByte);

	CloseHandle(hFile);

	return result;
}

