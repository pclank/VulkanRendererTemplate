#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// ASSIMP
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

// GLM
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// STB Image loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Tiny Obj Loader
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// C++ Libraries
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>

// Local Libraries
#include <Camera.hpp>
#include <Timer.hpp>
#include <Vertex.hpp>
#include <Model.hpp>
#include <GUI.hpp>
#include <AnimationClip.hpp>
#include <AnimationPlayer.hpp>
#include <Skybox.hpp>

// Wrappers
#include <RenderPass.hpp>
#include <Image.hpp>
#include <Swapchain.hpp>
#include <GraphicsPipeline.hpp>

// Macros
#define REQUIRE_GEOM_SHADERS
#define RANK_PHYSICAL_DEVICES
#define TEX_SAMPLER_MODE_REPEAT
#define HIGH_RES
//#define NO_MSAA
//#define ENABLE_CAMERA_ANIM
#define MODEL_IMPORT_DEBUG
#define USE_ASSIMP                  // DO NOT DISABLE!
//#define DISABLE_SKYBOX_ON_WIREFRAME

// Constants
#ifdef HIGH_RES
const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;
#else
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
#endif // HIGH_RES

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";
const std::string NORMAL_PATH = "textures/viking_room_normal.png";
const std::string SKYBOX_PATH = "textures/skybox/";
const std::string MODELS_FOLDER = "models/";
const std::string TEXTURES_FOLDER = "textures/";

const int MAX_FRAMES_IN_FLIGHT = 2;
const size_t MAX_BONES = 120;
const uint32_t MAX_MODELS = 10;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

// Structs
struct PhysicalDeviceContainer { 
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    PhysicalDeviceContainer(VkPhysicalDevice dev)
        : device(dev) {}
};

