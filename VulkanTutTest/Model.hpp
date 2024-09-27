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
#include <CubicInterpolation.hpp>

inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
    return glm::transpose(glm::make_mat4(&from.a1));
}

struct Mesh {
	const char* name;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::map<std::string, int> boneMap;			// Map connects node - bone names to indices in m_bones vector
	std::vector<BoneInfo> bones;				// Is indexed by the indices in bone_map
	std::vector<AnimationClip> animations;		// Animations associated with this mesh
	std::string dir;							// Mesh directory
	const aiScene* scene;						        // Points to scene of the mesh. Needed to preserve node tree for bone transformation calculations
	int boneCounter = 0;						// Number of bones in mesh rig
	glm::mat4 inverseTransform;					// Inverse transform matrix for mesh to scene. Possibly only useful if more submeshes are used

	/*uint32_t m_boneVertexCount;
	uint32_t m_skeletonVBO;
	uint32_t m_skeletonVAO;*/

	//Mesh(const char* name, const aiScene* scene) : name(name), scene(scene) {}
	Mesh(const aiScene* sceneP)
	{
        scene = sceneP;
        inverseTransform = glm::inverse(ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation));
	}
    Mesh() {};
};

struct Model {
    //Assimp::Importer importer;					// Assimp Importer for the scene (MUST LIVE!!!)
	const char* name;
	std::vector<Mesh> meshes;
	bool enabled = true;
	uint32_t pipelineIndex = 0;

    // Linear interpolation
	std::vector<glm::mat4> AnimateLI(double currentTime, std::vector<glm::vec3>* boneVertices)
	{
		// TODO: Handle multi-mesh models
		Mesh& mesh = meshes[0];

		// TODO: Switching between animations can be added!

		std::vector<glm::mat4> bone_transforms(mesh.boneCounter);                     // Vector to be passed to vertex shader, containing all bone transforms

		glm::mat4 initial_matrix = glm::mat4(1.0f);

		// Traverse nodes from root node
		TraverseNodeLI(currentTime, mesh.scene->mRootNode, initial_matrix, boneVertices);

		//bone_transforms.resize(mesh.boneCounter);

		// Traverse updated bones
		for (int i = 0; i < mesh.boneCounter; i++)
			bone_transforms[i] = mesh.bones[i].bone_transform;

		// Write bone transforms to vertex shader
        return bone_transforms;
	}

    void TraverseNodeLI(const double currentTime, const aiNode* node, const glm::mat4& parent_transform, std::vector<glm::vec3>* boneVertices)
    {
        // TODO: Handle multi-mesh models
        Mesh& mesh = meshes[0];

        std::string node_name(std::string(node->mName.data));
        glm::mat4 node_transform = ConvertMatrixToGLMFormat(node->mTransformation);

        // Get SQT
        SQT sqt;
        auto sqt_it = mesh.animations.back().poseSamples.find(node_name);
        if (sqt_it != mesh.animations.back().poseSamples.end())
        {
            const std::vector<SQT>& bonePoses = sqt_it->second.bonePoses;
            const int numFrames = static_cast<int>(bonePoses.size());

            // Check if keyframes exist
            if (numFrames > 0)
            {
                // Look for first keyframe
                int frame_index = 0;
                for (int i = 0; i < bonePoses.size() - 1; i++)
                {

                    if (bonePoses[i].time <= currentTime && currentTime < bonePoses[i + 1].time)
                        frame_index = i;
                }

                // Find frames
                int nextFrameIndex = frame_index + 1;

                const SQT& currentFrameSQT = bonePoses[frame_index];
                const SQT& nextFrameSQT = bonePoses[nextFrameIndex];

                // Calculate the interpolation factor
                float t = (currentTime - currentFrameSQT.time) / (nextFrameSQT.time - currentFrameSQT.time);

                // Interpolate scale, rotation and translation
                glm::vec3 scale = currentFrameSQT.scale + t * (nextFrameSQT.scale - currentFrameSQT.scale);
                glm::quat rotation = glm::normalize(glm::slerp(currentFrameSQT.rotation, nextFrameSQT.rotation, t));                    // SLERP IS THE WAY!
                glm::vec3 translation = currentFrameSQT.translation + t * (nextFrameSQT.translation - currentFrameSQT.translation);

                // Add them to the matrices
                glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);
                glm::mat4 rotation_matrix = glm::toMat4(rotation);
                glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), translation);

