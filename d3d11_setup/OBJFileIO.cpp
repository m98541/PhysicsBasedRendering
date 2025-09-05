#define _CRT_SECURE_NO_WARNINGS
#include "OBJFileIO.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LINE_SIZE_MAX 1024

int LineTypeSearch(const char* line);
int MtlLineTypeSearch(char* line, char** OutLineOffset);
int LineElementCount(const char* line);
int MtlElementSearch(MTL_ELEMENT* Buffer, unsigned int Len, const char* SearchMtlName);

bool ParseOBJFile(OBJFILE_DESC* ObjectDesc, const char* FileName)
{
	FILE* objectFp = fopen(FileName, "r");

	if (objectFp == NULL) return 0;

	ObjectDesc->fileName = FileName;
	ObjectDesc->faceFormat = 0;

	for (int i = 0; i < sizeof(ObjectDesc->lenArr)/sizeof(unsigned int); ObjectDesc->lenArr[i++] = 0);
	for (int i = 0; i < 4; ObjectDesc->elementCountArr[i++] = 0);

	char line[LINE_SIZE_MAX];
	fseek(objectFp, 0 , SEEK_SET);
	
	while (fgets(line , LINE_SIZE_MAX, objectFp))
	{
		
		int lineType = LineTypeSearch(line);
		if (lineType != LINE_TYPE_NOP )
		{
			if (lineType == LINE_TYPE_MTL_NAME)
			{
				
				strncpy(ObjectDesc->mtlFileName, line + 7,128);
				char* lineEnd =  strchr(ObjectDesc->mtlFileName , '\n');
				*lineEnd = '\0';
				printf("%s", ObjectDesc->mtlFileName);
				continue;
			}
			
			if (lineType < 3)
				ObjectDesc->faceFormat = ObjectDesc->faceFormat | lineType;

			
			ObjectDesc->lenArr[lineType]++;
			
			int primitive = LineElementCount(line);
			if (primitive == 3 && lineType < 4)
				ObjectDesc->elementCountArr[lineType] += 3;
			else if(lineType < 4)
			{
			
				ObjectDesc->elementCountArr[lineType] += (primitive - 2) * 3;
			}
		}
	
	}

	fclose(objectFp);



	FILE* mtlFp = fopen(ObjectDesc->mtlFileName,"r");
	if (mtlFp == NULL) {
		return 0;
	}
	while (fgets(line, LINE_SIZE_MAX, mtlFp))
	{
		char* lineOffset = nullptr;
		int lineType = MtlLineTypeSearch(line, &lineOffset);
		if (lineType == MTL_LINE_TYPE_NEWMTL)
			ObjectDesc->len.mtl++;
	}

	fclose(mtlFp);
	return 1;

}

