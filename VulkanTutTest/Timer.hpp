#pragma once
#include <vector>
#include <algorithm>

const uint32_t FPS_SAMPLES = 100;

/// <summary>
/// TimeData used by the Timer class
/// </summary>
struct TimeData
{
	double DeltaTime;
	float FPS;
};

/// <summary>
/// Timer class that manages scene timekeeping
/// </summary>
class Timer
{
public:
	Timer();

	/// <summary>
	/// Tick the timekeeping, updating internal TimeData
	/// </summary>
	void Tick();

	/// <summary>
	/// Get the TimeData for this timer
	/// 
	/// XXX: inlined here because this is a small, often called function
	/// When this is in a CPP file, the linker screams :(
	/// </summary>
	/// <returns></returns>
	inline TimeData GetData()
	{
		return m_timeData;
	}

	inline float* GetFPSS()
	{
		return fpss.data();
	}

private:
	double m_prevTime;
	double m_currentTime;
	std::vector<float> fpss;
	TimeData m_timeData;
};