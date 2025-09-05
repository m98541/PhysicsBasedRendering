#ifndef BMPFILEIO_H
#define BMPFILEIO_H

#define BMP_FORMAT_BGR 3
#define BMP_FORMAT_BGRA 4
 
typedef struct BMPFILE {
	unsigned char* data;
	unsigned int width;
	unsigned int height;
	unsigned int size;
	unsigned int format;
	unsigned int widthPadding;
}BMPFILE_T;

bool LoadBmpFile(const char* fileName, BMPFILE_T* ImageBuffer, unsigned int BMP_FORMAT);
bool SaveBmpFile(const char* fileName, BMPFILE_T* ImageBuffer);
#endif // !BMPFILEIO_H