bool ReadOBJFile(OBJFILE_DESC_T* OBJFileDESC ,OBJFILE_BUFFER_T** OBJFileBuffer)
{
	

	*OBJFileBuffer = (OBJFILE_BUFFER_T*)malloc(sizeof(OBJFILE_BUFFER_T));
	if (OBJFileDESC->len.vert == 0) return 0;
	(*OBJFileBuffer)->vertBuffer = (float*)malloc(sizeof(float) * 4 * OBJFileDESC->len.vert);
	memset((*OBJFileBuffer)->vertBuffer ,NULL, sizeof(float) * 4 * OBJFileDESC->len.vert);

	(*OBJFileBuffer)->texBuffer = (float*)malloc(sizeof(float) * 2 * OBJFileDESC->len.tex);
	memset((*OBJFileBuffer)->texBuffer ,NULL , sizeof(float) * 2 * OBJFileDESC->len.tex);

	(*OBJFileBuffer)->normBuffer = (float*)malloc(sizeof(float) * 4 * OBJFileDESC->len.normal);
	memset((*OBJFileBuffer)->normBuffer,NULL, sizeof(float) * 4 * OBJFileDESC->len.normal);
	
	(*OBJFileBuffer)->mtlBuffer = (MTL_ELEMENT*)malloc(sizeof(MTL_ELEMENT) * OBJFileDESC->len.mtl);
	memset((*OBJFileBuffer)->mtlBuffer ,NULL , sizeof(MTL_ELEMENT) * OBJFileDESC->len.mtl);

	(*OBJFileBuffer)->materialIdxBuffer = (unsigned int*)malloc(sizeof(unsigned int) * OBJFileDESC->elementCount.face);
	memset((*OBJFileBuffer)->materialIdxBuffer, NULL, sizeof(unsigned int) * OBJFileDESC->elementCount.face);

	(*OBJFileBuffer)->groupSet = (TRI_POLYGON_GROUP_T*)malloc(sizeof(TRI_POLYGON_GROUP_T) * OBJFileDESC->len.group);
	memset((*OBJFileBuffer)->groupSet ,NULL , sizeof(TRI_POLYGON_GROUP_T) * OBJFileDESC->len.group);

	(*OBJFileBuffer)->polygonSet = (TRI_POLYGON_T*)malloc(sizeof(TRI_POLYGON_T) * OBJFileDESC->len.useMtl);
	memset((*OBJFileBuffer)->polygonSet,NULL, sizeof(TRI_POLYGON_T) * OBJFileDESC->len.useMtl);

	int tempTextureNameArrSize = 0;
	char (*tempTextureNameArr)[CHAR_NAME_MAX] = (char(*)[CHAR_NAME_MAX])malloc(sizeof(char[CHAR_NAME_MAX]) * OBJFileDESC->len.mtl);

	unsigned int bufferLineSize = 0;
	char line[LINE_SIZE_MAX];
	
	//load mtl file
	FILE* mtlFp = fopen(OBJFileDESC->mtlFileName, "r");
	if (mtlFp == NULL) {
		return 0;
	}
	int texIdx = 0;
	int mtlIdx = -1;
	while (fgets(line, LINE_SIZE_MAX, mtlFp))
	{
		char* lineOffset = nullptr;
		int lineType = MtlLineTypeSearch(line,&lineOffset);
		if (lineType == LINE_TYPE_NOP) continue;
		

		(*OBJFileBuffer)->mtlBuffer[mtlIdx].mtlKeyWordEnable[lineType] = 1;
		int result = 1;
		switch (lineType)
		{
		case  MTL_LINE_TYPE_NEWMTL :
			mtlIdx++;
			result = sscanf(lineOffset, "newmtl %s", (*OBJFileBuffer)->mtlBuffer[mtlIdx].materialName);
			break;
		case  MTL_LINE_TYPE_KA :
			result = sscanf(lineOffset, "Ka %f %f %f"	, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].ambient
											, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].ambient + 1
											, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].ambient + 2);
			break;
		case  MTL_LINE_TYPE_KD :
			result = sscanf(lineOffset, "Kd %f %f %f", (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].diffuse
											, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].diffuse + 1
											, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].diffuse + 2);
			break;
		case  MTL_LINE_TYPE_KS :
			result = sscanf(lineOffset, "Ks %f %f %f"	, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].specular
											, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].specular + 1
											, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].specular + 2);
			break;
		case  MTL_LINE_TYPE_NS :
			result = sscanf(lineOffset, "Ns %f %f %f", (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].shine
				, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].shine + 1
				, (float*)(*OBJFileBuffer)->mtlBuffer[mtlIdx].shine + 2);
			break;
		case  MTL_LINE_TYPE_D  :
			result = sscanf(lineOffset, "d %f", &(*OBJFileBuffer)->mtlBuffer[mtlIdx].alphaValue);
			break;
		case  MTL_LINE_TYPE_TR :
			result = sscanf(lineOffset, "Tr %f", &(*OBJFileBuffer)->mtlBuffer[mtlIdx].alphaValue);
				(*OBJFileBuffer)->mtlBuffer[mtlIdx].alphaValue = 1.F - (*OBJFileBuffer)->mtlBuffer[mtlIdx].alphaValue;
			break;
		case  MTL_LINE_TYPE_ILLUM :
			result = sscanf(lineOffset, "illum %d", &(*OBJFileBuffer)->mtlBuffer[mtlIdx].lightModel);
			break;
		case  MTL_LINE_TYPE_MAP_KD :
			result = sscanf(lineOffset, "map_Kd %s", (*OBJFileBuffer)->mtlBuffer[mtlIdx].textureMapImageName);
			
			
			for (texIdx = 0; texIdx < tempTextureNameArrSize; texIdx++)
			{
				if (strcmp(tempTextureNameArr[texIdx], (*OBJFileBuffer)->mtlBuffer[mtlIdx].textureMapImageName) == 0)
				{
					(*OBJFileBuffer)->mtlBuffer[mtlIdx].textureIdx = texIdx;
					break;
				}
			}

			if (texIdx == tempTextureNameArrSize)
			{
				strcpy(tempTextureNameArr[tempTextureNameArrSize], (*OBJFileBuffer)->mtlBuffer[mtlIdx].textureMapImageName);
				(*OBJFileBuffer)->mtlBuffer[mtlIdx].textureIdx = tempTextureNameArrSize++;
			}

			break;
		case  MTL_LINE_TYPE_BUMP : 
			result = sscanf(lineOffset, "bump %f", &(*OBJFileBuffer)->mtlBuffer[mtlIdx].bump);
			break;
		case  MTL_LINE_TYPE_MAP_BUMP :
			result = sscanf(lineOffset, "map_Bump %s", (*OBJFileBuffer)->mtlBuffer[mtlIdx].bumpMapImageName);
			break;
		case  MTL_LINE_TYPE_MAP_D :
			result = sscanf(lineOffset, "map_D %s", (*OBJFileBuffer)->mtlBuffer[mtlIdx].alphaMapImageName);
			break;
		case  MTL_LINE_TYPE_NI :
			result = sscanf(lineOffset, "Ni %f", &(*OBJFileBuffer)->mtlBuffer[mtlIdx].ni);
			break;
		case  MTL_LINE_TYPE_KE :
			result = sscanf(lineOffset, "Ke %f %f %f", (*OBJFileBuffer)->mtlBuffer[mtlIdx].ke
												, (*OBJFileBuffer)->mtlBuffer[mtlIdx].ke + 1
												, (*OBJFileBuffer)->mtlBuffer[mtlIdx].ke + 2);
			break;
		case  MTL_LINE_TYPE_MAP_KS:
			result = sscanf(lineOffset, "map_Ks %s", (*OBJFileBuffer)->mtlBuffer[mtlIdx].specularMapImageName);
			break;
		default:
				break;
		}
		
		if (!result && mtlIdx >= 0 )
		{
			(*OBJFileBuffer)->mtlBuffer[mtlIdx].mtlKeyWordEnable[lineType] = 9;
		}




	}

	fclose(mtlFp);


	//read 
	FILE* fp = fopen(OBJFileDESC->fileName, "r");
	if (fp == NULL) return 0;
	
	int vIdx = 0;
	int tIdx = 0;
	int nIdx = 0;
	int fIdx = 0;
	int gIdx = -1;
	int pIdx = -1;
	int useMtlIdx = 0;
	int bufferOffset = 0;

	if (OBJFileDESC->len.vert > 0)
		bufferLineSize += 4;

	if (OBJFileDESC->len.tex > 0)
		bufferLineSize += 2;

	if (OBJFileDESC->len.normal > 0)
		bufferLineSize += 4;


	if (OBJFileDESC->len.face == 0) return 0;
	

	switch (OBJFileDESC->faceFormat)
	{
	case FACE_FORMAT_V:
		
		(*OBJFileBuffer)->objectBuffer = (float*)malloc(sizeof(float) * 4 * OBJFileDESC->elementCount.face);
		break;
	case FACE_FORMAT_V_T:
	case FACE_FORMAT_V_N:
		(*OBJFileBuffer)->objectBuffer = (float*)malloc(sizeof(float) * 4 * 2 * OBJFileDESC->elementCount.face);
		break;
	case FACE_FORMAT_V_N_T:
		(*OBJFileBuffer)->objectBuffer = (float*)malloc(sizeof(float) * 4 * 3 * OBJFileDESC->elementCount.face);
		break;
	default:
		return 0;
	}

	(*OBJFileBuffer)->textureImageNameArr = (char(*)[CHAR_NAME_MAX])malloc(sizeof(char[CHAR_NAME_MAX]) * tempTextureNameArrSize);

	(*OBJFileBuffer)->textureImageNameLen = tempTextureNameArrSize;
	for (int i = 0; i < tempTextureNameArrSize; i++)
	{
		strcpy((*OBJFileBuffer)->textureImageNameArr[i],tempTextureNameArr[i]);
	}

	
	while (fgets(line, LINE_SIZE_MAX, fp))
	{
	
		int lineType = LineTypeSearch(line);

		
		if (lineType == LINE_TYPE_V)
		{
			float x, y, z, w;
			x = y = z = 0.F;
			w = 1.F;
			sscanf(line, "v %f %f %f\n", &x, &y, &z);
			(*OBJFileBuffer)->vertBuffer[vIdx * 4 + 0] = x;
			(*OBJFileBuffer)->vertBuffer[vIdx * 4 + 1] = y;
			(*OBJFileBuffer)->vertBuffer[vIdx * 4 + 2] = z;
			(*OBJFileBuffer)->vertBuffer[vIdx * 4 + 3] = w;
			vIdx++;
		}
		else if (lineType == LINE_TYPE_VT)
		{
			float u, v;
			sscanf(line, "vt %f %f\n", &u, &v);

			(*OBJFileBuffer)->texBuffer[tIdx * 2 + 0] = u;
			(*OBJFileBuffer)->texBuffer[tIdx * 2 + 1] = v;
			tIdx++;

		}
		else if (lineType == LINE_TYPE_VN)
		{
			float x, y, z;
			sscanf(line, "vn %f %f %f\n", &x, &y, &z);

			(*OBJFileBuffer)->normBuffer[nIdx * 4 + 0] = x;
			(*OBJFileBuffer)->normBuffer[nIdx * 4 + 1] = y;
			(*OBJFileBuffer)->normBuffer[nIdx * 4 + 2] = z;
			(*OBJFileBuffer)->normBuffer[nIdx * 4 + 3] = 0.F; // vector_4 aligned
			nIdx++;
		}
		else if (LineTypeSearch(line) == LINE_TYPE_F)
		{
			int v, vt, vn;
			int primitive = LineElementCount(line);
			char lineTemp[LINE_SIZE_MAX];
			char nPrimitiveLine[LINE_SIZE_MAX];
			char* vertexIdx = nullptr;
			if (primitive > PRIMITIVE_TRIANGLE)
			{
				int triMax = primitive-2;
				memcpy(lineTemp ,line,LINE_SIZE_MAX);
				strtok(lineTemp , " ");

				char* faceElement1 = strtok(NULL, " ");
				
				char* faceElement2 = strtok(NULL, " ");
			
				char* faceElement3 = strtok(NULL, " ");

				char* temp;
				
				int lineSize = 0;
				for (int i = 0; i < triMax; i++)
				{
					sprintf(nPrimitiveLine + lineSize, "f %s %s %s\n", faceElement1, faceElement2, faceElement3);
					lineSize += strlen(nPrimitiveLine+lineSize);
					temp = faceElement2;
					faceElement2 = faceElement3;
					faceElement3 = temp;
					faceElement3 = strtok(NULL, " ");
				}
				vertexIdx = strtok(nPrimitiveLine, " ");
				
			}
			else
			{
				vertexIdx = strtok(line, " ");
			}
			
			
			vertexIdx = strtok(NULL, " ");

		
			while (vertexIdx != NULL) {
				v = vt = vn = 0;
				switch (OBJFileDESC->faceFormat)
				{
				case FACE_FORMAT_V:
					sscanf(vertexIdx, "%d", &v);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset, (*OBJFileBuffer)->vertBuffer + (v - 1) * 4, sizeof(float) * 4);
					bufferOffset += bufferLineSize;
					break;
				case FACE_FORMAT_V_T:
					sscanf(vertexIdx, "%d/%d", &v, &vt);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset, (*OBJFileBuffer)->vertBuffer + (v - 1) * 4, sizeof(float) * 4);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset + 4, (*OBJFileBuffer)->texBuffer + (vt - 1) * 2, sizeof(float) * 2);
					bufferOffset += bufferLineSize;

					break;
				case FACE_FORMAT_V_N:
					sscanf(vertexIdx, "%d//%d", &v, &vn);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset, (*OBJFileBuffer)->vertBuffer + (v - 1) * 4, sizeof(float) * 4);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset + 4, (*OBJFileBuffer)->normBuffer + (vn - 1) * 4, sizeof(float) * 4);
					bufferOffset += bufferLineSize;
					break;
				case FACE_FORMAT_V_N_T:
					sscanf(vertexIdx, "%d/%d/%d", &v, &vt, &vn);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset, (*OBJFileBuffer)->vertBuffer + (v - 1) * 4, sizeof(float) * 4);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset + 4, (*OBJFileBuffer)->texBuffer + (vt - 1) * 2, sizeof(float) * 2);
					memcpy((*OBJFileBuffer)->objectBuffer + bufferOffset + 6, (*OBJFileBuffer)->normBuffer + (vn - 1) * 4, sizeof(float) * 4);
					bufferOffset += bufferLineSize;
					break;
				default:
					return 0;
				}
				
				if (gIdx > -1)
					(*OBJFileBuffer)->groupSet[gIdx].vertexCount = (bufferOffset / bufferLineSize) - (*OBJFileBuffer)->groupSet[gIdx].groupOffset;

				if(pIdx > -1)
					(*OBJFileBuffer)->polygonSet[pIdx].vertexCount = (bufferOffset / bufferLineSize) - (*OBJFileBuffer)->polygonSet[pIdx].polygonOffset;

				(*OBJFileBuffer)->materialIdxBuffer[bufferOffset / bufferLineSize - 1] = useMtlIdx;
				vertexIdx = strtok(NULL, " ");
			}

		}
		else if (lineType == LINE_TYPE_MTL_USE)
		{
			pIdx++;
			(*OBJFileBuffer)->polygonSet[pIdx].polygonOffset = (bufferOffset / bufferLineSize);
			char mtlName[32];
			sscanf(line, "usemtl %s", &mtlName);

			useMtlIdx = MtlElementSearch((*OBJFileBuffer)->mtlBuffer, OBJFileDESC->len.mtl, mtlName);
			if (useMtlIdx == -1) useMtlIdx = 0;
		}
		else if (lineType == LINE_TYPE_G)
		{
			gIdx++;
			(*OBJFileBuffer)->groupSet[gIdx].groupOffset =  (bufferOffset / bufferLineSize);
		}
		else continue;
	
	}

	
	//Create Object 
	(*OBJFileBuffer)->polygonSetLen = OBJFileDESC->len.useMtl;
	(*OBJFileBuffer)->groupSetLen = OBJFileDESC->len.group;
	(*OBJFileBuffer)->objectBufferLineSize = bufferLineSize;
	(*OBJFileBuffer)->faceLen = OBJFileDESC->len.face;

	(*OBJFileBuffer)->objectBufferSize = bufferOffset;
	(*OBJFileBuffer)->objectBufferLen = OBJFileDESC->elementCount.face;
	free(tempTextureNameArr);
	fclose(fp);
	return 1;

}


