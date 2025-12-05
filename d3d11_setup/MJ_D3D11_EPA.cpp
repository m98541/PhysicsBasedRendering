#include "MJ_D3D11_EPA.h"
#include <EASTL/map.h>
#define EPSILON 1e-5f

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

bool FaceVisible(const XMVECTOR& faceNomral, const XMVECTOR& point , const XMVECTOR& facePoint0)
{
	XMVECTOR distance = XMVector3Dot(faceNomral, facePoint0);
	return XMVector3Dot(faceNomral , point).m128_f32[0] - distance.m128_f32[0] > EPSILON;
}

bool IsDuplicateVertex(const vector<EPA_FACE_T>& faceArr, const XMVECTOR& point)
{
	float allowableErrorSq = EPSILON * EPSILON;
	for (const auto& face : faceArr) {
		for (int i = 0; i < 3; ++i) {
			XMVECTOR diff = face.points[i] - point;
			XMVECTOR diffSq = XMVector3LengthSq(diff);
			float len = diffSq.m128_f32[0]; 
			if (len <= allowableErrorSq) return true;
		}
	}
	return false;
}

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
		
		XMFLOAT4* face = gjkInfo.faces[i];
		XMVECTOR f0 = XMLoadFloat4(&face[0]);
		XMVECTOR f1 = XMLoadFloat4(&face[1]);
		XMVECTOR f2 = XMLoadFloat4(&face[2]);
	
		XMVECTOR normal = XMVector3Normalize( XMVector3Cross(f1 - f0 , f2 - f0));
		if ((XMVector3Equal(normal ,  XMVectorZero())))
		{
			//printf("초기 오류 gjk level %d  \n", gjkInfo.level);
			double min = XMVector3Length(f0).m128_f32[0];
			double temp;
			double lineLen;
			XMVECTOR tempDir;
			XMVECTOR tempLine;
			XMVECTOR tempLineDir;

			if (!XMVector3Equal(f0, f1))
			{
				temp = XMVector3Length(f1).m128_f32[0];
				tempDir = f1;
				tempLine = f1 - f0;


			}
			else
			{
				temp = XMVector3Length(f2).m128_f32[0];
				tempDir = f2;
				tempLine = f2 - f0;
			}



			tempLineDir = XMVector3Cross(XMVector3Cross(tempLine, -f0), tempLine);
			lineLen = XMVector3Length(tempLineDir).m128_f32[0];

			if (min < temp)
			{
				if (min < lineLen)
				{
					outInfo.direction = XMVector3Normalize(-f0);
					outInfo.distance = min / 10;
					return outInfo;
				}
				else
				{
					outInfo.direction = XMVector3Normalize(tempLineDir);
					outInfo.distance = lineLen / 10;
					return outInfo;
				}
				
			}
			else
			{
				if (temp < lineLen)
				{
					outInfo.direction = XMVector3Normalize(-tempDir);
					outInfo.distance = temp / 10;
					return outInfo;
				}
				else
				{
					outInfo.direction = XMVector3Normalize(tempLineDir);
					outInfo.distance = lineLen / 10;
					return outInfo;
				}

			}
			

		
		}



		if (XMVector3Dot(normal, f0 - ORIGIN).m128_f32[0] < 0)
		{// 벡터의 노말이 안쪽을 보고 있는 상황

			swap(face[1] , face[2]);
			normal = -normal;
		}
		EPA_FACE_T newFace;

		memcpy(newFace.points , face , sizeof(XMVECTOR) * 3);
		newFace.norm = normal;
		newFace.distance =XMVector3Dot(normal , f0).m128_f32[0];
		faceArr.push_back(newFace);
	}
	
	int cnt = 0;
	while (true)
	{
		if (cnt > 1000) 
			assert(cnt > 1000 && "infinity loop");
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

		if (IsDuplicateVertex(faceArr, farestVector)) {
			outInfo.direction = nearestFace.norm;
			outInfo.distance = nearestFace.distance;
			return outInfo;
		}
		if (cnt > 64 || faceArr.size() > 90)
		{
			outInfo.direction = nearestFace.norm;
			outInfo.distance = nearestFace.distance;
			return outInfo;
		}
		


		for (int i = 0, size = faceArr.size(); i < size; i++) // 경계면 생성 루프
		{
			EPA_FACE_T* face = &faceArr[i];
			XMVECTOR normal = face->norm;

			if (FaceVisible(face->norm, farestVector, face->points[0]))
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

			XMVECTOR e1 = newFace.points[1] - newFace.points[0];
			XMVECTOR e2 = newFace.points[2] - newFace.points[0];
			XMVECTOR cross = XMVector3Cross(e1, e2);
			float lenSq = XMVectorGetX(XMVector3LengthSq(cross));
			if (lenSq < EPSILON) {
				
				outInfo.direction = nearestFace.norm;
				if (nearestFace.distance <= EPSILON)
				{
					nearestFace.distance = 0.5F;
				}
				outInfo.distance = nearestFace.distance * 1.5;
				return outInfo;
			}

			XMVECTOR norm = XMVector3Normalize(XMVector3Cross(newFace.points[1] - newFace.points[0], newFace.points[2] - newFace.points[0]));
			
			if (XMVector3Dot(norm, newFace.points[0] - ORIGIN).m128_f32[0] < 0)
			{// 벡터의 노말이 안쪽을 보고 있는 상황

				swap(newFace.points[1], newFace.points[2]);
				norm = -norm;
			}
			double dist = XMVector3Dot(norm, newFace.points[0]).m128_f32[0];

			

			if (dist >= EPSILON && !(XMVector3Equal(norm, XMVectorZero())))
			{
				newFace.norm = norm;
				newFace.distance = dist;
			}
			else
			{
				outInfo.direction = nearestFace.norm;
				outInfo.distance = nearestFace.distance;
				return outInfo;
			}
			
			faceArr.push_back(newFace);
		}
		
		horizonMap.clear();

	}


		
	


	
}

