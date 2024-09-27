#include "Timer.hpp"

#include <GLFW/glfw3.h>

Timer::Timer()
	:
	m_prevTime(0.0),
	m_currentTime(0.0),
	m_timeData()
{
	fpss.resize(FPS_SAMPLES);
}

void Timer::Tick()
{
	m_prevTime = m_currentTime;
	m_currentTime = glfwGetTime();

	m_timeData.DeltaTime = m_currentTime - m_prevTime;
	m_timeData.FPS = 1.0f / m_timeData.DeltaTime;

	// Set up FPS samples
	//for (uint32_t i = 0; i < FPS_SAMPLES - 1; i++)
	//{
	//	// rotate first array to the left
	//	
	//	fpss[i] = 
	//}

	std::rotate(fpss.begin(), fpss.begin() + 1, fpss.end());

	fpss[FPS_SAMPLES - 1] = m_timeData.FPS;
}