// Hash function for Vertex struct
namespace std
{
    template<> struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return (((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1 ^
                (hash<glm::vec3>()(vertex.norm) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 boneTransforms[MAX_BONES];
    float time;
    bool explode;
};

struct LightDataUBO {
    glm::vec3 lightPos;
    glm::vec3 camPos;
    bool blinn;
};

// Hardcoded vertices and such
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0,
                                        4, 5, 6, 6, 7, 4};

const std::vector<Vertex> gridVertices = {
    {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
    {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
    {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
    {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}
};

const std::vector<uint16_t> gridIndices = {0, 1, 2, 1, 3, 2};

// Global Functions

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

inline glm::vec3 Assimp2GLMVEC3(aiVector3D& src)
{
    return glm::vec3(src.x, src.y, src.z);
}

inline glm::vec2 Assimp2GLMVEC2(aiVector2D& src)
{
    return glm::vec2(src.x, src.y);
}

inline glm::mat4 AssimpMatrix2GLMMAT4(aiMatrix4x4& src)
{
    return glm::transpose(glm::make_mat4(&src.a1));
}

inline glm::quat Assimp2GLMQUAT(aiQuaternion& src)
{
    return glm::quat(src.w, src.x, src.y, src.z);
}

// Global objects and stuff
Camera cam = Camera(glm::vec3(0.0f, 0.0f, 3.0f));
Timer timer;
GUI gui = GUI(&cam, &timer);
//AnimationPlayer animPlayer = AnimationPlayer(0, nullptr, 0);
std::vector<AnimationPlayer> animPlayers;
std::vector<Assimp::Importer> importers = std::vector<Assimp::Importer>(MAX_MODELS);

// Callbacks
void MouseMovementCallback(GLFWwindow* window, double x_pos, double y_pos);
void MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);             // TODO: Implement!
void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Class
class Renderer {
public:
    void Run()
    {
        InitWindow();
        InitVulkan();
        InitGUI();
        MainLoop();
        Cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkAllocationCallbacks* allocationCallback;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;                                    // Logical device
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    VkSurfaceKHR surface;
    // Swapchain
    Swapchain sc;
    // Render passes
    VkRenderPass renderPass;
    VkRenderPass imguiRenderPass;
    RenderPass tmpRenderPassObj;
    // Descriptor set layouts
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSetLayout lightingDataDescriptorSetLayout;
    VkDescriptorSetLayout gridDescriptorSetLayout;
    VkDescriptorSetLayout imguiDescriptorSetLayout;
    // Graphics Pipelines
    std::vector<VkPipelineLayout> pipelineLayouts;
    std::vector<VkPipeline> graphicsPipelines;
    std::vector<VkPipelineLayout> wireframePipelineLayouts;
    std::vector<VkPipeline> wireframeGraphicsPipelines;
    VkPipelineLayout animatedWireframePipelineLayout;
    VkPipeline animatedWireframeGraphicsPipeline;
    VkPipelineLayout skyboxPipelineLayout;
    VkPipeline skyboxGraphicsPipeline;
    VkPipelineLayout skyboxWireframePipelineLayout;
    VkPipeline skyboxWireframeGraphicsPipeline;
    VkPipelineLayout gridPipelineLayout;
    VkPipeline gridGraphicsPipeline;
    std::vector<VkPipelineLayout> normalPipelineLayouts;
    std::vector<VkPipeline> normalGraphicsPipelines;
    VkPipelineLayout animatedNormalPipelineLayout;
    VkPipeline animatedNormalGraphicsPipeline;
    VkPipelineLayout uiPipelineLayout;
    VkPipeline uiPipeline;
    
    // Test Image
    VkImage testImage;
    VkDeviceMemory testMemory;
    VkImageView testImageView;

    // Frame buffers
    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkFramebuffer> imguiFramebuffers;
    // Command pools
    VkCommandPool commandPool;
    VkCommandPool transCommandPool;
    VkCommandPool imguiCommandPool;
    // Command buffers
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> imguiCommandBuffers;
    // Sync objects
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> uiRenderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;
    // Vertex and index buffers
    std::vector<VkBuffer> vertexBuffers;
    std::vector<VkDeviceMemory> vertexBufferMemories;
    std::vector<VkBuffer> indexBuffers;
    std::vector<VkDeviceMemory> indexBufferMemories;
    VkBuffer skyboxVertexBuffer;
    VkDeviceMemory skyboxVertexBufferMemory;
    VkBuffer gridVertexBuffer;
    VkDeviceMemory gridVertexBufferMemory;
    VkBuffer gridIndexBuffer;
    VkDeviceMemory gridIndexBufferMemory;
    /*std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;*/
    // Uniform buffers
    std::vector<std::vector<VkBuffer>> uniformBuffers;
    std::vector<std::vector<VkDeviceMemory>> uniformBuffersMemory;
    std::vector<std::vector<void*>> uniformBuffersMapped;
    std::vector<std::vector<VkBuffer>> lightingUniformBuffers;
    std::vector<std::vector<VkDeviceMemory>> lightingUniformBuffersMemory;
    std::vector<std::vector<void*>> lightingUniformBuffersMapped;
    std::vector<VkBuffer> skyboxUniformBuffers;
    std::vector<VkDeviceMemory> skyboxUniformBuffersMemory;
    std::vector<void*> skyboxUniformBuffersMapped;
    std::vector<VkBuffer> gridUniformBuffers;
    std::vector<VkDeviceMemory> gridUniformBuffersMemory;
    std::vector<void*> gridUniformBuffersMapped;
    //VkDescriptorPool descriptorPool;
    // Descriptors
    std::vector<VkDescriptorPool> descriptorPools;
    VkDescriptorPool imguiDescriptorPool;
    VkDescriptorPool skyboxDescriptorPool;
    VkDescriptorPool lightingDescriptorPool;
    VkDescriptorPool gridDescriptorPool;
    VkDescriptorPool normalDescriptorPool;
    //std::vector<VkDescriptorSet> descriptorSets;
    std::vector<std::vector<VkDescriptorSet>> descriptorSets;
    std::vector<VkDescriptorSet> skyboxDescriptorSet;
    std::vector<VkDescriptorSet> gridDescriptorSet;
    std::vector<VkDescriptorSet> imguiDescriptorSet;
    std::vector<VkDescriptorSet> normalDescriptorSet;
    uint32_t mipLevels;
    /*VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;*/
    // Images
    std::vector<VkImage> textureImages;
    std::vector<VkDeviceMemory> textureImageMemories;
    std::vector<VkImageView> textureImageViews;
    std::vector<VkSampler> textureSamplers;
    std::vector<VkImage> normalImages;
    std::vector<VkDeviceMemory> normalImageMemories;
    std::vector<VkImageView> normalImageViews;
    std::vector<VkSampler> normalSamplers;
    VkImage depthImage;
    VkDeviceMemory depthMemory;
    VkImageView depthImageView;
    // PLACEHOLDERS
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    // Models
    std::vector<Model> models = std::vector<Model>(MAX_MODELS);
    uint32_t emptyModelIndex = 0;
    // Multisampling
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_8_BIT;
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;
    // Skybox images
    VkImage skyboxImage;
    VkDeviceMemory skyboxImageMemory;
    VkImageView skyboxImageView;
    VkSampler skyboxSampler;

    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);       // Don't create OpenGL context
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Renderer", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        glfwSetKeyCallback(window, KeyboardCallback);
        glfwSetCursorPosCallback(window, MouseMovementCallback);
        glfwSetScrollCallback(window, MouseScrollCallback);
    }

    void InitImporter(Assimp::Importer& importer, const char* file = MODEL_PATH.c_str())
    {
        const aiScene* scene = importer.ReadFile(file,
            aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

        // If the import failed, report it
        if (nullptr == scene)
            throw std::runtime_error("model importing failed!");
    }

    void ImportModel(const uint32_t pipelineIndex, const char* file = MODEL_PATH.c_str())
    {
        Model model;

        Assimp::Importer& importer = importers[emptyModelIndex];
        InitImporter(importer, file);

        const aiScene* scene = importer.GetScene();

        std::cout << "---------------------" << std::endl;

        // Logging
#ifdef MODEL_IMPORT_DEBUG
        std::cout << "Loading scene " << scene->mName.C_Str() << std::endl;
        if (scene->HasAnimations())
            std::cout << "Scene has " << scene->mNumAnimations << " animation(s)!" << std::endl;
        if (scene->HasMeshes())
            std::cout << "Scene has " << scene->mNumMeshes << " mesh(es)!" << std::endl;
        if (scene->hasSkeletons())
            std::cout << "Scene has " << scene->mNumSkeletons << " skeletons!" << std::endl;
        if (scene->HasTextures())
            std::cout << "Scene has " << scene->mNumTextures << " texture(s)!" << std::endl;
#endif // MODEL_IMPORT_DEBUG

        // Empty scene check
        if (!scene->HasMeshes())
            throw std::runtime_error("failed to parse scene: no meshes!");

        model.name = scene->mName.C_Str();
        model.meshes.resize(scene->mNumMeshes);
        model.pipelineIndex = pipelineIndex;

        // Parse model
        for (size_t i = 0; i < scene->mNumMeshes; i++)
        {
            const aiMesh* mesh = scene->mMeshes[i];

            if (mesh->mNumVertices < 1)
                throw std::runtime_error("failed to parse mesh: empty vertices!");

            //Mesh tmpMesh = Mesh(scene->mName.C_Str(), scene);
            model.meshes[i] = Mesh(scene);
            model.meshes[i].name = scene->mName.C_Str();
            //model.meshes[i].scene = scene;

            model.meshes[i].vertices.resize(mesh->mNumVertices);

            // Parse mesh vertices
            for (size_t j = 0; j < mesh->mNumVertices; j++)
            {
                model.meshes[i].vertices[j].pos = Assimp2GLMVEC3(mesh->mVertices[j]);

                // Parse texCoords (first channel)
                model.meshes[i].vertices[j].texCoord = (mesh->mTextureCoords[0]) ? glm::vec2(mesh->mTextureCoords[0][j].x, 1.0f - mesh->mTextureCoords[0][j].y) : glm::vec2(0.0f);

                // Parse normals
                model.meshes[i].vertices[j].norm = Assimp2GLMVEC3(mesh->mNormals[j]);

                // Parse tangents and bitangents
                if (mesh->HasTangentsAndBitangents())
                {
                    model.meshes[i].vertices[j].tangent = Assimp2GLMVEC3(mesh->mTangents[j]);
                    model.meshes[i].vertices[j].biTangent = Assimp2GLMVEC3(mesh->mBitangents[j]);
                }
                else
                {
                    model.meshes[i].vertices[j].tangent = glm::vec3(1.0f);
                    model.meshes[i].vertices[j].biTangent = glm::vec3(1.0f);
                }
            }

            // Parse indices
            for (size_t j = 0; j < mesh->mNumFaces; j++)
                for (size_t z = 0; z < mesh->mFaces[j].mNumIndices; z++)
                    model.meshes[i].indices.push_back(mesh->mFaces[j].mIndices[z]);

            // Parse bones!
            ExtractBoneWeightForVertices(model.meshes[i].vertices, mesh, scene, model);

            std::cout << "Loaded mesh " << mesh->mName.C_Str() << " Successfully with " << model.meshes[i].vertices.size() << " vertices, and " << model.meshes[i].indices.size() << " triangles!" << std::endl;
        }

        // Parse animations
        ParseAnimations(scene, model);

        //models.push_back(model);
        models[emptyModelIndex] = model;

        // Map to Animation Player
        if (!model.meshes[0].animations.empty())
        {
            animPlayers.resize(animPlayers.size() + 1);
            //animPlayers.back().SetValues(0, &models.back(), models.size() - 1);
            animPlayers.back().SetValues(0, &models[emptyModelIndex], emptyModelIndex);
        }
    }

    void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, const aiMesh* mesh, const aiScene* scene, Model& model)
    {
        auto& boneInfoMap = model.meshes[0].boneMap;
        int& boneCount = model.meshes[0].boneCounter;

        std::cout << "Found " << mesh->mNumBones << " Bones in Mesh " << mesh->mName.C_Str() << std::endl;

        for (int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
        {
            int boneId = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            if (boneInfoMap.find(boneName) == boneInfoMap.end())
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = boneCount;
                newBoneInfo.offsetMatrix = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                //boneInfoMap[boneName] = boneCount;
                boneInfoMap.insert({ boneName, boneCount });
                model.meshes[0].bones.push_back(newBoneInfo);
                boneId = boneCount;
                boneCount++;
            }
            else
            {
                //boneId = boneInfoMap[boneName];
                boneId = boneInfoMap.find(boneName)->second;
            }
            assert(boneId != -1);
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;
            for (int weightIndex = 0; weightIndex < numWeights; weightIndex++)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= vertices.size());
                SetVertexBoneData(vertices[vertexId], boneId, weight);
            }
        }
    }

    void SetVertexBoneData(Vertex& vertex, int boneId, float weight)
    {
        uint32_t bone_number = vertex.bone_num;                     // The array index where the bone will be added

        if (bone_number > 3)
            throw std::runtime_error("ERROR::Exceeded MAX_BONES!");

        // Add Bone ID and Weight
        vertex.boneIDs[bone_number] = boneId;
        vertex.weights[bone_number] = weight;

        // Increment Bone Number in vertex
        vertex.bone_num++;
    }

    void ParseAnimations(const aiScene* scene, Model& model)
    {
        // Check for animations
        if (scene->HasAnimations())
        {
            uint32_t n_animations = scene->mNumAnimations;
            std::cout << "Found " << n_animations << " animations in Scene!" << std::endl;

            // Retrieve animations and parse them to AnimationClip objects
            for (unsigned int i = 0; i < n_animations; i++)
            {
                aiAnimation* current_animation = scene->mAnimations[i];

                // TODO: Currently considers same size of all channels (realistic?)
                // Parse channels
                int max_frames = 0;
                std::map<std::string, AnimationPose> poses;
                for (unsigned int j = 0; j < current_animation->mNumChannels; j++)
                {
                    aiNodeAnim* current_channel = current_animation->mChannels[j];

                    std::string channel_bone_name = std::string(current_channel->mNodeName.data);           // Get name of affected node

                    // Check keyframe number
                    if (current_channel->mNumPositionKeys > max_frames)
                        max_frames = current_channel->mNumPositionKeys;
                    if (current_channel->mNumRotationKeys > max_frames)
                        max_frames = current_channel->mNumRotationKeys;
                    if (current_channel->mNumScalingKeys > max_frames)
                        max_frames = current_channel->mNumScalingKeys;

                    // Parse SQTs of channel
                    std::vector<SQT> sqts;
                    for (unsigned int k = 0; k < current_channel->mNumPositionKeys; k++)
                    {
                        SQT new_sqt;
                        new_sqt.time = current_channel->mScalingKeys[k].mTime / current_animation->mTicksPerSecond;
                        new_sqt.scale = Assimp2GLMVEC3(current_channel->mScalingKeys[k].mValue);
                        new_sqt.translation = Assimp2GLMVEC3(current_channel->mPositionKeys[k].mValue);
                        new_sqt.rotation = Assimp2GLMQUAT(current_channel->mRotationKeys[k].mValue);

                        sqts.push_back(new_sqt);
                    }

                    AnimationPose new_pose;
                    new_pose.bone_name = channel_bone_name;
                    new_pose.bonePoses = sqts;

                    // Add AnimationPose to map
                    poses.insert({ channel_bone_name, new_pose });
                }

                AnimationClip new_animation_clip = AnimationClip(std::string(current_animation->mName.data), model.meshes[0].bones.size(),
                    max_frames, current_animation->mDuration / current_animation->mTicksPerSecond, current_animation->mTicksPerSecond, poses);
                model.meshes[0].animations.push_back(new_animation_clip);
            }
        }
        else
            std::cout << "No animations found!" << std::endl;
    }

    void InitGUI()
    {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls


        //CreateImGuiDescriptorPool();

        VkDescriptorPoolSize pool_sizes[] =
        {
                {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000} };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Create DescriptorPool for m_ImGuiDescriptorPool failed!");

        CreateImguiCommandPool();
        CreateImGuiCommandBuffers();
        CreateImGuiFramebuffers();
        CreateImGuiDescriptorSetLayout();
        CreateImGuiDescriptorSet();
        CreateUIGraphicsPipeline();

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsLight();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        init_info.QueueFamily = FindQueueFamilies(physicalDevice).graphicsFamily.value();
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imguiDescriptorPool;
        init_info.RenderPass = imguiRenderPass;
        //init_info.RenderPass = renderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
        init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
        //init_info.MSAASamples = msaaSamples;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        ImGui_ImplVulkan_Init(&init_info);

        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(imguiCommandPool);
        ImGui_ImplVulkan_CreateFontsTexture();
        EndSingleTimeCommands(commandBuffer, imguiCommandPool, graphicsQueue);

        /*for (uint32_t i = 0; i < descriptorSets.size(); i++)
            descriptorSets[i] = ImGui_ImplVulkan_AddTexture(textureSampler, colorImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);*/
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
    {
        auto app = reinterpret_cast<Renderer*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void LoadTexture(const uint32_t modelIndex, const char* file = TEXTURE_PATH.c_str(), bool isNormal = false)
    {
        CreateTextureImage(modelIndex, file, isNormal);
        CreateTextureImageView(modelIndex, isNormal);
        CreateTextureSampler(modelIndex, isNormal);
    }

    void AddModel(const uint32_t pipelineIndex = 0, bool passLightingData = false, const char* modelFile = MODEL_PATH.c_str(), const char* diffuseTextureFile = TEXTURE_PATH.c_str(), const char* normalTextureFile = NORMAL_PATH.c_str())
    {
        if (emptyModelIndex == models.size() - 1)
            throw std::runtime_error("Too many models! Increase vector size!");

        ImportModel(pipelineIndex, modelFile);
        LoadTexture(emptyModelIndex, diffuseTextureFile);
        LoadTexture(emptyModelIndex, normalTextureFile, true);
        CreateVertexBuffer(emptyModelIndex);
        CreateIndexBuffer(emptyModelIndex);
        CreateUniformBuffers(emptyModelIndex);
        if (passLightingData)
        {
            if (lightingUniformBuffers.empty())
                CreateLightingUniformBuffers(emptyModelIndex);
            CreateLightingDataDescriptorPool(emptyModelIndex);
        }
        else
            CreateDescriptorPool(emptyModelIndex);

        if (models[emptyModelIndex].meshes[0].animations.empty())
            CreateWireframeGraphicsPipeline("shaders/vert.spv", "shaders/frag.spv", passLightingData);

        CreateDescriptorSets(emptyModelIndex, passLightingData);
        CreateNormalsGraphicsPipeline(passLightingData);

        emptyModelIndex++;
    }

    void AddSkybox(const char* folder = SKYBOX_PATH.c_str())
    {
        LoadCubemap(folder);
        CreateSkyboxVertexBuffer();
        CreateSkyboxUniformBuffer();
        CreateSkyboxDescriptorPool();
        CreateSkyboxDescriptorSet();
    }

    void AddGrid()
    {
        CreateGridVertexBuffer();
        CreateGridIndexBuffer();
        CreateGridUniformBuffer();
        CreateGridDescriptorPool();
        CreateGridDescriptorSet();
    }

    void InitVulkan()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateImGuiRenderPass();
        CreateDescriptorSetLayout();
        CreateLightingDataDescriptorSetLayout();
        CreateGridDescriptorSetLayout();
        CreateGraphicsPipeline();
        CreateGraphicsPipeline("shaders/linear_skinning_vert.spv", "shaders/linear_skinning_frag.spv", true);
        CreateGraphicsPipeline("shaders/blinn_phong_vert.spv", "shaders/blinn_phong_frag.spv", true);
        CreateSkyboxGraphicsPipeline("shaders/skybox_vert.spv", "shaders/skybox_frag.spv");
        CreateSkyboxWireframeGraphicsPipeline();
        CreateGridGraphicsPipeline();
        CreateCommandPools();
        CreateColorResources();
        CreateDepthResources();
        CreateFramebuffers();
#ifdef USE_ASSIMP
        CreateTestImage();
        AddModel(2, true);
        AddModel(0, false, "models/suzanne.obj", "textures/marble.png", "textures/marble_normal.png");
        AddModel(2, true, "models/wicker_basket_02_2k.fbx", "models/textures/wicker_basket_02_diff_2k.jpg", "models/textures/wicker_basket_02_nor_dx_2k.png");
        //AddModel(1, true, "models/flair_edited.fbx", "textures/body_diffuse.png", "textures/body_normal.png");
        AddModel(1, true, "models/Capoeira_working.fbx", "textures/brick.png", "textures/brick_normal.png");
        AddModel(1, true, "models/Dancing Twerk_working.fbx", "textures/brick.png", "textures/brick_normal.png");
        AddModel(1, true, "models/Hokey Pokey_working.fbx", "textures/brick.png", "textures/brick_normal.png");

        //AddModel(1, true, "models/ymca.fbx", "textures/parasiteZombie_body_diffuse.png", "textures/parasiteZombie_body_normal.bmp");
        //AddModel(1, true, "models/wiggly.fbx", "textures/plaster.jpg", "textures/plaster_normal.png");
        // AddModel(1, false, "models/boy_animated.fbx", "textures/plaster.jpg", "textures/plaster_normal.png");
        //AddModel(1, "models/bob_lamp.fbx", "textures/plaster.jpg");
        //AddModel(1, "models/deer.fbx", "textures/plaster.jpg");
        //AddModel(1, "models/female_doctor.fbx", "textures/plaster.jpg");
#else
        LoadModel();
        CreateTextureImage();
        CreateTextureImageView();
        CreateTextureSampler();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
#endif // USE_ASSIMP
        CreateAnimatedWireframeGraphicsPipeline("shaders/linear_skinning_vert.spv", "shaders/linear_skinning_frag.spv", true);
        CreateAnimatedNormalGraphicsPipeline();
        gui.nModels = emptyModelIndex;
        gui.models = models.data();
        gui.animationPlayers = animPlayers.data();
        gui.nAnimationPlayers = animPlayers.size();
        gui.Setup();
        //AddSkybox();
        AddSkybox("textures/Yokohama3/");
        AddGrid();
        CreateNormalDescriptorPool();
        CreateNormalDescriptorSet();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void MainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            timer.Tick();

            glfwPollEvents();

            gui.Render();

            cam.UpdateVelocity(timer.GetData().DeltaTime);
            cam.MoveCamera(timer.GetData().DeltaTime);

            DrawFrame();
        }

        // Wait for device to finish operations before exiting
        vkDeviceWaitIdle(device);
    }

    void DrawFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult image_result = vkAcquireNextImageKHR(device, sc.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // Check whether swap chain recreation is required
        if (image_result == VK_ERROR_OUT_OF_DATE_KHR || image_result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            RecreateSwapChain();
            return;
        }
        else if (image_result != VK_SUCCESS)
            throw std::runtime_error("failed to present swap chain image!");

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        // Update model uniform buffers
        //for (size_t i = 0; i < models.size(); i++)
        for (size_t i = 0; i < emptyModelIndex; i++)
        {
            UpdateUniformBuffer(i, currentFrame);

            // Update lighting data UBO
            if (models[i].pipelineIndex == 2)
                UpdateLightingDataUniformBuffer(i, currentFrame);
        }

        // Update skybox uniform buffer
        UpdateSkyboxUniformBuffer(currentFrame);

        // Update grid uniform buffer
        UpdateGridUniformBuffer(currentFrame);

        // Submit command buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("failed to submit draw command buffer!");

        // UI render command buffer
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        VkSemaphore uiWaitSemaphores[] = { renderFinishedSemaphores[currentFrame] };
        submitInfo.pWaitSemaphores = uiWaitSemaphores;
        VkSemaphore uiSignalSemaphores[] = { uiRenderFinishedSemaphores[currentFrame] };
        submitInfo.pSignalSemaphores = uiSignalSemaphores;

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer!");

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = imguiRenderPass;
        //renderPassInfo.renderPass = renderPass;
        //renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.framebuffer = imguiFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = sc.extent;

        // Testing
        /*VkImageCopy region{};
        region.extent = { sc.extent.width, sc.extent.height, 1 };
        region.srcOffset = { 0, 0, 0 };
        region.dstOffset = { 0, 0, 0 };

        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.dstSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.layerCount = 1;*/

        VkImageBlit region{};

        region.srcOffsets[0].x = 0;
        region.srcOffsets[0].y = 0;
        region.srcOffsets[0].z = 0;
        region.dstOffsets[0].x = 0;
        region.dstOffsets[0].y = 0;
        region.dstOffsets[0].z = 0;
        region.srcOffsets[1].x = sc.extent.width;
        region.srcOffsets[1].y = sc.extent.height;
        region.srcOffsets[1].z = 1;
        region.dstOffsets[1].x = sc.extent.width;
        region.dstOffsets[1].y = sc.extent.height;
        region.dstOffsets[1].z = 1;
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.dstSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.dstSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.layerCount = 1;

        TransitionImageLayoutCmd(sc.images[currentFrame], 1, sc.format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT, commandBuffers[currentFrame], 1);

        TransitionImageLayoutCmd(textureImages[1], 1, sc.format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT, commandBuffers[currentFrame], 1);

        vkCmdBlitImage(commandBuffers[currentFrame], sc.images[currentFrame], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            textureImages[1], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);

        TransitionImageLayoutCmd(sc.images[currentFrame], 1, sc.format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT, commandBuffers[currentFrame], 1);

        TransitionImageLayoutCmd(textureImages[1], 1, sc.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_ASPECT_COLOR_BIT, commandBuffers[currentFrame], 1);

        // Begin UI render pass
        vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkBuffer vertexBuffersRend[] = { gridVertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffersRend, offsets);
        vkCmdBindIndexBuffer(commandBuffers[currentFrame], gridIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(sc.extent.width);
        viewport.height = static_cast<float>(sc.extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;
        vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

        vkCmdBindPipeline(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, uiPipeline);
        vkCmdBindDescriptorSets(commandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, uiPipelineLayout, 0, 1, imguiDescriptorSet.data(), 0, nullptr);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[currentFrame]);

        vkCmdEndRenderPass(commandBuffers[currentFrame]);
        if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("failed to render ui!");

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);

        // Presentation
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        //presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.pWaitSemaphores = uiSignalSemaphores;
        VkSwapchainKHR swapChains[] = { sc.swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        // Check whether swap chain recreation is required
        if (image_result == VK_ERROR_OUT_OF_DATE_KHR || image_result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            RecreateSwapChain();
            return;
        }
        else if (image_result != VK_SUCCESS)
            throw std::runtime_error("failed to present swap chain image!");

        vkQueuePresentKHR(presentQueue, &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void Cleanup()
    {
        // Clean up GUI
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (enableValidationLayers)
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);

        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyCommandPool(device, transCommandPool, nullptr);
        vkDestroyCommandPool(device, imguiCommandPool, nullptr);

        for (size_t i = 0; i < graphicsPipelines.size(); i++)
            vkDestroyPipeline(device, graphicsPipelines[i], nullptr);

        for (size_t i = 0; i < pipelineLayouts.size(); i++)
            vkDestroyPipelineLayout(device, pipelineLayouts[i], nullptr);

        for (size_t i = 0; i < wireframeGraphicsPipelines.size(); i++)
            vkDestroyPipeline(device, wireframeGraphicsPipelines[i], nullptr);
        for (size_t i = 0; i < wireframePipelineLayouts.size(); i++)
            vkDestroyPipelineLayout(device, wireframePipelineLayouts[i], nullptr);

        vkDestroyPipeline(device, skyboxGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
        vkDestroyPipeline(device, skyboxWireframeGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, skyboxWireframePipelineLayout, nullptr);
        vkDestroyPipeline(device, animatedWireframeGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, animatedWireframePipelineLayout, nullptr);
        vkDestroyPipeline(device, gridGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, gridPipelineLayout, nullptr);
        vkDestroyPipeline(device, animatedNormalGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, animatedNormalPipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, uiPipelineLayout, nullptr);
        vkDestroyPipeline(device, uiPipeline, nullptr);

        for (size_t i = 0; i < normalGraphicsPipelines.size(); i++)
            vkDestroyPipeline(device, normalGraphicsPipelines[i], nullptr);

        for (size_t i = 0; i < normalPipelineLayouts.size(); i++)
            vkDestroyPipelineLayout(device, normalPipelineLayouts[i], nullptr);

        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyRenderPass(device, imguiRenderPass, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, uiRenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        CleanupSwapChain();

        for (size_t i = 0; i < uniformBuffers.size(); i++)
        {
            for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
            {
                vkDestroyBuffer(device, uniformBuffers[i][j], nullptr);
                vkFreeMemory(device, uniformBuffersMemory[i][j], nullptr);
            }
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(device, skyboxUniformBuffers[i], nullptr);
            vkFreeMemory(device, skyboxUniformBuffersMemory[i], nullptr);
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroyBuffer(device, gridUniformBuffers[i], nullptr);
            vkFreeMemory(device, gridUniformBuffersMemory[i], nullptr);
        }

        for (size_t i = 0; i < lightingUniformBuffers.size(); i++)
        {
            for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
            {
                vkDestroyBuffer(device, lightingUniformBuffers[i][j], nullptr);
                vkFreeMemory(device, lightingUniformBuffersMemory[i][j], nullptr);
            }
        }

        for (size_t i = 0; i < textureImages.size(); i++)
        {
            vkDestroyImageView(device, textureImageViews[i], nullptr);
            vkDestroySampler(device, textureSamplers[i], nullptr);
            vkDestroyImage(device, textureImages[i], nullptr);
            vkFreeMemory(device, textureImageMemories[i], nullptr);
        }

        for (size_t i = 0; i < normalImages.size(); i++)
        {
            vkDestroyImageView(device, normalImageViews[i], nullptr);
            vkDestroySampler(device, normalSamplers[i], nullptr);
            vkDestroyImage(device, normalImages[i], nullptr);
            vkFreeMemory(device, normalImageMemories[i], nullptr);
        }

        vkDestroyImageView(device, skyboxImageView, nullptr);
        vkDestroySampler(device, skyboxSampler, nullptr);
        vkDestroyImage(device, skyboxImage, nullptr);
        vkFreeMemory(device, skyboxImageMemory, nullptr);

        for (size_t i = 0; i < descriptorPools.size(); i++)
            vkDestroyDescriptorPool(device, descriptorPools[i], nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, lightingDataDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, gridDescriptorSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, imguiDescriptorSetLayout, nullptr);

        vkDestroyDescriptorPool(device, skyboxDescriptorPool, nullptr);
        vkDestroyDescriptorPool(device, gridDescriptorPool, nullptr);
        vkDestroyDescriptorPool(device, lightingDescriptorPool, nullptr);
        vkDestroyDescriptorPool(device, normalDescriptorPool, nullptr);
        vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);
        
        // Clear all vertex and index buffers and memories
        for (size_t i = 0; i < vertexBuffers.size(); i++)
        {
            vkDestroyBuffer(device, vertexBuffers[i], nullptr);
            vkFreeMemory(device, vertexBufferMemories[i], nullptr);
            vkDestroyBuffer(device, indexBuffers[i], nullptr);
            vkFreeMemory(device, indexBufferMemories[i], nullptr);
        }

        vkDestroyBuffer(device, skyboxVertexBuffer, nullptr);
        vkFreeMemory(device, skyboxVertexBufferMemory, nullptr);
        vkDestroyBuffer(device, gridVertexBuffer, nullptr);
        vkFreeMemory(device, gridVertexBufferMemory, nullptr);
        vkDestroyBuffer(device, gridIndexBuffer, nullptr);
        vkFreeMemory(device, gridIndexBufferMemory, nullptr);

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);

        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    void CleanupSwapChain()
    {
        // Clean up color resources
        vkDestroyImageView(device, colorImageView, nullptr);
        vkDestroyImage(device, colorImage, nullptr);
        vkFreeMemory(device, colorImageMemory, nullptr);

        // Clean up depth resources
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthMemory, nullptr);

        vkDestroySwapchainKHR(device, sc.swapChain, nullptr);

        for (auto framebuffer : swapChainFramebuffers)
            vkDestroyFramebuffer(device, framebuffer, nullptr);

        for (auto imageView : sc.imageViews)
            vkDestroyImageView(device, imageView, nullptr);

        for (auto frameBuffer : imguiFramebuffers)
            vkDestroyFramebuffer(device, frameBuffer, nullptr);
    }

    void RecreateSwapChain()
    {
        // Check whether window is minimized
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            //glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        CreateSwapChain();
        CreateImageViews();
        CreateColorResources();
        CreateDepthResources();
        CreateFramebuffers();

        // We also need to take care of the UI
        ImGui_ImplVulkan_SetMinImageCount(MAX_FRAMES_IN_FLIGHT);
        /*CreateImGuiRenderPass();
        CreateImGuiFramebuffers();
        CreateImGuiCommandBuffers();*/
    }

    void CreateInstance()
    {
        // Check validation layers
        if (enableValidationLayers && !CheckValidationLayerSupport())
            throw std::runtime_error("validation layers requested, but not available!");

        // Application information
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Global extensions and validation layers
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> glfwExtensions = GetRequiredExtensions();  // Get extensions from GLFW

        createInfo.enabledExtensionCount = static_cast<uint32_t>(glfwExtensions.size());
        createInfo.ppEnabledExtensionNames = glfwExtensions.data();
        createInfo.enabledLayerCount = (enableValidationLayers) ? static_cast<uint32_t>(validationLayers.size()) : 0;
        createInfo.ppEnabledLayerNames = (enableValidationLayers) ? validationLayers.data() : nullptr;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else
            createInfo.pNext = nullptr;

        // Create instance
        if (vkCreateInstance(&createInfo, allocationCallback, &instance) != VK_SUCCESS)
            throw std::runtime_error("failed to create instance!");

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);

        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        std::cout << "Available extensions:\n";
        for (const auto& extension : extensions)
            std::cout << '\t' << extension.extensionName << '\n';

        bool missing_extensions = false;
        std::cout << "Required extensions:\n";
        for (int i = 0; i < static_cast<uint32_t>(glfwExtensions.size()); i++)
        {
            std::cout << '\t' << glfwExtensions[i] << '\n';
            missing_extensions = true;
            for (const auto& extension : extensions)
            {
                if (std::strcmp(extension.extensionName, glfwExtensions[i]))
                {
                    missing_extensions = false;
                    break;
                }
            }

            if (missing_extensions)
                std::cerr << "Missing extension: " << glfwExtensions[i] << std::endl;
        }
    }

    bool CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (!strcmp(layerName, layerProperties.layerName))
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
                return false;
        }

    }
    
    std::vector<const char*> GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        // Conditionally add Debug Utils extension
        if (enableValidationLayers)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void SetupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
            throw std::runtime_error("failed to set up debug messenger!");
    }

