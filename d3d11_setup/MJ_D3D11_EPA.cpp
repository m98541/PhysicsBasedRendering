#include "MJ_D3D11_EPA.h"
#include <EASTL/map.h>
#define EPSILON 1e-6f

using namespace eastl;
using namespace DirectX;

const XMVECTOR ORIGIN = { 0,0,0,0 };

struct HorizonEdgeKey {
	XMVECTOR* lowAddr;
	XMVECTOR* bigAddr;
};

struct HorizonKeyCompare
{
	bool operator()(const HorizonEdgeKey& keyA ,const HorizonEdgeKey& keyB )
	{
		if (keyA.bigAddr != keyB.bigAddr) return keyA.bigAddr < keyB.bigAddr;
		return keyA.lowAddr < keyB.lowAddr;
	}
};

struct HorizonEdge {
	XMVECTOR start;
	XMVECTOR end;
};

#include <stdio.h>
EPA_INFO_T CreateEPAInfo(gjkSimplex& gjkInfo, ConvexHull* A, DirectX::XMMATRIX matTRS_A, ConvexHull* B, DirectX::XMMATRIX matTRS_B)
{
	//init
	vector<EPA_FACE_T> faceArr;
	EPA_INFO_T outInfo;
	memset(&outInfo,NULL, sizeof(EPA_INFO_T));

	map<HorizonEdgeKey, HorizonEdge, HorizonKeyCompare > horizonMap;


	for (int i = 0; i < 4; i++)
	{
		XMVECTOR* face = gjkInfo.faces[i];
		XMVECTOR normal = XMVector3Normalize( XMVector3Cross(face[1] - face[0] , face[2] - face[0]));

		if (XMVector3Dot(normal, face[0] - ORIGIN).m128_f32[0] < 0)
		{// 벡터의 노말이 안쪽을 보고 있는 상황

			swap(face[1] , face[2]);
			normal = -normal;
		}
		EPA_FACE_T newFace;

		memcpy(newFace.points , face , sizeof(XMVECTOR) * 3);
		newFace.norm = normal;
		newFace.distance =XMVector3Dot(normal , face[0]).m128_f32[0];
		faceArr.push_back(newFace);
	}
	
	int cnt = 0;
	while (true)
	{
		if (cnt > 1000) 
			assert(cnt > 1000 && "infinity loop!!!");
		cnt++;
		EPA_FACE_T nearestFace = faceArr[0];
		
		for (int i = 1,size = faceArr.size(); i < size; i++)
		{
			if (nearestFace.distance > faceArr[i].distance)
			{
				nearestFace = faceArr[i];
			}
		}

		XMVECTOR farestVector = B->Support(nearestFace.norm, matTRS_B) - A->Support(-nearestFace.norm, matTRS_A);
		float projDist = XMVector3Dot(farestVector, nearestFace.norm).m128_f32[0];
		if (projDist - nearestFace.distance <= EPSILON || cnt > 30 || faceArr.size() > 30)
		{
			outInfo.direction = nearestFace.norm;
			outInfo.distance = nearestFace.distance;
			return outInfo;
		}


		for (int i = 0, size = faceArr.size(); i < size; i++) // 경계면 생성 루프
		{
			EPA_FACE_T* face = &faceArr[i];
			XMVECTOR normal = face->norm;

			if (XMVector3Dot(face->norm, farestVector - face->points[0]).m128_f32[0] >= EPSILON)
			{// 보이는 면 
				for (int j = 0; j < 3; j++)
				{
					HorizonEdge edge;
					edge.start = face->points[j];
					edge.end = face->points[(j + 1) % 3];

					HorizonEdgeKey key;
					if (&face->points[j] < &face->points[(j + 1) % 3])
					{
						key.lowAddr = &face->points[j];
						key.bigAddr = &face->points[(j + 1) % 3];
					}
					else
					{
						key.lowAddr = &face->points[(j + 1) % 3];
						key.bigAddr = &face->points[j];
					}

					if (horizonMap.count(key) == 0)
					{
						horizonMap[key] = edge;
					}
					else
					{
						horizonMap.erase(key);
					}


				}
			}
		}

		for (int i = 0; i < faceArr.size();)// 보이는 면 삭제 
		{
			EPA_FACE_T* face = &faceArr[i];
			XMVECTOR normal = face->norm;

			if (XMVector3Dot(face->norm, farestVector - face->points[0]).m128_f32[0] >= EPSILON)
			{
				faceArr[i] = faceArr.back();
				faceArr.pop_back();
			}
			else
			{
				i++;
			}
		}
	
		for (auto const& horizonEdgePair : horizonMap)// 경계면 기반 페이스 생성
		{
			HorizonEdge edge = horizonEdgePair.second;
			EPA_FACE_T newFace;
			newFace.points[0] = edge.start;
			newFace.points[1] = edge.end;
			newFace.points[2] = farestVector;
			newFace.norm = XMVector3Normalize(XMVector3Cross(newFace.points[1] - newFace.points[0], newFace.points[2] - newFace.points[0]));
			if (XMVector3Dot(newFace.norm, newFace.points[0] - ORIGIN).m128_f32[0] < 0)
			{// 벡터의 노말이 안쪽을 보고 있는 상황

				swap(newFace.points[1], newFace.points[2]);
				newFace.norm = -newFace.norm;
			}

			newFace.distance = XMVector3Dot(newFace.norm, newFace.points[0]).m128_f32[0];
			faceArr.push_back(newFace);
		}
		
		horizonMap.clear();

	}


		
	


	
}