                node_transform = translation_matrix * rotation_matrix * scale_matrix;
            }
        }

        // Combine with parent
        glm::mat4 global_transformation = parent_transform * node_transform;

        // Get Bone
        auto bone_it = mesh.boneMap.find(node_name);
        if (bone_it != mesh.boneMap.end())
        {
            mesh.bones[bone_it->second].bone_transform = mesh.inverseTransform * global_transformation * mesh.bones[bone_it->second].offsetMatrix;

            if (node->mParent) {
                // If node has a parent, add a visible connection from the parent to the node by placing bone vertices at the joint locations.
                glm::vec4 bonePositionParent = parent_transform * glm::vec4(0, 0, 0, 1);
                glm::vec4 bonePosition = global_transformation * glm::vec4(0, 0, 0, 1);

                boneVertices->push_back(glm::vec3(bonePositionParent.x, bonePositionParent.y, bonePositionParent.z));
                boneVertices->push_back(glm::vec3(bonePosition.x, bonePosition.y, bonePosition.z));
            }
        }

        // Recursion to traverse all nodes
        for (uint32_t i = 0; i < node->mNumChildren; i++)
            TraverseNodeLI(currentTime, node->mChildren[i], global_transformation, boneVertices);
    }

    // Cubic interpolation
    std::vector<glm::mat4> AnimateCI(double currentTime, std::vector<glm::vec3>* boneVertices)
    {
        // TODO: Handle multi-mesh models
        Mesh& mesh = meshes[0];

        // TODO: Switching between animations can be added!

        std::vector<glm::mat4> bone_transforms;                     // Vector to be passed to vertex shader, containing all bone transforms

        glm::mat4 initial_matrix = glm::mat4(1.0f);

        // Traverse nodes from root node
        TraverseNodeCI(currentTime, mesh.scene->mRootNode, initial_matrix, boneVertices);

        //bone_transforms.resize(mesh.boneCounter);

        // Traverse updated bones
        for (int i = 0; i < mesh.boneCounter; i++)
            bone_transforms[i] = mesh.bones[i].bone_transform;

        // Write bone transforms to vertex shader
        return bone_transforms;
    }

    void TraverseNodeCI(const double currentTime, const aiNode* node, const glm::mat4& parent_transform, std::vector<glm::vec3>* boneVertices)
    {
        // TODO: Handle multi-mesh models
        Mesh& mesh = meshes[0];

        std::string node_name(std::string(node->mName.data));
        glm::mat4 node_transform = ConvertMatrixToGLMFormat(node->mTransformation);

        // Get SQT
        SQT sqt;
        auto sqt_it = mesh.animations.back().poseSamples.find(node_name);
        if (sqt_it != mesh.animations.back().poseSamples.end())
        {
            const std::vector<SQT>& bonePoses = sqt_it->second.bonePoses;
            const int numFrames = static_cast<int>(bonePoses.size());

            // Check if keyframes exist
            if (numFrames > 0)
            {
                // Look for first keyframe
                int frame_index = 0;
                for (int i = 0; i < bonePoses.size() - 1; i++)
                {
                    if (bonePoses[i].time <= currentTime && currentTime < bonePoses[i + 1].time)
                        frame_index = i;
                }

                // Find frames
                int prevFrameIndex = std::max(frame_index - 1, 0);
                int nextFrameIndex = frame_index + 1;
                int nextNextFrameIndex = std::min(frame_index + 2, numFrames - 1);
                int nextNextNextFrameIndex = std::min(frame_index + 3, numFrames - 1);

                const SQT& prevFrameSQT = bonePoses[prevFrameIndex];
                const SQT& currentFrameSQT = bonePoses[frame_index];
                const SQT& nextFrameSQT = bonePoses[nextFrameIndex];
                const SQT& nextNextFrameSQT = bonePoses[nextNextFrameIndex];
                const SQT& nextNextNextFrameSQT = bonePoses[nextNextNextFrameIndex];

                // Calculate the interpolation factor
                float t = static_cast<float>((currentTime - currentFrameSQT.time) / (nextFrameSQT.time - currentFrameSQT.time));

                // Perform cubic interpolation for scale, rotation, and translation
                glm::vec3 scale = CubicInterpolate(
                    currentFrameSQT.scale,
                    nextFrameSQT.scale,
                    nextNextFrameSQT.scale,
                    nextNextNextFrameSQT.scale,
                    t
                );

                /*glm::quat rotation = glm::normalize(CubicInterpolate(
                    currentFrameSQT.rotation,
                    nextFrameSQT.rotation,
                    nextNextFrameSQT.rotation,
                    nextNextNextFrameSQT.rotation,
                    t
                ));*/
                glm::quat rotation = glm::normalize(slerp(
                    currentFrameSQT.rotation,
                    nextFrameSQT.rotation,
                    t
                ));

                glm::vec3 translation = CubicInterpolate(
                    currentFrameSQT.translation,
                    nextFrameSQT.translation,
                    nextNextFrameSQT.translation,
                    nextNextNextFrameSQT.translation,
                    t
                );

                // Add them to the matrices
                glm::mat4 scale_matrix = glm::scale(glm::mat4(1.0f), scale);
                glm::mat4 rotation_matrix = glm::toMat4(rotation);
                glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), translation);

                node_transform = translation_matrix * rotation_matrix * scale_matrix;
            }
        }

        // Combine with parent
        glm::mat4 global_transformation = parent_transform * node_transform;

        // Get Bone
        auto bone_it = mesh.boneMap.find(node_name);
        if (bone_it != mesh.boneMap.end())
        {
            mesh.bones[bone_it->second].bone_transform = mesh.inverseTransform * global_transformation * mesh.bones[bone_it->second].offsetMatrix;

            if (node->mParent) {
                // If node has a parent, add a visible connection from the parent to the node by placing bone vertices at the joint locations.
                glm::vec4 bonePositionParent = parent_transform * glm::vec4(0, 0, 0, 1);
                glm::vec4 bonePosition = global_transformation * glm::vec4(0, 0, 0, 1);

                boneVertices->push_back(glm::vec3(bonePositionParent.x, bonePositionParent.y, bonePositionParent.z));
                boneVertices->push_back(glm::vec3(bonePosition.x, bonePosition.y, bonePosition.z));
            }
        }

        // Recursion to traverse all nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            TraverseNodeCI(currentTime, node->mChildren[i], global_transformation, boneVertices);
        }
    }
};