    void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = DebugCallback;
        createInfo.pUserData = nullptr; // Optional
    }

    void PickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0)
            throw std::runtime_error("failed to find GPUs with Vulkan support!");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

#ifdef RANK_PHYSICAL_DEVICES
        std::multimap<int, PhysicalDeviceContainer> candidates;     // Ordered map

        for (const auto& device : devices)
        {
            PhysicalDeviceContainer deviceContainer(device);
            vkGetPhysicalDeviceProperties(device, &deviceContainer.properties);
            vkGetPhysicalDeviceFeatures(device, &deviceContainer.features);

            int deviceScore = RateDeviceSuitability(deviceContainer);
            candidates.insert(std::make_pair(deviceScore, deviceContainer));
        }

        if (candidates.rbegin()->first > 0)
            physicalDevice = candidates.rbegin()->second.device;
        else
            throw std::runtime_error("failed to find a suitable GPU!");
#else
        // Check for suitable physical device
        for (const auto& device : devices)
        {
            if (IsDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }
#endif // RANK_PHYSICAL_DEVICES

        // If no suitable physical device found
        if (physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("failed to find a suitable GPU!");
    }

    bool IsDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        QueueFamilyIndices queueIndices = FindQueueFamilies(device);

        bool extensionsSupported = CheckDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

#ifdef REQUIRE_GEOM_SHADERS
        return deviceFeatures.geometryShader && deviceFeatures.samplerAnisotropy && (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && queueIndices.IsComplete() && extensionsSupported && swapChainAdequate;
#else
        return (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && queueIndices.IsComplete() && extensionsSupported && swapChainAdequate;
#endif // REQUIRE_GEOM_SHADERS
    }

    int RateDeviceSuitability(PhysicalDeviceContainer device)
    {
        int score = 0;

        if (device.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;

        score += device.properties.limits.maxImageDimension2D;

#ifdef REQUIRE_GEOM_SHADERS
        if (!device.features.geometryShader)
            return 0;
#endif // REQUIRE_GEOM_SHADERS
        if (!device.features.samplerAnisotropy)
            return 0;

#ifndef NO_MSAA
        VkSampleCountFlagBits msaaSamplesNew = GetMaxUsableSampleCount(device);
        score += msaaSamples;

        if (msaaSamplesNew > msaaSamples)
        {
            msaaSamples = msaaSamplesNew;
            std::cout << "MSAA " << msaaSamples << "X" << std::endl;
        }
#endif // NO_MSAA

        return score;
    }

    bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions)
            requiredExtensions.erase(extension.extensionName);

        return requiredExtensions.empty();
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        
        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.graphicsFamily = i;

            if (!(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
                indices.transferFamily = i;

            VkBool32 presentSupport;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
                indices.presentFamily = i;

            if (indices.IsComplete())
                break;

            i++;
        }

        return indices;
    }

    void CreateLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Get Device features
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.sampleRateShading = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE;
#ifdef REQUIRE_GEOM_SHADERS
        deviceFeatures.geometryShader = VK_TRUE;
#endif // REQUIRE_GEOM_SHADERS

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
            createInfo.enabledLayerCount = 0;

        // Instantiate device
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
            throw std::runtime_error("failed to create logical device!");

        // Retrieve queue handles
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
        vkGetDeviceQueue(device, indices.transferFamily.value(), 0, &transferQueue);
    }

    void CreateSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        // Capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // Formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // Present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return availableFormat;
        }

        // TODO: Consider ranking rather than returning the first format available!
        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                return availablePresentMode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void CreateSwapChain()
    {
        sc = Swapchain(device, physicalDevice, surface, window);
    }

    void CreateImageViews()
    {
        sc.imageViews.resize(sc.images.size());

        for (size_t i = 0; i < sc.images.size(); i++)
        {
            sc.imageViews[i] = CreateImageView(sc.images[i], 1, sc.format, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    void CreateGraphicsPipeline(const char* vertShaderFile = "shaders/vert.spv", const char* fragShaderFile = "shaders/frag.spv", bool passLightingData = false, const char* geomShaderFile = "shaders/geom.spv")
    {
        pipelineLayouts.resize(pipelineLayouts.size() + 1);
        graphicsPipelines.resize(graphicsPipelines.size() + 1);

        VkDescriptorSetLayout& descriptorLayout = (passLightingData) ? lightingDataDescriptorSetLayout : descriptorSetLayout;
        GraphicsPipeline tmpGraphPipeline(device, sc, msaaSamples, pipelineLayouts.back(), descriptorLayout, renderPass,
            vertShaderFile, fragShaderFile, geomShaderFile, std::to_string(graphicsPipelines.size() - 1),
            graphicsPipelines.back());
    }

    void CreateWireframeGraphicsPipeline(const char* vertShaderFile = "shaders/vert.spv", const char* fragShaderFile = "shaders/frag.spv", bool passLightingData = false)
    {
        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        if (passLightingData)
            pipelineLayoutInfo.pSetLayouts = &lightingDataDescriptorSetLayout;
        else
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        wireframePipelineLayouts.resize(wireframePipelineLayouts.size() + 1);
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &wireframePipelineLayouts.back()) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = wireframePipelineLayouts.back();
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        wireframeGraphicsPipelines.resize(wireframeGraphicsPipelines.size() + 1);
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &wireframeGraphicsPipelines.back()) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        models[models.size() - 1].wireframeIndex = wireframeGraphicsPipelines.size() - 1;

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void CreateSkyboxGraphicsPipeline(const char* vertShaderFile = "shaders/skybox_vert.spv", const char* fragShaderFile = "shaders/skybox_frag.spv")
    {
        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Skybox::GetBindingDescription();
        auto attributeDescriptions = Skybox::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Skybox::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        /*depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;*/
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &skyboxPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = skyboxPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &skyboxGraphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void CreateSkyboxWireframeGraphicsPipeline(const char* vertShaderFile = "shaders/skybox_vert.spv", const char* fragShaderFile = "shaders/skybox_frag.spv")
    {  
        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Skybox::GetBindingDescription();
        auto attributeDescriptions = Skybox::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Skybox::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        /*depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;*/
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &skyboxWireframePipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = skyboxPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &skyboxWireframeGraphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void CreateAnimatedWireframeGraphicsPipeline(const char* vertShaderFile = "shaders/linear_skinning_vert.spv", const char* fragShaderFile = "shaders/linear_skinning_frag.spv", bool passLightingData = false)
    {
        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        if (passLightingData)
            pipelineLayoutInfo.pSetLayouts = &lightingDataDescriptorSetLayout;
        else
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        //pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &animatedWireframePipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = animatedWireframePipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &animatedWireframeGraphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void CreateGridGraphicsPipeline(const char* vertShaderFile = "shaders/grid_vert.spv", const char* fragShaderFile = "shaders/grid_frag.spv")
    {
        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;

        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        /*depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;*/
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &gridDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &gridPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = gridPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &gridGraphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void CreateNormalsGraphicsPipeline(bool passLightingData = false)
    {
        const char* vertShaderFile = "shaders/normDisplay_vert.spv";
        const char* fragShaderFile = "shaders/normDisplay_frag.spv";
        const char* geomShaderFile = "shaders/normDisplay_geom.spv";

        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);
        auto geomShaderCode = ReadFile(geomShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);
        VkShaderModule geomShaderModule = CreateShaderModule(device, geomShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // Geometry shader to pipeline
        VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
        geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        geomShaderStageInfo.module = geomShaderModule;
        geomShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        if (passLightingData)
            pipelineLayoutInfo.pSetLayouts = &lightingDataDescriptorSetLayout;
        else
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        normalPipelineLayouts.resize(normalPipelineLayouts.size() + 1);
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &normalPipelineLayouts.back()) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 3;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = normalPipelineLayouts.back();
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        normalGraphicsPipelines.resize(normalGraphicsPipelines.size() + 1);
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &normalGraphicsPipelines.back()) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, geomShaderModule, nullptr);
    }

    void CreateAnimatedNormalGraphicsPipeline()
    {
        const char* vertShaderFile = "shaders/linear_skinning_norm_vert.spv";
        const char* fragShaderFile = "shaders/normDisplay_frag.spv";
        const char* geomShaderFile = "shaders/normDisplay_geom.spv";

        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);
        auto geomShaderCode = ReadFile(geomShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);
        VkShaderModule geomShaderModule = CreateShaderModule(device, geomShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        // Geometry shader to pipeline
        VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
        geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
        geomShaderStageInfo.module = geomShaderModule;
        geomShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_TRUE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        /*if (passLightingData)
            pipelineLayoutInfo.pSetLayouts = &lightingDataDescriptorSetLayout;
        else
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;*/
        pipelineLayoutInfo.pSetLayouts = &lightingDataDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &animatedNormalPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 3;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = animatedNormalPipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &animatedNormalGraphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, geomShaderModule, nullptr);
    }

    void CreateUIGraphicsPipeline()
    {
        const char* vertShaderFile = "shaders/grid_vert.spv";
        const char* fragShaderFile = "shaders/grid_frag.spv";

        auto vertShaderCode = ReadFile(vertShaderFile);
        auto fragShaderCode = ReadFile(fragShaderFile);

        VkShaderModule vertShaderModule = CreateShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(device, fragShaderCode);

        // Vertex shader to pipeline
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        // Fragment shader to pipeline
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Vertices
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        auto bindingDescription = Vertex::GetBindingDescription();
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::GetAttributeDescriptions().size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)sc.extent.width;
        viewport.height = (float)sc.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        // Scissor
        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = sc.extent;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        //multisampling.rasterizationSamples = msaaSamples;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 0.2f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        // Depth & Stencil testing
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f; // Optional
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; // Optional
        depthStencil.back = {}; // Optional

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &imguiDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &uiPipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create pipeline layout!");

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = uiPipelineLayout;
        //pipelineInfo.layout = gridPipelineLayout;
        pipelineInfo.renderPass = imguiRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &uiPipeline) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics pipeline!");

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void CreateRenderPass()
    {
        const std::vector<VkFormat> formats = { sc.format, FindDepthFormat(), sc.format };
        const std::vector<VkSampleCountFlagBits> samples = { msaaSamples, msaaSamples, VK_SAMPLE_COUNT_1_BIT };
        const std::vector<VkAttachmentLoadOp> loadOps = { VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_DONT_CARE };
        const std::vector<VkAttachmentStoreOp> storeOps = { VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_ATTACHMENT_STORE_OP_STORE };
        const std::vector<VkImageLayout> initialLayouts = { VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_UNDEFINED };
        //const std::vector<VkImageLayout> finalLayouts = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
        const std::vector<VkImageLayout> finalLayouts = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        tmpRenderPassObj = RenderPass(device, VK_PIPELINE_BIND_POINT_GRAPHICS, formats, samples, loadOps, storeOps, initialLayouts, finalLayouts,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            0,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            "Main render pass", renderPass);
    }

    void CreateImGuiRenderPass()
    {
        const std::vector<VkFormat> formats = { sc.format };
        const std::vector<VkSampleCountFlagBits> samples = { VK_SAMPLE_COUNT_1_BIT };
        const std::vector<VkAttachmentLoadOp> loadOps = { VK_ATTACHMENT_LOAD_OP_LOAD };
        const std::vector<VkAttachmentStoreOp> storeOps = { VK_ATTACHMENT_STORE_OP_STORE };
        const std::vector<VkImageLayout> initialLayouts = { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
        const std::vector<VkImageLayout> finalLayouts = { VK_IMAGE_LAYOUT_PRESENT_SRC_KHR };
        RenderPass tmpImGuiRenderPass = RenderPass(device, VK_PIPELINE_BIND_POINT_GRAPHICS, formats, samples, loadOps, storeOps, initialLayouts, finalLayouts,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            "ImGui render pass", imguiRenderPass);
    }

    void CreateFramebuffers()
    {
        swapChainFramebuffers.resize(sc.images.size());

        // Framebuffer for each image view
        for (size_t i = 0; i < sc.imageViews.size(); i++)
        {
            std::array<VkImageView, 3> attachments = {
                colorImageView,
                depthImageView,
                sc.imageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = sc.extent.width;
            framebufferInfo.height = sc.extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer!");
        }
    }

    void CreateImGuiFramebuffers()
    {
        imguiFramebuffers.resize(sc.imageViews.size());

        VkImageView attachment[1];
        VkFramebufferCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        info.renderPass = imguiRenderPass;
        info.attachmentCount = 1;
        info.pAttachments = attachment;
        info.width = sc.extent.width;
        info.height = sc.extent.height;
        info.layers = 1;
        for (uint32_t i = 0; i < sc.imageViews.size(); i++)
        {
            attachment[0] = sc.imageViews[i];
            if (vkCreateFramebuffer(device, &info, nullptr, &imguiFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create framebuffer!");
        }
    }

    void CreateCommandPools()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

        // Graphics command pool
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics command pool!");

        // Transfer command pool
        poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &transCommandPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create transfer command pool");
    }

    void CreateImguiCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

        // IMGUI command pool
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &imguiCommandPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create graphics command pool!");
    }

    void CreateCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers!");
    }

    void CreateImGuiCommandBuffers()
    {
        imguiCommandBuffers.resize(sc.imageViews.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = imguiCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)imguiCommandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, imguiCommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate command buffers!");
    }

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("failed to begin recording command buffer!");

        // Render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = sc.extent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};            // Clear to furthest possible depth
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // Begin render pass
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Render skybox
#ifdef DISABLE_SKYBOX_ON_WIREFRAME
        if (gui.skybox_flag && !gui.wireframe_flag)
#else
        if (gui.skybox_flag)
#endif // DISABLE_SKYBOX_ON_WIREFRAME
        {
            // Bind graphics pipeline
            if (gui.wireframe_flag)
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxWireframeGraphicsPipeline);
            else
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxGraphicsPipeline);

            VkBuffer vertexBuffersRend[] = { skyboxVertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersRend, offsets);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(sc.extent.width);
            viewport.height = static_cast<float>(sc.extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = sc.extent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            if (gui.wireframe_flag)
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxWireframePipelineLayout, 0, 1, skyboxDescriptorSet.data(), 0, nullptr);
            else
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout, 0, 1, skyboxDescriptorSet.data(), 0, nullptr);

            vkCmdDraw(commandBuffer, static_cast<uint32_t>(108), 1, 0, 0);
        }

        // Render grid
        if (gui.grid_flag)
        {
            // Bind graphics pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gridGraphicsPipeline);

            VkBuffer vertexBuffersRend[] = { gridVertexBuffer };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersRend, offsets);
            vkCmdBindIndexBuffer(commandBuffer, gridIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(sc.extent.width);
            viewport.height = static_cast<float>(sc.extent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = sc.extent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gridPipelineLayout, 0, 1, gridDescriptorSet.data(), 0, nullptr);

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(gridIndices.size()), 1, 0, 0, 0);
        }

        // Render models
        //for (size_t i = 0; i < models.size(); i++)
        for (size_t i = 0; i < emptyModelIndex; i++)
        {
            // Only render model if enabled
            if (!models[i].enabled)
                continue;

            // Render mesh
            for (auto& mesh : models[i].meshes)
            {
                // Bind graphics pipeline
                if (gui.wireframe_flag && !mesh.animations.empty())
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, animatedWireframeGraphicsPipeline);
                else if (gui.wireframe_flag && mesh.animations.empty())
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframeGraphicsPipelines[models[i].wireframeIndex]);
                else
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelines[models[i].pipelineIndex]);

                VkBuffer vertexBuffersRend[] = { vertexBuffers[mesh.vertexBufferIndex] };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffersRend, offsets);
                vkCmdBindIndexBuffer(commandBuffer, indexBuffers[mesh.vertexBufferIndex], 0, VK_INDEX_TYPE_UINT32);

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(sc.extent.width);
                viewport.height = static_cast<float>(sc.extent.height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = { 0, 0 };
                scissor.extent = sc.extent;
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayouts[models[i].pipelineIndex], 0, 1, &descriptorSets[i][currentFrame], 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

                // Normal drawing
                if (!gui.normals_flag)
                    continue;

                // Bind graphics pipeline for normal drawing
                if (!mesh.animations.empty())
                {
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, animatedNormalGraphicsPipeline);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, animatedNormalPipelineLayout, 0, 1, &descriptorSets[i][currentFrame], 0, nullptr);
                }
                else
                {
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normalGraphicsPipelines[i]);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, normalPipelineLayouts[i], 0, 1, &descriptorSets[i][currentFrame], 0, nullptr);
                }

                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
            }
        }

        // Record dear imgui primitives into command buffer
        //ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        // End render pass
        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
            throw std::runtime_error("failed to record command buffer!");
    }

    void CreateSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        uiRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &uiRenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
                throw std::runtime_error("failed to create sync objects for a frame!");
        }
    }

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        //bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;

        QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.transferFamily.value() };
        bufferInfo.queueFamilyIndexCount = 2;
        bufferInfo.pQueueFamilyIndices = queueFamilyIndices;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("failed to create vertex buffer!");

        // Memory allocation
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate vertex buffer memory!");

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    void CreateVertexBuffer(const size_t modelIndex)
    {
        for (size_t i = 0; i < models[modelIndex].meshes.size(); i++)
        {
#ifdef USE_ASSIMP
            VkDeviceSize bufferSize = sizeof(models[modelIndex].meshes[i].vertices[0]) * models[modelIndex].meshes[i].vertices.size();
#else
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
#endif // USE_ASSIMP

            // Staging buffer
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);

            // Map memory
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
#ifdef USE_ASSIMP
            memcpy(data, models[modelIndex].meshes[i].vertices.data(), static_cast<size_t>(bufferSize));
#else
            memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
#endif // USE_ASSIMP
            vkUnmapMemory(device, stagingBufferMemory);

            // Resize buffer and memory vectors
            vertexBuffers.resize(vertexBuffers.size() + 1);
            vertexBufferMemories.resize(vertexBufferMemories.size() + 1);

            models[modelIndex].meshes[i].vertexBufferIndex = vertexBuffers.size() - 1;

            CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                vertexBuffers[models[modelIndex].meshes[i].vertexBufferIndex], vertexBufferMemories[models[modelIndex].meshes[i].vertexBufferIndex]);

            CopyBuffer(stagingBuffer, vertexBuffers[models[modelIndex].meshes[i].vertexBufferIndex], bufferSize);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }
    }

    void CreateIndexBuffer(const size_t modelIndex)
    {
        for (size_t i = 0; i < models[modelIndex].meshes.size(); i++)
        {
#ifdef USE_ASSIMP
            VkDeviceSize bufferSize = sizeof(models[modelIndex].meshes[i].indices[0]) * models[modelIndex].meshes[i].indices.size();
#else
            VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
#endif // USE_ASSIMP

            // Staging buffer
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);

            // Map memory
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
#ifdef USE_ASSIMP
            memcpy(data, models[modelIndex].meshes[i].indices.data(), static_cast<size_t>(bufferSize));
#else
            memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
#endif // USE_ASSIMP
            vkUnmapMemory(device, stagingBufferMemory);

            // Resize buffer and memory vectors
            indexBuffers.resize(indexBuffers.size() + 1);
            indexBufferMemories.resize(indexBufferMemories.size() + 1);

            CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indexBuffers[models[modelIndex].meshes[i].vertexBufferIndex], indexBufferMemories[models[modelIndex].meshes[i].vertexBufferIndex]);

            CopyBuffer(stagingBuffer, indexBuffers[models[modelIndex].meshes[i].vertexBufferIndex], bufferSize);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }
    }

    void CreateGridIndexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(gridIndices[0]) * gridIndices.size();

        // Staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        // Map memory
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, gridIndices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            gridIndexBuffer, gridIndexBufferMemory);

        CopyBuffer(stagingBuffer, gridIndexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(transCommandPool);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = bufferSize;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        EndSingleTimeCommands(commandBuffer, transCommandPool, transferQueue);
    }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void CreateSkyboxVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(float) * 108;

        // Staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        // Map memory
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, box_vertices, static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            skyboxVertexBuffer, skyboxVertexBufferMemory);

        CopyBuffer(stagingBuffer, skyboxVertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void CreateGridVertexBuffer()
    {
        VkDeviceSize bufferSize = sizeof(Vertex) * gridVertices.size();

        // Staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        // Map memory
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, gridVertices.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device, stagingBufferMemory);

        CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            gridVertexBuffer, gridVertexBufferMemory);

        CopyBuffer(stagingBuffer, gridVertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void CreateDescriptorSetLayout()
    {
        // UBO descriptor layout
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        //uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // Combined image sampler descriptor layout
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }

    void CreateGridDescriptorSetLayout()
    {
        // UBO descriptor layout
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &gridDescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }

    void CreateImGuiDescriptorSetLayout()
    {
        // UBO descriptor layout
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &imguiDescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }

    void CreateLightingDataDescriptorSetLayout()
    {
        // UBO descriptor layout
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        //uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // Combined image sampler descriptor layout for diffuse
        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // Lighting data UBO descriptor layout
        VkDescriptorSetLayoutBinding lightingUBOLayoutBinding{};
        lightingUBOLayoutBinding.binding = 2;
        lightingUBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lightingUBOLayoutBinding.descriptorCount = 1;
        lightingUBOLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        lightingUBOLayoutBinding.pImmutableSamplers = nullptr; // Optional

        // Combined image sampler descriptor layout for normal
        VkDescriptorSetLayoutBinding normalSamplerLayoutBinding{};
        normalSamplerLayoutBinding.binding = 3;
        normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalSamplerLayoutBinding.descriptorCount = 1;
        normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        normalSamplerLayoutBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, samplerLayoutBinding, lightingUBOLayoutBinding, normalSamplerLayoutBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &lightingDataDescriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor set layout!");
    }

    void CreateUniformBuffers(const size_t modelIndex)
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        uniformBuffers.resize(emptyModelIndex + 1);
        uniformBuffersMemory.resize(emptyModelIndex + 1);
        uniformBuffersMapped.resize(emptyModelIndex + 1);
        uniformBuffers[modelIndex].resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory[modelIndex].resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped[modelIndex].resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                uniformBuffers[modelIndex][i], uniformBuffersMemory[modelIndex][i]);

            // Persistent mapping
            vkMapMemory(device, uniformBuffersMemory[modelIndex][i], 0, bufferSize, 0, &uniformBuffersMapped[modelIndex][i]);
        }
    }

    void CreateSkyboxUniformBuffer()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        skyboxUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        skyboxUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        skyboxUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                skyboxUniformBuffers[i], skyboxUniformBuffersMemory[i]);

            // Persistent mapping
            vkMapMemory(device, skyboxUniformBuffersMemory[i], 0, bufferSize, 0, &skyboxUniformBuffersMapped[i]);
        }
    }

    void CreateGridUniformBuffer()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);
        gridUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        gridUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        gridUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                gridUniformBuffers[i], gridUniformBuffersMemory[i]);

            // Persistent mapping
            vkMapMemory(device, gridUniformBuffersMemory[i], 0, bufferSize, 0, &gridUniformBuffersMapped[i]);
        }
    }

    void CreateLightingUniformBuffers(const uint32_t modelIndex)
    {
        VkDeviceSize bufferSize = sizeof(LightDataUBO);
        lightingUniformBuffers.resize(emptyModelIndex + 1);
        lightingUniformBuffersMemory.resize(emptyModelIndex + 1);
        lightingUniformBuffersMapped.resize(emptyModelIndex + 1);
        lightingUniformBuffers[modelIndex].resize(MAX_FRAMES_IN_FLIGHT);
        lightingUniformBuffersMemory[modelIndex].resize(MAX_FRAMES_IN_FLIGHT);
        lightingUniformBuffersMapped[modelIndex].resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                lightingUniformBuffers[modelIndex][i], lightingUniformBuffersMemory[modelIndex][i]);

            // Persistent mapping
            vkMapMemory(device, lightingUniformBuffersMemory[modelIndex][i], 0, bufferSize, 0, &lightingUniformBuffersMapped[modelIndex][i]);
        }
    }

    void UpdateUniformBuffer(const size_t modelIndex, uint32_t currentFrame)
    {
        UniformBufferObject ubo{};
#ifdef ENABLE_CAMERA_ANIM
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();

        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), sc.extent.width / (float)sc.extent.height, 0.1f, 10.0f);   // without sc.extent we get wrong aspect ratio if resize happens
