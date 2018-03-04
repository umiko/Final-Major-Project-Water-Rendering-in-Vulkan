#include "application.hpp"

int main() {
	Application application;

	try {
		application.run();
	}
	catch (const std::runtime_error &error) {
		std::cerr << error.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

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
}

void Application::create_instance()
{
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

	////////////////////////////////////////////////////////////////////////////////////////////////////////
	////																								////
	////										Extensions												////
	////																								////
	////////////////////////////////////////////////////////////////////////////////////////////////////////


	std::vector<const char*> required_extensions = get_required_extensions();
	if (!check_extension_support(required_extensions)) {
		throw std::runtime_error("Not all required extensions are supported by the system.");
	}

	create_information.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
	create_information.ppEnabledExtensionNames = required_extensions.data();
	create_information.enabledLayerCount = 0;

	if (vkCreateInstance(&create_information, nullptr, &m_instance) != VK_SUCCESS) {
		throw std::runtime_error("VK_INSTANCE creation failed");
	}
	else {
		std::cout << "VK_INSTANCE creation successful" << std::endl;
	}
}

void Application::main_loop()
{
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
	}
}

void Application::clean_up()
{
	vkDestroyInstance(m_instance, nullptr);
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

DWORD Application::enable_virtual_terminal() {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode))
	{
		return GetLastError();
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode))
	{
		return GetLastError();
	}
	return 1;
}

std::vector<const char*> Application::get_required_extensions()
{
	uint32_t glfw_required_extension_count = 0;
	const char** glfw_extension_names;

	glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);

	std::vector<const char*> final_required_extensions(glfw_extension_names, glfw_extension_names + glfw_required_extension_count);

	if(enableValidationLayers)
		final_required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	return final_required_extensions;
}

bool Application::check_extension_support(std::vector<const char*> required_extensions) {
	enable_virtual_terminal();

	std::cout << "Required Extensions:" << std::endl;
	for (const char* extension_name : required_extensions)
	{
		std::cout << "\t" << extension_name << std::endl;
	}
	//get number of supported extensions
	uint32_t supported_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);

	//enumerate supported extensions and sort it alphabetically
	std::vector<VkExtensionProperties> supported_extensions(supported_extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());
	std::sort(supported_extensions.begin(), supported_extensions.end(), compare_extensions);

	//list supported extensions
	std::cout << "Supported Extensions:" << std::endl;
	short supported_required_extension_counter = 0;
	for (const auto& extension : supported_extensions) {
		bool required_extension = false;
		for (const char* required_extension_name : required_extensions) {
			if (strcmp(required_extension_name, extension.extensionName)==0) {
				++supported_required_extension_counter;
				required_extension = true;
			}
		}
		std::cout << (required_extension ? "\x1b[42m\t" : "\x1b[0m\t") << extension.extensionName << " V" << extension.specVersion << std::endl;
	}
	return (supported_required_extension_counter == required_extensions.size());
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
			return false;
		}
	}

	return true;
}
