#pragma once

#include <string>

const std::string MODEL_PATH = "models/viking_room.obj";
const std::string TEXTURE_PATH = "textures/viking_room.png";
const std::string NORMAL_PATH = "textures/viking_room_normal.png";
const std::string SKYBOX_PATH = "textures/skybox/";
const std::string MODELS_FOLDER = "models/";
const std::string TEXTURES_FOLDER = "textures/";

// Used as postfixes for the cubemap image files
const std::vector<std::string> postfixes = {
    "right.jpg",    // +X
    "left.jpg",     // -X
    "top.jpg",      // +Y
    "bottom.jpg",   // -Y
    "front.jpg",    // +Z
    "back.jpg"      // -Z
};