#else
        ubo.proj = cam.GetCurrentProjectionMatrix(sc.extent.width, sc.extent.height);
        ubo.view = cam.GetCurrentViewMatrix();

        ubo.model = glm::mat4(1.0f);
        if (modelIndex == 0)
        {
            ubo.model = glm::rotate(
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        if (modelIndex == 1)
        {
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();

            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            ubo.model = glm::rotate(ubo.model, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        }

        ubo.model = glm::translate(ubo.model, glm::vec3(gui.model_translations[modelIndex][0], gui.model_translations[modelIndex][1], gui.model_translations[modelIndex][2]));
        ubo.model = glm::scale(ubo.model, glm::vec3(gui.model_scales[modelIndex]));

#endif // ENABLE_CAMERA_ANIM
        ubo.proj[1][1] *= -1;   // Flip sign of scaling factor

        // Animate model
        bool reset_animations = false;
        for (auto& animPlayer : animPlayers)
        {
            if (modelIndex == animPlayer.modelIndex && !animPlayer.tgt_model->meshes[0].animations.empty())
            {
                animPlayer.is_playing = gui.play_animation_flag;

                std::vector<glm::vec3> skeletonBones;                   // TODO: NOT USED NOW!
                animPlayer.UpdateTime(timer.GetData().DeltaTime, gui.animation_speed);

                // Checks for animation reset
                /*if (gui.reset_animation_flag)
                {
                    animPlayer.ResetTime();
                    reset_animations = true;
                }*/

                std::vector<glm::mat4> boneTranforms;
                if (gui.cubic_interpolation_flag)
                    boneTranforms = animPlayer.tgt_model->AnimateCI(animPlayer.animation_time, &skeletonBones);
                else
                    boneTranforms = animPlayer.tgt_model->AnimateLI(animPlayer.animation_time, &skeletonBones);
                memcpy(ubo.boneTransforms, boneTranforms.data(), boneTranforms.size() * sizeof(glm::mat4));
            }
        }

        //ubo.time = glfwGetTime();
        ubo.explode = static_cast<bool>(gui.explode_flags[modelIndex]);
        ubo.time = gui.explosion_rates[modelIndex];

        memcpy(uniformBuffersMapped[modelIndex][currentFrame], &ubo, sizeof(ubo));
    }

    void UpdateSkyboxUniformBuffer(uint32_t currentFrame)
    {
        UniformBufferObject ubo{};

        ubo.model = glm::mat4(1.0f);
        ubo.proj = cam.GetCurrentProjectionMatrix(sc.extent.width, sc.extent.height);
        ubo.view = glm::mat4(glm::mat3(cam.GetCurrentViewMatrix()));                        // Remove translation from view matrix

        ubo.proj[1][1] *= -1;   // Flip sign of scaling factor

        memcpy(skyboxUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }

    void UpdateLightingDataUniformBuffer(const size_t modelIndex, uint32_t currentFrame)
    {
        LightDataUBO lightUBO{};

        lightUBO.blinn = gui.blinn_flag;
        lightUBO.lightPos = glm::vec3(gui.light_pos[0], gui.light_pos[1], gui.light_pos[2]);
        lightUBO.camPos = cam.position;

        memcpy(lightingUniformBuffersMapped[0][currentFrame], &lightUBO, sizeof(lightUBO));
    }

    void UpdateGridUniformBuffer(uint32_t currentFrame)
    {
        UniformBufferObject ubo{};

        ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.model = glm::scale(ubo.model, glm::vec3(200.0f));
        ubo.proj = cam.GetCurrentProjectionMatrix(sc.extent.width, sc.extent.height);
        ubo.view = cam.GetCurrentViewMatrix();

        ubo.proj[1][1] *= -1;   // Flip sign of scaling factor

        memcpy(gridUniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
    }

    void CreateDescriptorPool(const size_t modelIndex)
    {
        descriptorPools.resize(descriptorPools.size() + 1);

        std::array<VkDescriptorPoolSize, 2> poolSizes;
        // UBO
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        // Sampler
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        //poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPools[modelIndex]) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void CreateImGuiDescriptorPool()
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool));
    }

    void CreateSkyboxDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes;
        // UBO
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        // Sampler
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        //poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &skyboxDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void CreateGridDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 1> poolSizes;
        // UBO
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        //poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &gridDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void CreateNormalDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 1> poolSizes;
        // UBO
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &normalDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void CreateLightingDataDescriptorPool(const uint32_t modelIndex)
    {
        descriptorPools.resize(descriptorPools.size() + 1);

        std::array<VkDescriptorPoolSize, 4> poolSizes;
        // UBO
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        // Diffuse Sampler
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        // LightData UBO
        poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        // Normal Sampler
        poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        //poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        /*if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &lightingDescriptorPool) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");*/

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPools[modelIndex]) != VK_SUCCESS)
            throw std::runtime_error("failed to create descriptor pool!");
    }

    void CreateDescriptorSets(const size_t modelIndex, bool passLightingData = false)
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        std::vector<VkDescriptorSetLayout> lightingDataLayouts(MAX_FRAMES_IN_FLIGHT, lightingDataDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        /*if (passLightingData)
            allocInfo.descriptorPool = lightingDescriptorPool;
        else
            allocInfo.descriptorPool = descriptorPools[modelIndex];*/
        allocInfo.descriptorPool = descriptorPools[modelIndex];
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        if (passLightingData)
            allocInfo.pSetLayouts = lightingDataLayouts.data();
        else
            allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(descriptorSets.size() + 1);
        descriptorSets[modelIndex].resize(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets[modelIndex].data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // UBO
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[modelIndex][i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // Sampler
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = textureSamplers[modelIndex];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageViews[modelIndex];

            VkDescriptorBufferInfo lightingBufferInfo{};

            // Update configurations
            if (passLightingData)
            {
                // Lighting data UBO
                lightingBufferInfo.buffer = lightingUniformBuffers[0][i];
                lightingBufferInfo.offset = 0;
                lightingBufferInfo.range = sizeof(LightDataUBO);

                // Normal map sampler
                VkDescriptorImageInfo normalImageInfo{};
                normalImageInfo.sampler = normalSamplers[modelIndex];
                normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                normalImageInfo.imageView = normalImageViews[modelIndex];

                std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[modelIndex][i];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = &bufferInfo;
                descriptorWrites[0].pImageInfo = nullptr; // Optional
                descriptorWrites[0].pTexelBufferView = nullptr; // Optional

                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstSet = descriptorSets[modelIndex][i];
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pBufferInfo = nullptr;   // Optional
                descriptorWrites[1].pImageInfo = &imageInfo;
                descriptorWrites[1].pTexelBufferView = nullptr; // Optional

                descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[2].dstSet = descriptorSets[modelIndex][i];
                descriptorWrites[2].dstBinding = 2;
                descriptorWrites[2].dstArrayElement = 0;
                descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[2].descriptorCount = 1;
                descriptorWrites[2].pBufferInfo = &lightingBufferInfo;
                descriptorWrites[2].pImageInfo = nullptr;
                descriptorWrites[2].pTexelBufferView = nullptr;

                descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[3].dstSet = descriptorSets[modelIndex][i];
                descriptorWrites[3].dstBinding = 3;
                descriptorWrites[3].dstArrayElement = 0;
                descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[3].descriptorCount = 1;
                descriptorWrites[3].pBufferInfo = nullptr;
                descriptorWrites[3].pImageInfo = &normalImageInfo;
                descriptorWrites[3].pTexelBufferView = nullptr;

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
            }
            else
            {
                std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
                descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[0].dstSet = descriptorSets[modelIndex][i];
                descriptorWrites[0].dstBinding = 0;
                descriptorWrites[0].dstArrayElement = 0;
                descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[0].descriptorCount = 1;
                descriptorWrites[0].pBufferInfo = &bufferInfo;
                descriptorWrites[0].pImageInfo = nullptr; // Optional
                descriptorWrites[0].pTexelBufferView = nullptr; // Optional

                descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[1].dstSet = descriptorSets[modelIndex][i];
                descriptorWrites[1].dstBinding = 1;
                descriptorWrites[1].dstArrayElement = 0;
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pBufferInfo = nullptr;   // Optional
                descriptorWrites[1].pImageInfo = &imageInfo;
                descriptorWrites[1].pTexelBufferView = nullptr; // Optional

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
            }
        }
    }

    void CreateSkyboxDescriptorSet()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = skyboxDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        skyboxDescriptorSet.resize(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

        if (vkAllocateDescriptorSets(device, &allocInfo, skyboxDescriptorSet.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // UBO
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = skyboxUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // Sampler
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = skyboxSampler;
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = skyboxImageView;

            // Update configurations
            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = skyboxDescriptorSet[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = skyboxDescriptorSet[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = nullptr;   // Optional
            descriptorWrites[1].pImageInfo = &imageInfo;
            descriptorWrites[1].pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void CreateGridDescriptorSet()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, gridDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = gridDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        gridDescriptorSet.resize(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

        if (vkAllocateDescriptorSets(device, &allocInfo, gridDescriptorSet.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // UBO
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = gridUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // Update configurations
            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = gridDescriptorSet[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void CreateImGuiDescriptorSet()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, imguiDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = imguiDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        imguiDescriptorSet.resize(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

        if (vkAllocateDescriptorSets(device, &allocInfo, imguiDescriptorSet.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // UBO
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = gridUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // Update configurations
            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = imguiDescriptorSet[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void CreateNormalDescriptorSet()
    {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, gridDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = normalDescriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        normalDescriptorSet.resize(static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT));

        if (vkAllocateDescriptorSets(device, &allocInfo, normalDescriptorSet.data()) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate descriptor sets!");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // UBO
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = skyboxUniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            // Update configurations
            std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = normalDescriptorSet[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;
            descriptorWrites[0].pImageInfo = nullptr; // Optional
            descriptorWrites[0].pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    void CreateTextureImage(const size_t modelIndex, const char* file = TEXTURE_PATH.c_str(), bool isNormal = false)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        // TODO: Handle for multiple textures!
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight))) + 1);

        if (!pixels)
            throw std::runtime_error("failed to load texture image!");

        // Staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<uint32_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        if (isNormal)
        {
            normalImages.resize(normalImages.size() + 1);
            normalImageMemories.resize(normalImageMemories.size() + 1);

            CreateImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                normalImages[modelIndex], normalImageMemories[modelIndex]);

            TransitionImageLayout(normalImages[modelIndex], mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

            CopyBufferToImage(stagingBuffer, normalImages[modelIndex], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

            //TransitionImageLayout(textureImage, mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

            GenerateMipmaps(normalImages[modelIndex], VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels);
        }
        else
        {
            textureImages.resize(textureImages.size() + 1);
            textureImageMemories.resize(textureImageMemories.size() + 1);

            CreateImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                textureImages[modelIndex], textureImageMemories[modelIndex]);

            TransitionImageLayout(textureImages[modelIndex], mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

            CopyBufferToImage(stagingBuffer, textureImages[modelIndex], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

            //TransitionImageLayout(textureImage, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

            GenerateMipmaps(textureImages[modelIndex], VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
        }

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        // Image creation
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        imageInfo.flags = 0; // Optional

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
            throw std::runtime_error("failed to create image!");

        // Allocate memory for image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate image memory!");

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void LoadCubemap(const char* folder = SKYBOX_PATH.c_str())
    {
        int texWidth, texHeight, texChannels;
        std::vector<stbi_uc*> pixelVector;
        for (uint32_t i = 0; i < 6; i++)
        {
            std::string fileName = folder + postfixes[i];
            stbi_uc* pixels = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if (!pixels)
                throw std::runtime_error("failed to load cubemap image!");

            pixelVector.push_back(pixels);
        }

        const VkDeviceSize layerSize = texWidth * texHeight * 4;        // Size of each layer
        const VkDeviceSize imageSize = layerSize * 6;                   // Total size

        // Silly solution
        stbi_uc* fillPixels = (stbi_uc*)malloc(imageSize);
        for (size_t i = 0; i < 6; i++)
        {
            for (size_t j = 0; j < static_cast<uint32_t>(layerSize); j++)
                fillPixels[i * layerSize + j] = pixelVector[i][j];
        }

        // Staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);

        //memcpy(data, pixelVector.data(), static_cast<uint32_t>(imageSize));
        //for (uint32_t i = 0; i < pixelVector.size(); i++)
        //    memcpy((void*)(reinterpret_cast<uint32_t>(data) + (layerSize * i)), pixelVector[i], static_cast<uint32_t>(layerSize));
        memcpy(data, fillPixels, static_cast<uint32_t>(imageSize));

        vkUnmapMemory(device, stagingBufferMemory);

        for (uint32_t i = 0; i < pixelVector.size(); i++)
            stbi_image_free(pixelVector[i]);

        CreateCubeImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            skyboxImage, skyboxImageMemory);

        TransitionImageLayout(skyboxImage, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, 6);

        CopyBufferToImage(stagingBuffer, skyboxImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 6);

        //TransitionImageLayout(skyboxImage, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        GenerateMipmaps(skyboxImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels, 6);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        // ImageView and sampler creation
        CreateCubemapImageView();
        CreateCubemapSampler();
    }

    void CreateCubeImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
    {
        // Image creation
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 6;                          // Cube faces count as array layers in Vulkan
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
            throw std::runtime_error("failed to create image!");

        // Allocate memory for image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
            throw std::runtime_error("failed to allocate image memory!");

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void CreateCubemapImageView()
    {
        skyboxImageView = CreateImageView(skyboxImage, mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE, 6);
    }

    void CreateCubemapSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
#ifdef TEX_SAMPLER_MODE_REPEAT
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
#else
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
#endif // TEX_SAMPLER_MODE_REPEAT
        samplerInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);

        if (vkCreateSampler(device, &samplerInfo, nullptr, &skyboxSampler) != VK_SUCCESS)
            throw std::runtime_error("failed to create sampler!");
    }

    void GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels, uint32_t layerCount = 1)
    {
        // Linear filter support
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if ((!formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            throw std::runtime_error("physical device does not support current mipmap generation or texture image format does not support linear blitting!");

        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = layerCount;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = layerCount;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // Last level
        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        EndSingleTimeCommands(commandBuffer, commandPool, graphicsQueue);
    }

    VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool, VkQueue queue)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void TransitionImageLayout(VkImage image, uint32_t mipLevels, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspect, uint32_t layerCount = 1)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPool);

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspect;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = layerCount;

        // Access masks and pipeline stages
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

            if (HasStencilComponent(format))
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else
            throw std::invalid_argument("unsupported layout transition!");

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        EndSingleTimeCommands(commandBuffer, commandPool, graphicsQueue);
    }

    void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount = 1)
    {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(commandPool);

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = layerCount;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(
            commandBuffer,
            buffer,
            image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        EndSingleTimeCommands(commandBuffer, commandPool, graphicsQueue);
    }

    void CreateTextureImageView(const size_t modelIndex, bool isNormal = false)
    {
        if (isNormal)
        {
            normalImageViews.resize(normalImageViews.size() + 1);
            normalImageViews[modelIndex] = CreateImageView(normalImages[modelIndex], mipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
        }
        else
        {
            textureImageViews.resize(textureImageViews.size() + 1);
            textureImageViews[modelIndex] = CreateImageView(textureImages[modelIndex], mipLevels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
        }
    }

    VkImageView CreateImageView(VkImage image, uint32_t mipLevels, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = layerCount;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
            throw std::runtime_error("failed to create texture image view!");

        return imageView;
    }

    void CreateTextureSampler(const size_t modelIndex, bool isNormal = false)
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
#ifdef TEX_SAMPLER_MODE_REPEAT
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
#else
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
#endif // TEX_SAMPLER_MODE_REPEAT
        samplerInfo.anisotropyEnable = VK_TRUE;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        std::cout << "Max Anisotropy is: " << properties.limits.maxSamplerAnisotropy << std::endl;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);

        if (isNormal)
        {
            normalSamplers.resize(normalSamplers.size() + 1);

            if (vkCreateSampler(device, &samplerInfo, nullptr, &normalSamplers[modelIndex]) != VK_SUCCESS)
                throw std::runtime_error("failed to create sampler!");
        }
        else
        {
            textureSamplers.resize(textureSamplers.size() + 1);

            if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSamplers[modelIndex]) != VK_SUCCESS)
                throw std::runtime_error("failed to create sampler!");
        }
    }
    
    Image depthImageTmp;

    void CreateDepthResources()
    {
        VkFormat depthFormat = FindDepthFormat();

        depthImageTmp = Image(device, physicalDevice, depthFormat, sc.extent.width, sc.extent.height, 1, msaaSamples,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

        depthImage = depthImageTmp.image;
        depthImageView = depthImageTmp.imageView;
        depthMemory = depthImageTmp.imageMemory;

        /*CreateImage(sc.extent.width, sc.extent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthMemory);

        depthImageView = CreateImageView(depthImage, 1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);*/

        // NOT REALLY NEEDED
        TransitionImageLayout(depthImage, 1, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    }

    Image colorImageTmp;

    void CreateColorResources()
    {
        VkFormat colorFormat = sc.format;

        /*colorImageTmp = Image(device, physicalDevice, colorFormat, sc.extent.width, sc.extent.height, 1, msaaSamples,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, "color resource");*/
        colorImageTmp = Image(device, physicalDevice, colorFormat, sc.extent.width, sc.extent.height, 1, msaaSamples,
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, "color resource");

        colorImage = colorImageTmp.image;
        colorImageView = colorImageTmp.imageView;
        colorImageMemory = colorImageTmp.imageMemory;

        /*CreateImage(sc.extent.width, sc.extent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
        colorImageView = CreateImageView(colorImage, 1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);*/
    }

    void CreateTestImage()
    {
        VkFormat format = sc.format;

        CreateImage(sc.extent.width, sc.extent.height, 1, msaaSamples, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, testImage, testMemory);
        testImageView = CreateImageView(testImage, 1, format, VK_IMAGE_ASPECT_COLOR_BIT);

        //TransitionImageLayout(testImage, 1, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
                return format;
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                return format;
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat FindDepthFormat()
    {
        return FindSupportedFormat(
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool HasStencilComponent(VkFormat format)
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void LoadModel()
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
            throw std::runtime_error(warn + err);

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        // Parse shapes in model
        for (const auto& shape : shapes)
        {
            // Parse indices in shape
            for (const auto& index : shape.mesh.indices)
            {
                Vertex vertex{};

                vertex.pos = { attrib.vertices[index.vertex_index * 3 + 0], attrib.vertices[index.vertex_index * 3 + 1] , attrib.vertices[index.vertex_index * 3 + 2] };
                vertex.texCoord = { attrib.texcoords[index.texcoord_index * 2 + 0], 1.0f - attrib.texcoords[index.texcoord_index * 2 + 1] };
                vertex.color = { 1.0f, 1.0f, 1.0f };
                vertex.norm = { attrib.normals[index.normal_index * 3 + 0], attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] };
                // TODO: Add the rest!

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }

            std::cout << "Loaded " << shape.name << "!" << std::endl;
        }

        std::cout << "Load Successful with " << vertices.size() << " vertices, and " << indices.size() << " triangles!" << std::endl;
    }

    VkSampleCountFlagBits GetMaxUsableSampleCount(PhysicalDeviceContainer deviceContainer)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(deviceContainer.device, &physicalDeviceProperties);

        VkSampleCountFlags maxCount = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

        // TODO: Ignore first three options for now (not supported by image formats)!
        /*if (maxCount & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (maxCount & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (maxCount & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }*/
        if (maxCount & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (maxCount & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (maxCount & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (maxCount & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (maxCount & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (maxCount & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }
};

// Driver code

int main()
{    
    Renderer app;

    try {
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Timer
    float time = timer.GetData().DeltaTime;

    // Exit on ESC Key Press
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(window, true);
        // TODO: Clear up!
    }

    // Enable/Disable Camera
    if (gui.spacebar_down && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        cam.enabled = !cam.enabled;

        // Enable/Disable Cursor
        if (cam.enabled)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        gui.spacebar_down = false;
    }

    if (!gui.spacebar_down && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        gui.spacebar_down = true;

    // Ignore Keyboard Inputs for Camera Movement if arcball_mode == true
    if (cam.arcball_mode)
        return;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam.MoveCamera(FWD, time);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam.MoveCamera(AFT, time);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cam.MoveCamera(UPWARD, time);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cam.MoveCamera(DOWNWARD, time);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam.MoveCamera(LEFT, time);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam.MoveCamera(RIGHT, time);
}

void MouseMovementCallback(GLFWwindow* window, double x_pos, double y_pos)
{
    float xpos = static_cast<float>(x_pos);
    float ypos = static_cast<float>(y_pos);

    if (gui.first_mouse_flag)
    {
        gui.lastX = xpos;
        gui.lastY = ypos;
        gui.first_mouse_flag = false;
    }

    float xoffset = xpos - gui.lastX;
    float yoffset = gui.lastY - ypos;

    gui.lastX = xpos;
    gui.lastY = ypos;

    TimeData time = timer.GetData();
    if (cam.arcball_mode)
        cam.RotateArcballCamera(xoffset, yoffset, WIDTH, HEIGHT, time.DeltaTime);
    else
        cam.RotateCamera(xoffset, yoffset);
}

void MouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset)
{
    TimeData time = timer.GetData();
    cam.MoveArcballCamera(y_offset, time.DeltaTime);
}
