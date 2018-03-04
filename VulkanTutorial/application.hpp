#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <algorithm>
#include <string>
#include <Windows.h>
#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "helper.hpp"


class Application {
public:
	void run();
private:
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const char* WINDOW_TITLE = "Vulkan";
	GLFWwindow * m_window;

	const char* APPLICATION_NAME = "Vulkan Playground";
	VkInstance m_instance;

	const std::vector<const char*> validation_layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	void initialize_window();
	void initialize_vulkan();
	void create_instance();
	void main_loop();
	void clean_up();
	//This allows colors to be displayed using escape sequences
	DWORD enable_virtual_terminal();
	//collects all required extensions in one vector for easy comparison later on
	std::vector<const char*> get_required_extensions();
	//checks if the given required extensions are supported by the system
	bool check_extension_support(std::vector<const char*> required_extensions);
	//checks if the validation layers are supported
	bool check_validation_layer_support();
	
};

#endif // !APPLICATION_HPP
