#include "Timer.hpp"

#include <GLFW/glfw3.h>

Timer::Timer()
	:
	m_prevTime(0.0),
	m_currentTime(0.0),
	m_timeData()
{
	fpss.resize(FPS_SAMPLES);
	deltas.resize(FPS_SAMPLES);
}

void Timer::Tick()
{
	m_prevTime = m_currentTime;
	m_currentTime = glfwGetTime();

	m_timeData.DeltaTime = m_currentTime - m_prevTime;
	m_timeData.FPS = 1.0f / m_timeData.DeltaTime;

	std::rotate(fpss.begin(), fpss.begin() + 1, fpss.end());
	fpss[FPS_SAMPLES - 1] = m_timeData.FPS;

	std::rotate(deltas.begin(), deltas.begin() + 1, deltas.end());
	deltas[FPS_SAMPLES - 1] = m_timeData.DeltaTime;
}