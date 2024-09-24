#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

#include <Vertex.hpp>
#include <AnimationClip.hpp>

struct Mesh {
	const char* name;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::map<std::string, int> boneMap;			// Map connects node - bone names to indices in m_bones vector
	std::vector<BoneInfo> bones;				// Is indexed by the indices in bone_map
	std::vector<AnimationClip> animations;		// Animations associated with this mesh
	std::string dir;							// Mesh directory
	//Assimp::Importer importer;					// Assimp Importer for the scene (MUST LIVE!!!)
	const aiScene* scene;						// Points to scene of the mesh. Needed to preserve node tree for bone transformation calculations
	int boneCounter = 0;						// Number of bones in mesh rig
	glm::mat4 inverseTransform;					// Inverse transform matrix for mesh to scene. Possibly only useful if more submeshes are used

	/*uint32_t m_boneVertexCount;
	uint32_t m_skeletonVBO;
	uint32_t m_skeletonVAO;*/

	//Mesh(const char* name, const aiScene* scene) : name(name), scene(scene) {}
	Mesh()
	{
		scene = nullptr;
	}
};

struct Model {
	const char* name;
	std::vector<Mesh> meshes;
	bool enabled = true;
	uint32_t pipelineIndex = 0;
};
