#include "MJ_Time_Manager.h"
#include <Windows.h>

TimeManager::TimeManager()
{
	startTick = 0;
	endTick = 0;
	deltaTimeMs = DEFAULT_DT_60FPS_MS;
}
TimeManager::~TimeManager()
{

}
void TimeManager::Initialize()
{
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	TimeManager::startTick = temp.QuadPart;
	TimeManager::endTick = temp.QuadPart;

	temp.QuadPart = 0LL;
	QueryPerformanceFrequency(&temp);
	TimeManager::frequency = temp.QuadPart;

	TimeManager::deltaTimeMs = DEFAULT_DT_60FPS_MS;

}


void TimeManager::Update()
{

	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	TimeManager::endTick = temp.QuadPart;

	TimeManager::deltaTimeMs = static_cast<double>(TimeManager::endTick - TimeManager::startTick)
				/ static_cast<double>(TimeManager::frequency);
	TimeManager::deltaTimeMs *= 1000.0;

	if (TimeManager::deltaTimeMs > 100.0)
	{	//<게임엔진 아키텍처> 게임이 중단되었다 다시 가동 될 때 delta 타임이 길어져 스파이크 발생할 수 있음
		//위를 방지하기위한 코드( 60 fps delta time 초기값 고정)
		TimeManager::deltaTimeMs = DEFAULT_DT_60FPS_MS;
	}

	TimeManager::startTick = endTick;
}

double TimeManager::GetDeltaTime1d()
{
	return TimeManager::deltaTimeMs;
}

float TimeManager::GetDeltaTime1f()
{
	return static_cast<float>(TimeManager::deltaTimeMs);
}