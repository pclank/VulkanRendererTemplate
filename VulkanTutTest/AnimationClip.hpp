#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <vector>
#include <map>

/// <summary>
/// Scale, Rotation, Time and Translation data for a keyframe
/// </summary>
struct SQT {
	double time;					// Time of Keyframe

	// Keyframe Data
	glm::vec3 scale;
	glm::quat rotation;
	glm::vec3 translation;
};

/// <summary>
/// The Pose list for a single Bone
/// 
/// XXX: The Bone is referred to by name
/// </summary>
struct AnimationPose {
	std::vector<SQT> bonePoses;		// SQTs for each keyframe
	std::string bone_name;			// Name of bone
};

struct AnimationClip {
	double duration;									// Animation duration
	double ticks_per_second;							// Ticks per second
	std::map<std::string, AnimationPose> poseSamples;	// Map from bone name to AnimationPose
	std::string nameID;									// Name of animation (currently not used)
	int n_bones;										// Number of bones in animation
	int max_frames;										// Maximum number of keyframes in a channel
};
