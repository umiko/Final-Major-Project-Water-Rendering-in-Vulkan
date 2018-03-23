#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <algorithm>
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "helper.hpp"

struct QueueFamilyIndices {
	int graphics_family = -1;
	int presentation_family = -1;

	bool isComplete() {
		return graphics_family >= 0 && presentation_family >= 0;
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
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
	VkDevice m_logical_device;
	VkQueue m_graphics_queue;
	VkQueue m_presentation_queue;

	//Debug stuff

	VkDebugReportCallbackEXT callback;

	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchain_image_format;
	VkExtent2D m_swapchain_extent;
	VkRenderPass m_render_pass;
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_graphics_pipeline;
	VkCommandPool m_command_pool;

	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	std::vector<VkFramebuffer> m_swapchain_framebuffers;
	std::vector<VkCommandBuffer> m_command_buffers;

	VkSemaphore m_image_available_semaphore;
	VkSemaphore m_render_finished_semaphore;

	const std::vector<const char*> validation_layers = {
		"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_monitor"
	};

	const std::vector<const char*> device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	void initialize_window();
	void initialize_vulkan();
	void main_loop();
	void clean_up();

	//Initialization stuff

	void create_instance();
	void setup_debug_callback();
	void create_surface();
	void pick_physical_device();
	void create_logical_device();
	void create_swapchain();
	void create_image_views();
	void create_render_pass();
	void create_graphics_pipeline();
	void create_framebuffers();
	void create_command_pool();
	void create_command_buffers();
	void create_semaphores();

	void draw_frame();

	void recreate_swapchain();

	void clean_up_swapchain();

	//Vulkan helper stuff thats not unvulkan enough to be declared just helper stuff
	static void on_window_resized(GLFWwindow* window, int width, int height);

	//collects all required instance extensions in one vector for easy comparison later on
	std::vector<const char*> get_required_instance_extensions();

	//checks if the validation layers are supported
	bool check_validation_layer_support();
	//checks if the given required extensions are supported by the system
	bool check_extension_support(std::vector<const char*> required_extensions, std::vector<VkExtensionProperties> supported_extensions);
	bool check_instance_extension_support(std::vector<const char*> required_extensions);
	bool check_device_extension_support(VkPhysicalDevice physical_device);
	//scores the physical devices of the system
	int evaluate_physical_device_capabilities(VkPhysicalDevice device);
	QueueFamilyIndices find_queue_families(VkPhysicalDevice physical_device);
	SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device);
	VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats);
	VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> available_present_modes);
	VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities);
	VkShaderModule create_shader_module(const std::vector<char> &code);

	//Debug stuff
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData);
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
};

#endif // !APPLICATION_HPP