void ReleaseOBJBuffer(OBJFILE_BUFFER_T** OBJFileBuffer)
{
	if (!OBJFileBuffer || !*OBJFileBuffer) return;

	free((*OBJFileBuffer)->mtlBuffer);
	free((*OBJFileBuffer)->vertBuffer);
	free((*OBJFileBuffer)->texBuffer);
	free((*OBJFileBuffer)->normBuffer);
	free((*OBJFileBuffer)->objectBuffer);
	free(*OBJFileBuffer);
	*OBJFileBuffer = NULL;
 
}






int LineTypeSearch(const char* line)
{
	if ((line[0] == 'v') && (line[1] == ' ' || line[1] == 0)) return LINE_TYPE_V;
	else if ((line[0] == 'v') && (line[1] == 't')) return LINE_TYPE_VT;
	else if ((line[0] == 'v') && (line[1] == 'n')) return LINE_TYPE_VN;
	else if ((line[0] == 'f') && (line[1] == ' ' || line[1] == 0)) return LINE_TYPE_F;
	else if (strncmp(line, "mtllib", 6) == 0) return LINE_TYPE_MTL_NAME;
	else if (strncmp(line, "usemtl", 6) == 0) return LINE_TYPE_MTL_USE;
	else if ((line[0] == 'g') && (line[1] == ' ' || line[1] == 0)) return LINE_TYPE_G;
	else if ((line[0] == 'o') && (line[1] == ' ' || line[1] == 0)) return LINE_TYPE_O;

	return LINE_TYPE_NOP;
}


