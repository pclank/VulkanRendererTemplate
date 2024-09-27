#pragma once

enum GUI_BUTTON {
    RESET_BUTTON,
    PLAY_PAUSE_BUTTON
};

struct GUI {
	float test_translation[3] = { 0.0f };
    float light_pos[3] = { 0.0f, 5.0f, 1.0f };
	float test_scale = 1.0f;
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
	float lastX = 0.0f;
	float lastY = 0.0f;
    Camera* cam;
    Timer* timer;
    Model* models = nullptr;
    uint32_t nModels = 0;

    GUI(Camera* cam, Timer* timer) : cam(cam), timer(timer) {}

	void Render()
	{
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Your friendly (???) Vulkan renderer");
        ImGui::Text("FPS: %.2f", timer->GetData().FPS);
        ImGui::PlotLines("FPS", timer->GetFPSS(), FPS_SAMPLES);
        ImGui::Separator();
        ImGui::Text("Campos: %.2f, %.2f, %.2f", cam->position.x, cam->position.y, cam->position.z);
        ImGui::Checkbox("Arcball mode", &cam->arcball_mode);
        ImGui::SliderFloat("Camera sensitivity", &cam->look_sensitivity, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Camera speed", &cam->movement_speed, 0.1f, 15.0f, "%.1f");
        ImGui::Separator();
        ImGui::SliderFloat3("Suzanne translation", test_translation, -10.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("Suzanne scale", &test_scale, 0.1f, 5.0f, "%.2f");
        ImGui::SliderFloat("Animated model scale", &animated_scale, 0.1f, 5.0f, "%.2f");
        ImGui::Separator();
        ImGui::SliderFloat3("Light position", light_pos, -10.0f, 10.0f, "%.2f");
        ImGui::Checkbox("Blinn mode", &blinn_flag);
        ImGui::Separator();
        ImGui::Checkbox("Wireframe rendering", &wireframe_flag);
        ImGui::Checkbox("Skybox rendering", &skybox_flag);
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
            const std::string str = std::string("Model ") + std::to_string(i) + " enabled";
            ImGui::Checkbox(str.c_str(), &models[i].enabled);
        }
        ImGui::End();

        //ImGui::ShowDemoWindow();

        ImGui::Render();
	}

    // Button callbacks
    void ButtonCallback(GUI_BUTTON button)
    {
        if (button == RESET_BUTTON)
            reset_animation_flag = true;
        else if (button == PLAY_PAUSE_BUTTON)
            play_animation_flag = !play_animation_flag;
    }
};