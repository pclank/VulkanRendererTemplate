#pragma once

#include<AnimationPlayer.hpp>

enum GUI_BUTTON {
    RESET_BUTTON,
    PLAY_PAUSE_BUTTON
};

struct GUI {
	//float test_translation[3] = { 0.0f };
	//float test_scale = 1.0f;
    float light_pos[3] = { 0.0f, 5.0f, 1.0f };
    std::vector<float> model_scales;
    std::vector<std::array<float, 3>> model_translations;
    std::vector<uint32_t> explode_flags;
    std::vector<float> explosion_rates;
    float animated_scale = 1.0f;
    float animation_speed = 1.0f;
	bool spacebar_down = false;
	bool first_mouse_flag = true;
    bool wireframe_flag = false;
    bool skybox_flag = true;
    bool reset_animation_flag = false;
    bool play_animation_flag = false;
    bool blinn_flag = false;
    bool cubic_interpolation_flag = false;
    bool grid_flag = false;
    bool normals_flag = false;
	float lastX = 0.0f;
	float lastY = 0.0f;
    Camera* cam;
    Timer* timer;
    Model* models = nullptr;
    AnimationPlayer* animationPlayers = nullptr;
    uint32_t nModels = 0;
    uint32_t nAnimationPlayers = 0;

    GUI(Camera* cam, Timer* timer) : cam(cam), timer(timer) {}

    void Setup()
    {
        model_scales = std::vector<float>(nModels, 1.0f);
        model_translations.resize(nModels);
        explode_flags.resize(nModels);
        explosion_rates = std::vector<float>(nModels, 4.7f);
    }

	void Render()
	{
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Your friendly (???) Vulkan renderer");
        ImGui::Text("FPS: %.2f", timer->GetData().FPS);
        //ImGui::PlotLines("FPS", timer->GetFPSS(), FPS_SAMPLES);
        ImGui::PlotLines("ms", timer->GetDeltas(), FPS_SAMPLES);
        ImGui::Separator();
        ImGui::Text("Campos: %.2f, %.2f, %.2f", cam->position.x, cam->position.y, cam->position.z);
        ImGui::Checkbox("Arcball mode", &cam->arcball_mode);
        ImGui::SliderFloat("Camera sensitivity", &cam->look_sensitivity, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Camera speed", &cam->movement_speed, 20.0f, MAX_SPEED, "%.1f");
        /*ImGui::Separator();
        ImGui::SliderFloat3("Suzanne translation", test_translation, -10.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("Suzanne scale", &test_scale, 0.1f, 5.0f, "%.2f");
        ImGui::SliderFloat("Animated model scale", &animated_scale, 0.1f, 5.0f, "%.2f");*/
        ImGui::Separator();
        ImGui::SliderFloat3("Light position", light_pos, -50.0f, 50.0f, "%.2f");
        ImGui::Checkbox("Blinn mode", &blinn_flag);
        ImGui::Separator();
        ImGui::Checkbox("Wireframe rendering", &wireframe_flag);
        ImGui::Checkbox("Skybox rendering", &skybox_flag);
        ImGui::Checkbox("Grid rendering", &grid_flag);
        ImGui::Checkbox("Draw normals", &normals_flag);
        ImGui::Separator();
        ImGui::SliderFloat("Animation speed", &animation_speed, 0.1f, 2.0f, "%.2f");
        ImGui::Checkbox("Cubic interpolation", &cubic_interpolation_flag);
        ImGui::BeginGroup();
        if (ImGui::Button("Reset animation"))
            ButtonCallback(RESET_BUTTON);
        if (ImGui::Button("Start/Pause animation"))
            ButtonCallback(PLAY_PAUSE_BUTTON);
        ImGui::EndGroup();
        ImGui::Separator();
        for (size_t i = 0; i < nModels; i++)
        {
            const std::string strIndex = std::string("Model ") + std::to_string(i);
            const std::string strEnabled = std::string("Model ") + std::to_string(i) + " enabled";
            const std::string strTranslation = std::string("Model ") + std::to_string(i) + " translation";
            const std::string strScale = std::string("Model ") + std::to_string(i) + " scaling";
            const std::string strExplodeFlag = std::string("Model ") + std::to_string(i) + " explode";
            const std::string strExplodeRate = std::string("Model ") + std::to_string(i) + " explode rate";
            const std::string strAnim = std::string(" Model ") + std::to_string(i) + " current animation";
            ImGui::TextColored(ImVec4(1, 1, 0, 1), strIndex.c_str());
            ImGui::Checkbox(strEnabled.c_str(), &models[i].enabled);
            ImGui::SliderFloat3(strTranslation.c_str(), model_translations[i].data(), -10.0f, 10.0f, "%.2f");
            ImGui::SliderFloat(strScale.c_str(), &model_scales[i], 0.01f, 5.0f, "%.02f");
            ImGui::Checkbox(strExplodeFlag.c_str(), (bool*)&explode_flags[i]);
            ImGui::SliderFloat(strExplodeRate.c_str(), &explosion_rates[i], 0.0f, 10.0f, "%.1f");
            if (models[i].meshes[0].animations.size() > 0)
                ImGui::SliderInt(strAnim.c_str(), &models[i].currentAnim, 0, std::max(0, static_cast<int>(models[i].meshes[0].animations.size()) - 1));
        }
        ImGui::End();

        //ImGui::ShowDemoWindow();

        ImGui::Render();
	}

    // Button callbacks
    void ButtonCallback(GUI_BUTTON button)
    {
        if (button == RESET_BUTTON)
        {
            reset_animation_flag = true;
            for (uint32_t i = 0; i < nAnimationPlayers; i++)
                animationPlayers[i].ResetTime();
        }
        else if (button == PLAY_PAUSE_BUTTON)
            play_animation_flag = !play_animation_flag;
    }
};