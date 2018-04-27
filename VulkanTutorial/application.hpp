#pragma once

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <algorithm>
#include <string>
#include <string_view>
#include <map>
#include <set>
#include <array>
#include <chrono>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLM_FORCE_RADIANS
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "logger.hpp"
#include "vertex.hpp"
#include "helper.hpp"
#include "ocean.hpp"
#include "displacement.hpp"


//Vulkan works with queues to which commands need to be submitted.
//commands can be recorded, stored and are executed when submitted to a queue.
//Some queues are more suitable than others or can only offer certain features.
//Therefore we need to make sure that we know which queues support the desired functionality.
//This struct enables us to save the optimal queue indices and check if all needed queues are found.
struct QueueFamilyIndices
{
	//Index of a queue supporting graphics operations
	int graphics_family = -1;
	//Index of a queue supporting presentation operations
	int presentation_family = -1;

	//Returns true if required queues are found
	bool isComplete()
	{
		return graphics_family >= 0 && presentation_family >= 0;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

//a vertex with position, color and texture coordinate

//A uniform or constant buffer.
//Used to get regularly changing data to the shader
struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;
};

//The main application
class Application
{
  public:
	void run();

  private:
	const int WIDTH = 800;
	const int HEIGHT = 600;
	const char *WINDOW_TITLE = "Vulkan";
	const char *APPLICATION_NAME = "Vulkan Playground";

	bool m_enable_wireframe = true;

	float m_time=0;

	Ocean* m_ocean;

	GLFWwindow *m_window;

	//basics

	VkInstance m_instance;
	VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
	VkDevice m_logical_device;

	//queues

	VkQueue m_graphics_queue;
	VkQueue m_presentation_queue;

	VkDebugReportCallbackEXT callback;

	VkSurfaceKHR m_surface;
	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchain_image_format;
	VkExtent2D m_swapchain_extent;
	VkRenderPass m_render_pass;
	VkDescriptorSetLayout m_descriptor_set_layout;
	VkPipelineLayout m_pipeline_layout;
	VkPipeline m_graphics_pipeline;

	//buffers, images and pools

	VkCommandPool m_command_pool;
	VkBuffer m_vertex_buffer;
	VkDeviceMemory m_vertex_buffer_memory;
	VkBuffer m_index_buffer;
	VkDeviceMemory m_index_buffer_memory;

	VkBuffer m_displacement_buffer;
	VkDeviceMemory m_displacement_memory;

	VkBuffer m_uniform_buffer;
	VkDeviceMemory m_uniform_buffer_memory;
	VkDescriptorPool m_descriptor_pool;
	VkDescriptorSet m_descriptor_set;
	VkImage m_texture_image;
	VkDeviceMemory m_texture_image_memory;
	VkImageView m_texture_image_view;
	VkSampler m_texture_sampler;

	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	std::vector<VkFramebuffer> m_swapchain_framebuffers;
	std::vector<VkCommandBuffer> m_command_buffers;

	//semaphores

	VkSemaphore m_image_available_semaphore;
	VkSemaphore m_render_finished_semaphore;

	const std::vector<const char *> validation_layers = {
		"VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_monitor"};

	const std::vector<const char *> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	std::vector<Vertex> m_vertices = {
		{ { -0.5f, -0.5f, 1.0f },{ 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f } },
	{ { 0.5f, -0.5f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f } },
	{ { 0.5f, 0.5f, 0.0f },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },
	{ { -0.5f, 0.5f, 0.0f },{ 1.0f, 1.0f, 1.0f },{ 1.0f, 1.0f } }
	};

	std::vector<uint32_t> m_indices{0, 1, 2, 2, 3, 0};

	std::vector<Displacement> m_displacements = {};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	//application lifecycle
	void configure_application();

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
	void create_descriptor_set_layout();
	void create_graphics_pipeline();
	void create_framebuffers();
	void create_command_pool();

	//texturing

	void create_texture_image();
	void create_texture_image_view();
	void create_texture_sampler();

	//vertices

	void create_vertex_buffer();
	void create_index_buffer();

	void create_displacement_buffer();

	//descriptors

	void create_uniform_buffer();
	void create_descriptor_pool();
	void create_descriptor_set();

	//command buffers

	void create_command_buffers();
	void create_semaphores();

	//update stuff

	void draw_frame();
	void update_buffers();

	//swapchain creation

	void recreate_swapchain();
	void clean_up_swapchain();

	//window input reactions

	static void on_window_resized(GLFWwindow *window, int width, int height);

	std::vector<const char *> get_required_instance_extensions();

	//support checks
	bool check_validation_layer_support();
	bool check_extension_support(std::vector<const char *> required_extensions, std::vector<VkExtensionProperties> supported_extensions);
	bool check_instance_extension_support(std::vector<const char *> required_extensions);
	bool check_device_extension_support(VkPhysicalDevice physical_device);

	//evaluations and selections
	int evaluate_physical_device_capabilities(VkPhysicalDevice device);
	QueueFamilyIndices find_queue_families(VkPhysicalDevice physical_device);
	SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice device);
	VkSurfaceFormatKHR choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats);
	VkPresentModeKHR choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> available_present_modes);
	VkExtent2D choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities);
	uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties);

	//creation and transformation helpers
	VkShaderModule create_shader_module(const std::vector<char> &code);
	void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);
	void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
	void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory);
	VkImageView create_image_view(VkImage image, VkFormat format);
	void copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

	//buffer recording helpers
	VkCommandBuffer begin_single_time_commands();
	void end_single_time_commands(VkCommandBuffer command_buffer);

	//Debug stuff
	static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char *layerPrefix, const char *msg, void *userData);
	VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback);
	void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator);
};