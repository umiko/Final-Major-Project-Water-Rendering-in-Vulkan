#include "application.hpp"

int main() {
	Application application;
#ifdef _DEBUG
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
	create_surface();
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
	
	std::vector<const char*> required_instance_extensions = get_required_instance_extensions();

	if (!check_instance_extension_support(required_instance_extensions)) {
		throw std::runtime_error("Not all glfw required extensions are supported by the system.");
	}
	create_information.enabledExtensionCount = static_cast<uint32_t>(required_instance_extensions.size());
	create_information.ppEnabledExtensionNames = required_instance_extensions.data();
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

std::vector<const char*> Application::get_required_instance_extensions()
{
	uint32_t glfw_required_extension_count = 0;
	const char** glfw_extension_names;

	glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);

	std::vector<const char*> final_required_extensions(glfw_extension_names, glfw_extension_names + glfw_required_extension_count);

	if (enableValidationLayers)
		final_required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	return final_required_extensions;
}

bool Application::check_instance_extension_support(std::vector<const char*> required_extensions)
{
	uint32_t supported_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);
	//enumerate supported extensions
	std::vector<VkExtensionProperties> supported_extensions(supported_extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());
	return check_extension_support(required_extensions, supported_extensions);
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
		throw std::runtime_error("No devices with Vulkan support found. Get a decent GPU next time.");
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
		err("No suitable GPU found, throwing exception...");
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

	if (!device_features.geometryShader) {
		score = 0;
		warn(std::string("\t") + device_properties.deviceName + " has no geometry shader and is therefore scoring " + std::to_string(score));
	}
	if (!find_queue_families(physical_device).isComplete()) {
		score = 0;
		warn(std::string("\t") + device_properties.deviceName + " has failed queue checks and is therefore scoring " + std::to_string(score));
	}
	if (!check_device_extension_support(physical_device)) {
		score = 0;
		warn(std::string("\t") + device_properties.deviceName + " has failed extension checks and is therefore scoring " + std::to_string(score));
	}
	else {
		info(std::string("\t") + device_properties.deviceName + std::string(" is scoring ") + std::to_string(score));
	}
	return score;
}

void Application::create_surface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("Window Surface creation failed");
	}
}

void Application::create_logical_device()
{
	//redo the indices, selected device might be different than last checked one after all
	QueueFamilyIndices indices = find_queue_families(m_physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_informations;
	std::set<int> unique_queue_families = { indices.graphics_family, indices.presentation_family };

	float queue_priority = 1.0f;
	for (int queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_informations.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features = {};

	VkDeviceCreateInfo logical_device_create_info = {};
	logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_informations.size());
	logical_device_create_info.pQueueCreateInfos = queue_create_informations.data();
	logical_device_create_info.pEnabledFeatures = &device_features;
	logical_device_create_info.enabledExtensionCount = 0;
	logical_device_create_info.enabledLayerCount = 0;
	//if validation layers are enabled overwrite the just set enabledLayerCount
	if (enableValidationLayers) {
		logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		logical_device_create_info.ppEnabledLayerNames = validation_layers.data();
	}

	if (vkCreateDevice(m_physical_device, &logical_device_create_info, nullptr, &m_logical_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}
	//single queue, therefore index 0
	vkGetDeviceQueue(m_logical_device, indices.graphics_family, 0, &m_graphics_queue);
	//single queue, therefore index 0
	vkGetDeviceQueue(m_logical_device, indices.presentation_family, 0, &m_presentation_queue);
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

	info("\tLooking for Queues...");

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());
	int queue_index = 0;
	for (const auto& queue_family : queue_families) {
		//Can it render???
		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = queue_index;
			info(std::string("\t\tQueue ") + std::to_string(queue_index) + " has a graphics bit");
		}
		//Can it present???
		VkBool32 presentation_support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, m_surface, &presentation_support);


		if (queue_family.queueCount > 0 && presentation_support != VK_FALSE) {
			indices.presentation_family = queue_index;
			info(std::string("\t\tQueue ") + std::to_string(queue_index) + " is able to present");
		}

		if (indices.isComplete()) {
			succ(std::string("\t\tQueue ") + std::to_string(queue_index) + " is is a suitable queue, aborting search");
			break;
		}
		queue_index++;
	}
	return indices;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Vulkan Helper Stuff
////	
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool Application::check_extension_support(std::vector<const char*> required_extensions, std::vector<VkExtensionProperties> supported_extensions) {
	//list supported extensions
	info("Extension Support:");
	bool required_extension_supported = false;
	//outer loop is required extensions, if one isnt fulfilled it breaks and returns false, otherwise returns true.
	for (const char* required_extension_name : required_extensions) {
		//reset supported extension flag
		required_extension_supported = false;
		for (const auto& supported_extension : supported_extensions) {
			//extension supported, why bother going further?
			if (strcmp(required_extension_name, supported_extension.extensionName) == 0) {
				required_extension_supported = true;
				break;
			}
		}
		std::string extension_output = (std::string("\t") + required_extension_name);
		required_extension_supported ? succ(extension_output) : warn(extension_output);
		//dont bother going further if a requested extension failed
		if (!required_extension_supported) {
			break;
		}
	}
	return required_extension_supported;
}

bool Application::check_device_extension_support(VkPhysicalDevice physical_device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> supported_extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, supported_extensions.data());
	return check_extension_support(device_extensions, supported_extensions);
}

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
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}
