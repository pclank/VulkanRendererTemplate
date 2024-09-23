#pragma once

struct GUI {
	float test_translation[3] = { 0.0f };
	float test_scale = 1.0f;
	bool spacebar_down = false;
	bool first_mouse_flag = true;
	float lastX = 0.0f;
	float lastY = 0.0f;
    Camera* cam;
    Timer* timer;

    GUI(Camera* cam, Timer* timer) : cam(cam), timer(timer) {}

	void Render()
	{
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Your friendly (???) Vulkan render");
        ImGui::Text("FPS: %.2f", timer->GetData().FPS);
        ImGui::Separator();
        ImGui::Text("Campos: %.2f, %.2f, %.2f", cam->position.x, cam->position.y, cam->position.z);
        ImGui::Checkbox("Arcball mode", &cam->arcball_mode);
        ImGui::SliderFloat("Camera sensitivity", &cam->look_sensitivity, 0.1f, 5.0f, "%.1f");
        ImGui::SliderFloat("Camera speed", &cam->movement_speed, 0.1f, 15.0f, "%.1f");
        ImGui::Separator();
        ImGui::SliderFloat3("Suzanne translation", test_translation, -10.0f, 10.0f, "%.2f");
        ImGui::SliderFloat("Suzanne scale", &test_scale, 0.1f, 5.0f, "%.2f");
        ImGui::End();

        //ImGui::ShowDemoWindow();

        ImGui::Render();
	}
};