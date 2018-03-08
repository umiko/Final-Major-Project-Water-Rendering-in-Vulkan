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
#include <map>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "helper.hpp"

struct QueueFamilyIndices {
	int graphics_family = -1;

	bool isComplete() {
		return graphics_family >= 0;
	}
};


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
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;

	VkDebugReportCallbackEXT callback;

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
	void main_loop();
	void clean_up();

	void create_instance();
	//collects all required extensions in one vector for easy comparison later on
	std::vector<const char*> get_required_extensions();
	//checks if the given required extensions are supported by the system
	bool check_extension_support(std::vector<const char*> required_extensions);
	//checks if the validation layers are supported

	bool check_validation_layer_support();
	void setup_debug_callback();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	void pick_physical_device();
	int evaluate_physical_device_capabilities(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physical_device);

};

#endif // !APPLICATION_HPP
