#ifndef MJ_TIME_MANAGER_H
#define MJ_TIME_MANAGER_H

#define DEFAULT_DT_60FPS_MS 16.666666666666668
#define DEFAULT_DT_30FPS_MS 33.333333333333336

class TimeManager
{
public:
	TimeManager();
	~TimeManager();

	void Initialize();
	void Update();

	double GetDeltaTime1d();
	float GetDeltaTime1f();

private:
	long long frequency;
	long long startTick;
	long long endTick;
	double deltaTimeMs;
};

#endif // !MJ_TIME_MANAGER_H
