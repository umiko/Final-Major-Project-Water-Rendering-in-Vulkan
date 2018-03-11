#include "application.hpp"

int main() {
	Application application;
#ifdef DEBUG
	enable_virtual_terminal();
#endif // DEBUG


	try {
		application.run();
	}
	catch (const std::runtime_error &error) {
		std::cerr << error.what() << std::endl;
		return EXIT_FAILURE;
	}
	int i;
	std::cin >> i;
	return EXIT_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Main Functions
////	
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Application::run()
{
	initialize_window();
	initialize_vulkan();
	main_loop();
	clean_up();

}

void Application::initialize_window()
{
	glfwInit();
	//dont use openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE, nullptr, nullptr);
}

void Application::initialize_vulkan()
{
	create_instance();
	setup_debug_callback();
	pick_physical_device();
	create_logical_device();
}

void Application::main_loop()
{
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}

void Application::create_instance()
{
	//Check if debug mode is active and check if the validation layers are supported
	if (enableValidationLayers && !check_validation_layer_support()) {
		throw std::runtime_error("Requested validation layers not available");
	}

	VkApplicationInfo application_info = {};
	application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	application_info.pApplicationName = "Vulkan Experimentation";
	application_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.pEngineName = "No Engine";
	application_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	application_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_information = {};
	create_information.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_information.pApplicationInfo = &application_info;

	std::vector<const char*> required_extensions = get_required_extensions();
	if (!check_extension_support(required_extensions)) {
		throw std::runtime_error("Not all required extensions are supported by the system.");
	}

	create_information.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
	create_information.ppEnabledExtensionNames = required_extensions.data();
	if (enableValidationLayers) {
		create_information.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_information.ppEnabledLayerNames = validation_layers.data();
	}
	else {
		create_information.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_information, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("VK_INSTANCE creation failed");
	}
	else {
		succ("VK_INSTANCE creation successful");
	}
}


/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Extensions and Layers
////	
/////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<const char*> Application::get_required_extensions()
{
	uint32_t glfw_required_extension_count = 0;
	const char** glfw_extension_names;

	glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);

	std::vector<const char*> final_required_extensions(glfw_extension_names, glfw_extension_names + glfw_required_extension_count);

	if (enableValidationLayers)
		final_required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	return final_required_extensions;
}

bool Application::check_extension_support(std::vector<const char*> required_extensions) {

	info("Required Extensions:");
	for (const char* extension_name : required_extensions)
	{
		info(std::string("\t") + extension_name);
	}
	//get number of supported extensions
	uint32_t supported_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);

	//enumerate supported extensions and sort it alphabetically
	std::vector<VkExtensionProperties> supported_extensions(supported_extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());
	std::sort(supported_extensions.begin(), supported_extensions.end(), compare_extensions);

	//list supported extensions
	info("Supported Extensions:");
	short supported_required_extension_counter = 0;
	for (const auto& extension : supported_extensions) {
		bool required_extension = false;
		for (const char* required_extension_name : required_extensions) {
			if (strcmp(required_extension_name, extension.extensionName) == 0) {
				++supported_required_extension_counter;
				required_extension = true;
			}
		}
		std::string extension_output = (std::string("\t") + extension.extensionName) + " V" + std::to_string(extension.specVersion);
		required_extension ? succ(extension_output) : info(extension_output);
	}
	return (supported_required_extension_counter == required_extensions.size());
}

void Application::setup_debug_callback()
{
	if (!enableValidationLayers) return;
	VkDebugReportCallbackCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	create_info.pfnCallback = debug_callback;

	if (CreateDebugReportCallbackEXT(m_instance, &create_info, nullptr, &callback) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

bool Application::check_validation_layer_support()
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> supported_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, supported_layers.data());

	for (const char* layer_name : validation_layers) {
		bool layer_found = false;
		for (const VkLayerProperties& layer_properties : supported_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}
		if (!layer_found) {
			warn("Validation layers not supported");
			return false;
		}
	}
	succ("Validation layers supported");
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Physical Devices
////	
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Application::pick_physical_device()
{
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
	if (device_count == 0)
		std::runtime_error("No devices with Vulkan support found. Get a decent GPU next time.");
	std::vector<VkPhysicalDevice> physical_devices_found(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, physical_devices_found.data());

	std::multimap<int, VkPhysicalDevice> gpu_candidates;

	info("Found GPUs:");

	for (const auto& device : physical_devices_found) {
		gpu_candidates.insert(std::make_pair(evaluate_physical_device_capabilities(device), device));
	}

	if (gpu_candidates.rbegin()->first > 0) {
		m_physical_device = gpu_candidates.rbegin()->second;
#ifdef _DEBUG
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(m_physical_device, &device_properties);

		succ(std::string("Selected ") + device_properties.deviceName);
#endif
	}

	if (m_physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("Your GPU is not suitable");
	}
}

int Application::evaluate_physical_device_capabilities(VkPhysicalDevice physical_device) {
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(physical_device, &device_features);

	int score = 0;

	switch (device_properties.deviceType) {
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		score += 10000;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		score += 5000;
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		score += 2500;
		break;
	default:
		break;
	}
	score += device_properties.limits.maxImageDimension2D;
	if (!device_features.geometryShader || find_queue_families(physical_device).isComplete()) {
		score = 0;
		warn(std::string(device_properties.deviceName) + " has no geometry shader and is therefore scoring " + std::to_string(score));
	}
	info(std::string("\t") + device_properties.deviceName + std::string(" is scoring ") + std::to_string(score));
	return score;
}

void Application::create_logical_device()
{
	QueueFamilyIndices indices = find_queue_families(m_physical_device);

	VkDeviceQueueCreateInfo queue_create_info = {};
	queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex = indices.graphics_family;
	queue_create_info.queueCount = 1;

	float queue_priority = 1.0f;

	queue_create_info.pQueuePriorities = &queue_priority;

	VkPhysicalDeviceFeatures device_features = {};

	VkDeviceCreateInfo logical_device_create_info = {};
	logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	logical_device_create_info.pQueueCreateInfos = &queue_create_info;
	logical_device_create_info.queueCreateInfoCount = 1;

	logical_device_create_info.pEnabledFeatures = &device_features;
	logical_device_create_info.enabledExtensionCount = 0;
	if (enableValidationLayers) {
		logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		logical_device_create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else
	{
		logical_device_create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physical_device, &logical_device_create_info, nullptr, &m_logical_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Queue Families
////	
/////////////////////////////////////////////////////////////////////////////////////////////////////

QueueFamilyIndices Application::find_queue_families(VkPhysicalDevice physical_device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
	int i = 0;
	for (const auto& queue_family : queue_families) {
		queue_family.
		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}
	return indices;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Vulkan Helper Stuff
////	
/////////////////////////////////////////////////////////////////////////////////////////////////////

VkResult Application::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator, VkDebugReportCallbackEXT * pCallback) {
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void Application::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks * pAllocator) {
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}



VKAPI_ATTR VkBool32 VKAPI_CALL Application::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData)
{
	std::cerr << "Validation layer: " << msg << std::endl;

	return VK_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Cleanup
////	
/////////////////////////////////////////////////////////////////////////////////////////////////////

void Application::clean_up()
{
	vkDestroyDevice(m_logical_device, nullptr);
	DestroyDebugReportCallbackEXT(m_instance, callback, nullptr);

	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}