int MtlLineTypeSearch(char* Line,char** OutLineOffset)
{
	if ((*OutLineOffset = strstr(Line, "newmtl")) != NULL) return MTL_LINE_TYPE_NEWMTL;
	else if ((*OutLineOffset = strstr(Line, "map_Ks")) != NULL) return MTL_LINE_TYPE_MAP_KS;
	else if ((*OutLineOffset = strstr(Line, "map_Bump")) != NULL) return MTL_LINE_TYPE_MAP_BUMP;
	else if ((*OutLineOffset = strstr(Line, "map_Kd")) != NULL) return MTL_LINE_TYPE_MAP_KD;
	else if ((*OutLineOffset = strstr(Line, "map_D")) != NULL) return MTL_LINE_TYPE_MAP_D;
	else if ((*OutLineOffset = strstr(Line, "Ka")) != NULL) return MTL_LINE_TYPE_KA;
	else if ((*OutLineOffset = strstr(Line, "Kd")) != NULL) return MTL_LINE_TYPE_KD;
	else if ((*OutLineOffset = strstr(Line, "Ks")) != NULL) return MTL_LINE_TYPE_KS;
	else if ((*OutLineOffset = strstr(Line, "Ns")) != NULL) return MTL_LINE_TYPE_NS;
	else if ((*OutLineOffset = strstr(Line, "Tr")) != NULL) return MTL_LINE_TYPE_TR;
	else if ((*OutLineOffset = strstr(Line, "illum")) != NULL) return MTL_LINE_TYPE_ILLUM;
	else if ((*OutLineOffset = strstr(Line, "Td")) != NULL) return MTL_LINE_TYPE_TD;
	else if ((*OutLineOffset = strstr(Line, "bump")) != NULL) return MTL_LINE_TYPE_BUMP;
	else if ((*OutLineOffset = strstr(Line, "Ni")) != NULL) return MTL_LINE_TYPE_NI;
	else if ((*OutLineOffset = strstr(Line, "Ke")) != NULL) return MTL_LINE_TYPE_KE;
	else if ((*OutLineOffset = strstr(Line, "d ")) != NULL) return MTL_LINE_TYPE_D;
	return LINE_TYPE_NOP;
}

int MtlElementSearch(MTL_ELEMENT* Buffer ,unsigned int Len,const char* SearchMtlName)
{
	int i = 0;
	for (i; i < Len; i++)
	{
		if (strcmp(Buffer[i].materialName, SearchMtlName) == 0) return i;
	}
	return -1;
}


int LineElementCount(const char* line)
{
	int cnt = 0;
	for (int i = 0; line[i] != '\0'; i++)
	{
		if (line[i] == ' ')cnt++;
	}
	return cnt;
}

