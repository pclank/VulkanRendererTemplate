#pragma once

#include <AnimationClip.hpp>
#include <Model.hpp>

struct AnimationPlayer {
	double animation_time = 0.0;
	bool is_playing = true;

	uint32_t current_anim;					// Animation index
	Model tgt_model;

	AnimationPlayer(uint32_t anim_index, Model model) : current_anim(anim_index), tgt_model(model) {};
	AnimationPlayer() {}

	/// <summary>
	/// Uses global time to set new animation time, while checking that it doesn't surpass its duration, in which case it resets
	/// </summary>
	/// <param name="global_time">: time from global timer</param>
	double UpdateTime(double global_time, float animation_speed)
	{
		// If animation is paused, return
		if (!is_playing)
			return animation_time;

		double new_time = animation_time + global_time * animation_speed;

		// Check whether time exceeds animation duration, then reset
		if (new_time > tgt_model.meshes[0].animations[current_anim].duration)
		{
			ResetTime();

			return animation_time;
		}

		// Update time
		animation_time = new_time;

		return animation_time;
	}

	/// <summary>
	/// Resets time
	/// </summary>
	inline void ResetTime()
	{
		animation_time = 0.0;			// Unexpected functionality is unexpected
	}

	/// <summary>
	/// Updates the animation player with another model and its animation
	/// </summary>
	/// <param name="anim"></param>
	/// <param name="model"></param>
	void SetValues(uint32_t anim_index, Model model)
	{
		tgt_model = model;
		current_anim = anim_index;

		ResetTime();
	}
};
