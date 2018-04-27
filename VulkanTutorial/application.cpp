#include "application.hpp"

int main()
{
	Application application;
	//to enable colored console output, COLORMODE must be defined
#ifdef COLORMODE
	enable_virtual_terminal();
#endif // DEBUG

	//try to run the application
	try
	{
		application.run();
	}
	//if an error occurs, put out an error message and if build for release exit the application
	catch (const std::runtime_error &error)
	{
		err(error.what());
#ifndef _DEBUG
		return EXIT_FAILURE;
#endif
	}
#ifdef _DEBUG
	//to keep the console open and not to miss the error message wait for input
	int i;
	std::cin >> i;
#endif
	return EXIT_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Main Functions
////
/////////////////////////////////////////////////////////////////////////////////////////////////////

//Run the applications lifecycle
void Application::run()
{	
	configure_application();
	initialize_window();
	initialize_vulkan();
	main_loop();
	clean_up();
}

//handles the generation of the ocean surface
void Application::configure_application()
{
	uint32_t ocean_resolution = 64;
	//dont want a dialog every time i need to debug something
#ifndef _DEBUG
	std::cout << "Please enter the resolution the plane should have(Power of 2):";
	std::cin >> ocean_resolution;
	if (ocean_resolution <= 1) {
		throw std::runtime_error("Number is unfit for grid creation");
	}
	else if (ocean_resolution >= 2048) {
		std::cout << "This will take ages to generate, are you sure you wanna try?[Y/N]";
		char c;
		std::cin >> c;
		if (tolower(c) != 'y') {
			info("Probably the right choice");
			throw std::runtime_error("User aborted execution");
		}
	}
#endif // !_DEBUG

	m_ocean = new Ocean(ocean_resolution, 4.0f);
	m_vertices = m_ocean->getVertices();
	//TODO: generate first displacement map here
	for (Vertex vert : m_vertices) {
		m_displacements.push_back({ { (float)rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, (float)rand() / RAND_MAX } });
	}
	m_indices = m_ocean->getIndices();

	//
#ifndef _DEBUG
	std::cout << "Would you like to display the wave as wireframe?[Y/N]" << std::endl;
	char c;
	std::cin >> c;
	if (tolower(c) != 'y') {
		m_enable_wireframe = false;
	}
#endif // !_DEBUG

}

//get a window going using glfw
void Application::initialize_window()
{
	glfwInit();
	//dont use openGL
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//be resizable
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE, nullptr, nullptr);

	//store a reference to the application
	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, Application::on_window_resized);
}

//set up everything thats needed to render an image using vulkan
void Application::initialize_vulkan()
{
	info("Initializing Vulkan...");
	create_instance();
	setup_debug_callback();
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_swapchain();
	create_image_views();
	create_render_pass();
	create_descriptor_set_layout();
	create_graphics_pipeline();
	create_framebuffers();
	create_command_pool();

	create_texture_image();
	create_texture_image_view();
	create_texture_sampler();

	create_vertex_buffer();
	create_index_buffer();

	create_displacement_buffer();

	create_uniform_buffer();
	create_descriptor_pool();
	create_descriptor_set();
	create_command_buffers();
	create_semaphores();
	succ("Vulkan Initialized");
}

//Here values are updated and the animations happen
void Application::main_loop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		glfwPollEvents();
		//TODO: update goes here

		update_buffers();
		draw_frame();
		//wait until everything is done
		vkQueueWaitIdle(m_presentation_queue);
	}
	vkDeviceWaitIdle(m_logical_device);
}

#pragma region Initialization

//creates a vulkan instance
void Application::create_instance()
{
	info("Creating Vulkan instance...");
	//Check if debug mode is active and check if the validation layers are supported
	if (enableValidationLayers && !check_validation_layer_support())
	{
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

	//check if the required extensions are supported, if not, throw an exception
	std::vector<const char *> required_instance_extensions = get_required_instance_extensions();
	if (!check_instance_extension_support(required_instance_extensions))
	{
		throw std::runtime_error("Not all glfw required extensions are supported by the system.");
	}
	create_information.enabledExtensionCount = static_cast<uint32_t>(required_instance_extensions.size());
	create_information.ppEnabledExtensionNames = required_instance_extensions.data();

	//enable validation layers
	if (enableValidationLayers)
	{
		create_information.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_information.ppEnabledLayerNames = validation_layers.data();
	}
	else
	{
		create_information.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_information, nullptr, &m_instance) != VK_SUCCESS)
	{
		throw std::runtime_error("VK_INSTANCE creation failed");
	}
	succ("VK_INSTANCE creation successful");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Extensions and Layers
////
/////////////////////////////////////////////////////////////////////////////////////////////////////

//setting up debug callback for validation layers
void Application::setup_debug_callback()
{
	info("Setting up Debug Callback...");
	//skip if validation layers are disabled
	if (!enableValidationLayers)
		return;

	VkDebugReportCallbackCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
	create_info.pfnCallback = debug_callback;

	if (CreateDebugReportCallbackEXT(m_instance, &create_info, nullptr, &callback) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug callback!");
	}
	succ("Debug Callback set up");
}

//Acquiring the actual function pointer from extension and executing it
//basically a proxy function to create a debug callback
VkResult Application::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

//Proxy function to destroy a debug callback
void Application::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

//checks if validation layers are supported, for debug purposes
bool Application::check_validation_layer_support()
{
	info("Checking validation layer support...");
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> supported_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, supported_layers.data());

	//TODO: rewrite with the shortened set removal variant
	for (const char *layer_name : validation_layers)
	{
		bool layer_found = false;
		for (const VkLayerProperties &layer_properties : supported_layers)
		{
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}
		if (!layer_found)
		{
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

//selects the most suitable device to render on
void Application::pick_physical_device()
{
	info("Picking physical device...");

	//check if a device is present
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);
	if (device_count == 0)
	{
		throw std::runtime_error("No devices with Vulkan support found. Get a decent GPU next time.");
	}

	std::vector<VkPhysicalDevice> physical_devices_found(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, physical_devices_found.data());

	std::multimap<int, VkPhysicalDevice> gpu_candidates;

	info("Found GPUs:");

	//check the capabilities of the device and add it by its score to the multimap
	for (const auto &device : physical_devices_found)
	{
		gpu_candidates.insert(std::make_pair(evaluate_physical_device_capabilities(device), device));
	}

	//pick the highest number and select it as device if its value is not 0
	if (gpu_candidates.rbegin()->first > 0)
	{
		m_physical_device = gpu_candidates.rbegin()->second;
		//for debug purposes, put out the device name and its score
#ifdef _DEBUG
		VkPhysicalDeviceProperties device_properties;
		vkGetPhysicalDeviceProperties(m_physical_device, &device_properties);

		succ(std::string("Selected ") + device_properties.deviceName);
#endif
	}

	//if no device was picked, put out an error
	if (m_physical_device == VK_NULL_HANDLE)
	{
		throw std::runtime_error("Your GPU is not suitable");
	}
}

//get the window surface from glfw
void Application::create_surface()
{
	info("Creating Surface...");
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Window Surface creation failed");
	}
	succ("Surface created");
}

//create a logical device to interface with the actual gpu
void Application::create_logical_device()
{
	info("Creating logical device...");
	//redo the indices, selected device might be different than last checked one after all
	QueueFamilyIndices indices = find_queue_families(m_physical_device);

	//TODO: outsource queue creation and queue stuff in some kind of queue manager
	std::vector<VkDeviceQueueCreateInfo> queue_create_informations;
	std::set<int> unique_queue_families = {indices.graphics_family, indices.presentation_family};

	float queue_priority = 1.0f;
	for (int queue_family : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_informations.push_back(queue_create_info);
	}
	//Set device features that need to be enabled
	VkPhysicalDeviceFeatures device_features = {};
	//TODO: Wireframe happens here
	device_features.fillModeNonSolid = VK_TRUE;
	//also enable texture sampling
	device_features.samplerAnisotropy = VK_TRUE;
	//geometry shader
	device_features.geometryShader = VK_TRUE;

	//fill in the struct so the creation function knows what is needed
	VkDeviceCreateInfo logical_device_create_info = {};
	logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_informations.size());
	logical_device_create_info.pQueueCreateInfos = queue_create_informations.data();
	logical_device_create_info.pEnabledFeatures = &device_features;
	logical_device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	logical_device_create_info.ppEnabledExtensionNames = device_extensions.data();
	logical_device_create_info.enabledLayerCount = 0;

	//if validation layers are enabled overwrite the just set enabledLayerCount
	if (enableValidationLayers)
	{
		logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		logical_device_create_info.ppEnabledLayerNames = validation_layers.data();
	}

	if (vkCreateDevice(m_physical_device, &logical_device_create_info, nullptr, &m_logical_device) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device");
	}

	//retrieve created queues
	//single queue, therefore index 0
	vkGetDeviceQueue(m_logical_device, indices.graphics_family, 0, &m_graphics_queue);
	//single queue, therefore index 0
	vkGetDeviceQueue(m_logical_device, indices.presentation_family, 0, &m_presentation_queue);

	succ("Logical Device creation Successful!");
}

//this will create the swaochain
void Application::create_swapchain()
{
	info("Creating Swapchain...");
	//use helper functions to gather information needed for the swapchain creation
	SwapChainSupportDetails swapchain_support = query_swapchain_support(m_physical_device);

	VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats);
	VkPresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.present_modes);
	VkExtent2D extent = choose_swapchain_extent(swapchain_support.capabilities);
	//do triple buffering
	uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
	//check if we arent trying to do too many images
	if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
	{
		image_count = swapchain_support.capabilities.maxImageCount;
	}

	//tell  the create function the requirements of the swapchain
	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//TODO: Get a dedicated transfer queue for better performance
	QueueFamilyIndices indices = find_queue_families(m_physical_device);
	uint32_t queue_family_indices[] = {(uint32_t)indices.graphics_family, (uint32_t)indices.presentation_family};
	if (indices.graphics_family != indices.presentation_family)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swapchain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_logical_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		throw std::runtime_error("Swapchain creation failed");
	}

	//After creating the swapchain get the images
	vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, nullptr);
	m_swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, m_swapchain_images.data());

	//save those two as they will be needed later on
	m_swapchain_image_format = surface_format.format;
	m_swapchain_extent = extent;
	succ("Swapchain created successfully");
}

//create image views for the swapchain images
void Application::create_image_views()
{
	info("Creating image views...");
	m_swapchain_image_views.resize(m_swapchain_images.size());
	for (size_t i = 0; i < m_swapchain_images.size(); i++)
	{
		m_swapchain_image_views[i] = create_image_view(m_swapchain_images[i], m_swapchain_image_format);
	}
	succ("Image Views created");
}

//create the render pass
//it tells vulkan which kind of buffers and images we work with
void Application::create_render_pass()
{
	info("Creating Render Pass...");
	VkAttachmentDescription color_attachment_description = {};
	color_attachment_description.format = m_swapchain_image_format;
	//change in case of multisampling
	color_attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_reference = {};
	//index of color attachment description
	color_attachment_reference.attachment = 0;
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass_description = {};
	subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_description.colorAttachmentCount = 1;
	subpass_description.pColorAttachments = &color_attachment_reference;

	//ensure that images are available at the point we
	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass = 0;
	//wait for swapchain to finish reading the image
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask = 0;
	//so you can write to the image
	subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = 1;
	render_pass_create_info.pAttachments = &color_attachment_description;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass_description;
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &subpass_dependency;

	if (vkCreateRenderPass(m_logical_device, &render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		throw std::runtime_error("Render Pass creation failed");
	}

	succ("Render Pass Created");
}

//create a uniform buffer and texture sampler descriptor to use in the shader
//The uniform buffer and texture sampler can be updated to get new values
//this tells vulkan where it can find which descriptor
void Application::create_descriptor_set_layout()
{
	info("Creating descriptor set layout...");

	//configure uniform buffer layout binding
	VkDescriptorSetLayoutBinding ubo_layout_binding = {};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr;
	//configure texture sampler layout binding
	VkDescriptorSetLayoutBinding sampler_layout_binding = {};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	//create the descriptor layout
	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_layout_binding, sampler_layout_binding};

	VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
	descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_set_layout_create_info.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptor_set_layout_create_info.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_logical_device, &descriptor_set_layout_create_info, nullptr, &m_descriptor_set_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed creating descriptor set layout");
	}
	succ("Descriptor set layout created");
}

//sets up each step of the graphics pipeline and creates the graphics pipeline itself
void Application::create_graphics_pipeline()
{
	info("Creating graphics pipeline...");

	//setting up shader modules
	std::vector<char> vert_shader_code = read_file("shaders/vert.spv");
	std::vector<char> geom_shader_code = read_file("shaders/geom.spv");
	std::vector<char> frag_shader_code = read_file("shaders/frag.spv");

	VkShaderModule vert_shader_module;
	VkShaderModule geom_shader_module;
	VkShaderModule frag_shader_module;

	vert_shader_module = create_shader_module(vert_shader_code);
	geom_shader_module = create_shader_module(geom_shader_code);
	frag_shader_module = create_shader_module(frag_shader_code);

	//create vertex shader stage
	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	//create geometry shader stage
	VkPipelineShaderStageCreateInfo geom_shader_stage_info = {};
	geom_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geom_shader_stage_info.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geom_shader_stage_info.module = geom_shader_module;
	geom_shader_stage_info.pName = "main";

	//create fragment shader stage
	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, geom_shader_stage_info, frag_shader_stage_info};

	//set up vertex input
	auto vertex_binding_descriptions = Vertex::get_binding_description();
	auto vertex_attribute_descriptions = Vertex::get_attribute_descriptions();

	auto displacement_binding_descriptions = Displacement::get_binding_description();
	auto displacement_attribute_descriptions = Displacement::get_attribute_descriptions();

	std::vector<VkVertexInputBindingDescription> input_binding_descriptions = { vertex_binding_descriptions, displacement_binding_descriptions };
	std::vector<VkVertexInputAttributeDescription> input_attribute_descriptions = {};
	input_attribute_descriptions.push_back(vertex_attribute_descriptions[0]);
	input_attribute_descriptions.push_back(vertex_attribute_descriptions[1]);
	input_attribute_descriptions.push_back(vertex_attribute_descriptions[2]);
	input_attribute_descriptions.push_back(displacement_attribute_descriptions[0]);



	VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
	vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_create_info.vertexBindingDescriptionCount = 2;
	vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()+displacement_attribute_descriptions.size());

	vertex_input_create_info.pVertexBindingDescriptions = input_binding_descriptions.data();
	vertex_input_create_info.pVertexAttributeDescriptions = input_attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	//set up the viewport
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapchain_extent.width;
	viewport.height = (float)m_swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = m_swapchain_extent;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = &viewport;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = &scissor;

	//Set up rasterization
	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	//TODO: this is where you wireframe, REQUIRES A GPU FEATURE
	rasterization_state_create_info.polygonMode = (m_enable_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL);
	rasterization_state_create_info.lineWidth = 1.0f;
	rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	//has to be counter clockwise because of projection matrix y-flip
	rasterization_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.0f; // Optional
	rasterization_state_create_info.depthBiasClamp = 0.0f;			// Optional
	rasterization_state_create_info.depthBiasSlopeFactor = 0.0f;	// Optional

	//TODO:Depth stencil goes here
	

	//configure multisampling for anti-aliasing
	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state_create_info.minSampleShading = 1.0f;			// Optional
	multisample_state_create_info.pSampleMask = nullptr;			// Optional
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE; // Optional
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;		// Optional

	//configure color blending between the new image and the old image, in our case, we do not desire color blending
	VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
	color_blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment_state.blendEnable = VK_FALSE;
	color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {};
	color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.logicOpEnable = VK_FALSE;
	color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_state_create_info.attachmentCount = 1;
	color_blend_state_create_info.pAttachments = &color_blend_attachment_state;
	color_blend_state_create_info.blendConstants[0] = 0.0f;
	color_blend_state_create_info.blendConstants[1] = 0.0f;
	color_blend_state_create_info.blendConstants[2] = 0.0f;
	color_blend_state_create_info.blendConstants[3] = 0.0f;

	//configure states that can be changed without pipeline recreation
	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH};

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = 2;
	dynamic_state_create_info.pDynamicStates = dynamic_states;

	//configure the pipeline layout to use the uniform buffer and texture sampler at a later point
	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 1;
	pipeline_layout_create_info.pSetLayouts = &m_descriptor_set_layout;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_logical_device, &pipeline_layout_create_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
	{
		throw std::runtime_error("Pipeline layout creation failed");
	}

	//put it all together and create the graphics pipeline
	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = 3;
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	graphics_pipeline_create_info.pDepthStencilState = nullptr;
	graphics_pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = nullptr;

	graphics_pipeline_create_info.layout = m_pipeline_layout;

	graphics_pipeline_create_info.renderPass = m_render_pass;
	//index of the sub pass where this graphics pipeline will be used
	graphics_pipeline_create_info.subpass = 0;

	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(m_logical_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS)
	{
		throw std::runtime_error("Graphics Pipeline creation failed");
	}

	//destroy the shader modules, they are in the pipeline now and no longer needed here
	vkDestroyShaderModule(m_logical_device, frag_shader_module, nullptr);
	vkDestroyShaderModule(m_logical_device, geom_shader_module, nullptr);
	vkDestroyShaderModule(m_logical_device, vert_shader_module, nullptr);

	succ("Created graphics pipeline");
}

//Framebuffer is a wrapper for the attachments created during render pass creation
void Application::create_framebuffers()
{
	info("Creating framebuffers...");
	//make sure we have enough framebuffers
	m_swapchain_framebuffers.resize(m_swapchain_image_views.size());

	//create a framebuffer for each imageview
	for (size_t i = 0; i < m_swapchain_image_views.size(); i++)
	{
		VkImageView attachments[] = {
			m_swapchain_image_views[i]};

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = m_render_pass;
		framebuffer_create_info.attachmentCount = 1;
		framebuffer_create_info.pAttachments = attachments;
		framebuffer_create_info.width = m_swapchain_extent.width;
		framebuffer_create_info.height = m_swapchain_extent.height;
		framebuffer_create_info.layers = 1;

		if (vkCreateFramebuffer(m_logical_device, &framebuffer_create_info, nullptr, &m_swapchain_framebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Framebuffer creation failed");
		}
	}
	succ("Framebuffers created");
}

//command pools are needed to manage memory of commandbuffers
void Application::create_command_pool()
{
	info("Creating Command Pool");
	QueueFamilyIndices queue_family_indices = find_queue_families(m_physical_device);

	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex = queue_family_indices.graphics_family;
	command_pool_create_info.flags = 0;

	if (vkCreateCommandPool(m_logical_device, &command_pool_create_info, nullptr, &m_command_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
	succ("Command Pool created");
}

//creates a texture image and puts it on device memory
void Application::create_texture_image()
{
	info("Creating Texture Image...");

	//load image
	int texture_width, texture_height, texture_channels;
	stbi_uc *pixels = stbi_load("textures/texture.jpg", &texture_width, &texture_height, &texture_channels, STBI_rgb_alpha);
	VkDeviceSize image_size = texture_width * texture_height * 4;

	if (!pixels)
	{
		throw std::runtime_error("Failed to load texture");
	}

	//prepare the staging buffer
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;

	create_buffer(image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	//copy the image to the staging buffer
	void *data;
	vkMapMemory(m_logical_device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(image_size));
	vkUnmapMemory(m_logical_device, staging_buffer_memory);

	//free the image memory
	stbi_image_free(pixels);

	//create the destination image
	create_image(texture_width, texture_height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_texture_image, m_texture_image_memory);

	//copy the staging buffer to the destination image
	transition_image_layout(m_texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(staging_buffer, m_texture_image, static_cast<uint32_t>(texture_width), static_cast<uint32_t>(texture_height));
	transition_image_layout(m_texture_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//destroy unused buffers
	vkDestroyBuffer(m_logical_device, staging_buffer, nullptr);
	vkFreeMemory(m_logical_device, staging_buffer_memory, nullptr);

	succ("Texture Image created");
}

//creates an image view for the texture
void Application::create_texture_image_view()
{
	m_texture_image_view = create_image_view(m_texture_image, VK_FORMAT_R8G8B8A8_UNORM);
}

//create the sampler to use in the shader
void Application::create_texture_sampler()
{
	info("Creating Texture Sampler...");
	VkSamplerCreateInfo sampler_create_info = {};
	sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_create_info.magFilter = VK_FILTER_LINEAR;
	sampler_create_info.minFilter = VK_FILTER_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.anisotropyEnable = VK_TRUE;
	sampler_create_info.maxAnisotropy = 16;
	sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	sampler_create_info.unnormalizedCoordinates = VK_FALSE;
	sampler_create_info.compareEnable = VK_FALSE;
	sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(m_logical_device, &sampler_create_info, nullptr, &m_texture_sampler) != VK_SUCCESS)
	{
		throw std::runtime_error("Texture Sampler Creation failed");
	}
	succ("Texture Sampler Created");
}

//create the vertex buffer used in the shader
void Application::create_vertex_buffer()
{
	info("Creating vertex buffer...");
	//determine vertex buffer size
	VkDeviceSize buffer_size = sizeof(m_vertices[0]) * m_vertices.size();

	//create staging buffer
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	//copy data to staging buffer
	void *data;
	vkMapMemory(m_logical_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, m_vertices.data(), (size_t)buffer_size);
	vkUnmapMemory(m_logical_device, staging_buffer_memory);

	//create target buffer
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertex_buffer, m_vertex_buffer_memory);

	//copy to target buffer
	copy_buffer(staging_buffer, m_vertex_buffer, buffer_size);

	//destroy staging buffer
	vkDestroyBuffer(m_logical_device, staging_buffer, nullptr);
	vkFreeMemory(m_logical_device, staging_buffer_memory, nullptr);

	succ("Vertex buffer created");
}

//create the index buffer responding to the vertex buffer
void Application::create_index_buffer()
{
	info("Creating Index Buffer...");
	//determine size of buffer
	VkDeviceSize buffer_size = sizeof(m_indices[0]) * m_indices.size();

	//create a staging buffer
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer, staging_buffer_memory);

	//copy index data to buffer
	void *data;
	vkMapMemory(m_logical_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, m_indices.data(), (size_t)buffer_size);
	vkUnmapMemory(m_logical_device, staging_buffer_memory);

	//create target buffer
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_index_buffer, m_index_buffer_memory);

	//copy to target buffer
	copy_buffer(staging_buffer, m_index_buffer, buffer_size);

	//destroy staging buffer
	vkDestroyBuffer(m_logical_device, staging_buffer, nullptr);
	vkFreeMemory(m_logical_device, staging_buffer_memory, nullptr);

	succ("Index Buffer created");
}

void Application::create_displacement_buffer()
{
	//every vertex needs a displacement
	VkDeviceSize buffer_size = sizeof(Displacement) * m_vertices.size();

	create_buffer(buffer_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_displacement_buffer, m_displacement_memory);

}

//create the uniform buffer
void Application::create_uniform_buffer()
{
	info("Creating Uniform Buffer...");
	VkDeviceSize buffer_size = sizeof(UniformBufferObject);
	create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniform_buffer, m_uniform_buffer_memory);
	succ("Uniform Buffer created");
}

//create a descriptor pool for descriptor set creation
void Application::create_descriptor_pool()
{
	info("Creating Descriptor Pool...");

	//2 pools, uniform buffer and texture sampler
	std::array<VkDescriptorPoolSize, 2> descriptor_pool_sizes = {};
	descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_pool_sizes[0].descriptorCount = 1;
	descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_pool_sizes[1].descriptorCount = 1;
	//create descriptor pool
	VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
	descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
	descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
	descriptor_pool_create_info.maxSets = 1;

	if (vkCreateDescriptorPool(m_logical_device, &descriptor_pool_create_info, nullptr, &m_descriptor_pool) != VK_SUCCESS)
	{
		throw std::runtime_error("Descriptor pool creation failed");
	}
	succ("Descriptor Pool created");
}

//create the descriptor sets which will be accessible from the shader
void Application::create_descriptor_set()
{
	info("Creating Descriptor Set...");
	//define the layout
	VkDescriptorSetLayout descriptor_set_layouts[] = {m_descriptor_set_layout};
	VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
	descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptor_set_allocate_info.descriptorPool = m_descriptor_pool;
	descriptor_set_allocate_info.descriptorSetCount = 1;
	descriptor_set_allocate_info.pSetLayouts = descriptor_set_layouts;

	if (vkAllocateDescriptorSets(m_logical_device, &descriptor_set_allocate_info, &m_descriptor_set) != VK_SUCCESS)
	{
		throw std::runtime_error("Descriptor set allocation failed");
	}

	//define descriptors
	VkDescriptorBufferInfo descriptor_buffer_info = {};
	descriptor_buffer_info.buffer = m_uniform_buffer;
	descriptor_buffer_info.offset = 0;
	descriptor_buffer_info.range = sizeof(UniformBufferObject);

	VkDescriptorImageInfo descriptor_image_info = {};
	descriptor_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descriptor_image_info.imageView = m_texture_image_view;
	descriptor_image_info.sampler = m_texture_sampler;

	std::array<VkWriteDescriptorSet, 2> write_descriptor_sets = {};
	write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].dstSet = m_descriptor_set;
	write_descriptor_sets[0].dstBinding = 0;
	write_descriptor_sets[0].dstArrayElement = 0;
	write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].descriptorCount = 1;
	write_descriptor_sets[0].pBufferInfo = &descriptor_buffer_info;

	write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[1].dstSet = m_descriptor_set;
	write_descriptor_sets[1].dstBinding = 1;
	write_descriptor_sets[1].dstArrayElement = 0;
	write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_sets[1].descriptorCount = 1;
	write_descriptor_sets[1].pImageInfo = &descriptor_image_info;

	vkUpdateDescriptorSets(m_logical_device, static_cast<uint32_t>(write_descriptor_sets.size()), write_descriptor_sets.data(), 0, nullptr);

	succ("Descriptor Set created");
}

void Application::create_command_buffers()
{
	info("Creating Command Buffers...");
	//allocate command buffers
	m_command_buffers.resize(m_swapchain_framebuffers.size());

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = m_command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = (uint32_t)m_command_buffers.size();

	if (vkAllocateCommandBuffers(m_logical_device, &command_buffer_allocate_info, m_command_buffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Commandbuffer allocation failed");
	}
	succ("Command buffer allocated");

	//record the command buffer
	info("Recording Command Buffers...");
	for (size_t i = 0; i < m_command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo command_buffer_begin_info = {};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		command_buffer_begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(m_command_buffers[i], &command_buffer_begin_info);

		vkCmdBindDescriptorSets(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_layout, 0, 1, &m_descriptor_set, 0, nullptr);

		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = m_render_pass;
		render_pass_begin_info.framebuffer = m_swapchain_framebuffers[i];
		render_pass_begin_info.renderArea.offset = {0, 0};
		render_pass_begin_info.renderArea.extent = m_swapchain_extent;

		VkClearValue clear_value = {0.0f, 0.0f, 0.0f, 1.0f};
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_value;

		vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

		VkBuffer vertex_buffers[] = { m_vertex_buffer };
		VkBuffer displacement_buffers[] = {m_displacement_buffer};
		VkDeviceSize offsets[] = {0};

		vkCmdBindVertexBuffers(m_command_buffers[i], 0, 1, vertex_buffers, offsets);
		vkCmdBindVertexBuffers(m_command_buffers[i], 1, 1, displacement_buffers, offsets);

		vkCmdBindIndexBuffer(m_command_buffers[i], m_index_buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(m_command_buffers[i], static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(m_command_buffers[i]);

		if (vkEndCommandBuffer(m_command_buffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Command Buffer Recording failed.");
		}
	}
	succ("Command Buffers recorded.");
}

//creates the semaphores needed to signal that an image is ready for rendering or presentation
void Application::create_semaphores()
{
	info("Creating semaphores...");
	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(m_logical_device, &semaphore_create_info, nullptr, &m_image_available_semaphore) != VK_SUCCESS || vkCreateSemaphore(m_logical_device, &semaphore_create_info, nullptr, &m_render_finished_semaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("Semaphore creation failed");
	}
	succ("Semaphores created");
}

#pragma endregion

void Application::draw_frame()
{
	//check if the image we want to render to is suitable
	uint32_t image_index;
	VkResult drawing_result = vkAcquireNextImageKHR(m_logical_device, m_swapchain, std::numeric_limits<uint64_t>::max(), m_image_available_semaphore, VK_NULL_HANDLE, &image_index);

	if (drawing_result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain();
		return;
	}
	else if (drawing_result != VK_SUCCESS && drawing_result == VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed acquiring swapchain image");
	}

	//submit the rendering commands form command buffers
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = {m_image_available_semaphore};

	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[image_index];

	VkSemaphore signal_semaphores[] = {m_render_finished_semaphore};
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;
	if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS)
	{
		throw std::runtime_error("Draw Command submission failed");
	}

	//present the rendered image
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = {m_swapchain};
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	//if the result is outdated or not optimal, recreate the swapchain
	//if its not success, throw an error
	drawing_result = vkQueuePresentKHR(m_presentation_queue, &present_info);
	if (drawing_result == VK_ERROR_OUT_OF_DATE_KHR || drawing_result == VK_SUBOPTIMAL_KHR)
	{
		recreate_swapchain();
	}
	else if (drawing_result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swapchain image");
	}

	
}

//update uniform buffer objects with fresh values
void Application::update_buffers()
{
	static auto start_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.projection = glm::perspective(glm::radians(45.0f), m_swapchain_extent.width / (float)m_swapchain_extent.height, 0.1f, 10.0f);
	//change y sign because glms clip coordinate is inverted, was designed for opengl, not vulkan after all
	ubo.projection[1][1] *= -1;

	//TODO: change this line to run after draw but before wait for idle;
	m_displacements = m_ocean->update_waves(time);

	void *data;
	vkMapMemory(m_logical_device, m_uniform_buffer_memory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(m_logical_device, m_uniform_buffer_memory);

	void *displacement_data;
	VkDeviceSize buffer_size = sizeof(Displacement)*m_displacements.size();
	vkMapMemory(m_logical_device, m_displacement_memory, 0, buffer_size, 0, &displacement_data);
	memcpy(displacement_data, m_displacements.data(), (size_t)buffer_size);
	vkUnmapMemory(m_logical_device, m_displacement_memory);
}

//recreates the swapchain, for example in the event the current one is not suitable anymore
void Application::recreate_swapchain()
{
	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	if (width == 0 || height == 0)
		return;

	vkDeviceWaitIdle(m_logical_device);

	clean_up_swapchain();

	create_swapchain();
	create_image_views();
	create_render_pass();
	create_graphics_pipeline();
	create_framebuffers();
	create_command_buffers();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Queue Families
////
/////////////////////////////////////////////////////////////////////////////////////////////////////

//looks for queues to use
QueueFamilyIndices Application::find_queue_families(VkPhysicalDevice physical_device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

	info("Looking for Queues...");
	int queue_index = 0;
	for (const auto &queue_family : queue_families)
	{
		//Can it render???
		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family = queue_index;
			info(std::string("\tQueue ") + std::to_string(queue_index) + " has a graphics bit");
		}
		//Can it present???
		VkBool32 presentation_support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, m_surface, &presentation_support);

		if (queue_family.queueCount > 0 && presentation_support != VK_FALSE)
		{
			indices.presentation_family = queue_index;
			info(std::string("\tQueue ") + std::to_string(queue_index) + " is able to present");
		}

		if (indices.isComplete())
		{
			succ("\tQueues are suitable, aborting search");
			break;
		}

		queue_index++;
	}
	return indices;
}

//retrieves the details of the swapchain
SwapChainSupportDetails Application::query_swapchain_support(VkPhysicalDevice device)
{
	info("Querying swapchain support...");
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
	if (format_count)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);
	if (present_mode_count)
	{
		details.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, details.present_modes.data());
	}
	succ("Swapchain details retrieved");
	return details;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Vulkan Helper Stuff
////
/////////////////////////////////////////////////////////////////////////////////////////////////////

//retrieves the extensions needed to initialize an instance of vulkan and a glfw window
std::vector<const char *> Application::get_required_instance_extensions()
{
	info("Getting required instance extensions...");
	uint32_t glfw_required_extension_count = 0;
	const char **glfw_extension_names;

	glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);

	std::vector<const char *> final_required_extensions(glfw_extension_names, glfw_extension_names + glfw_required_extension_count);

	//add validation layers if needed
	if (enableValidationLayers)
	{
		final_required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}
	return final_required_extensions;
}

//scores a physical device on feature availability and type
int Application::evaluate_physical_device_capabilities(VkPhysicalDevice physical_device)
{
	info("Evaluating Physical devices...");
	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(physical_device, &device_properties);

	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(physical_device, &device_features);

	int score = 0;

	switch (device_properties.deviceType)
	{
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
	if (!device_features.fillModeNonSolid)
	{
		warn("Wireframe not supported, switching to wireframe will not be available.");
		//TODO: dont allow wireframe switching
	}

	if (!device_features.samplerAnisotropy)
	{
		warn("Anisotrpic filter not supported");
		//TODO: dont allow anisotropic filtering
	}

	if (!device_features.geometryShader)
	{
		score = 0;
		warn(std::string("\t") + device_properties.deviceName + " has no geometry shader and is therefore scoring " + std::to_string(score));
	}
	if (!find_queue_families(physical_device).isComplete())
	{
		score = 0;
		warn(std::string("\t") + device_properties.deviceName + " has failed queue checks and is therefore scoring " + std::to_string(score));
	}
	if (!check_device_extension_support(physical_device))
	{
		score = 0;
		warn(std::string("\t") + device_properties.deviceName + " has failed extension checks and is therefore scoring " + std::to_string(score));
	}
	//further tests that require extensions
	else
	{
		bool swapchain_adequate = false;
		SwapChainSupportDetails swapchain_support = query_swapchain_support(physical_device);
		swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
		if (!swapchain_adequate)
		{
			score = 0;
			warn(std::string("\t") + device_properties.deviceName + " does not offer sufficient swapchain support and is therefore scoring " + std::to_string(score));
		}
	}

	info(std::string("\t") + device_properties.deviceName + std::string(" is scoring ") + std::to_string(score));

	return score;
}

//checks if the required extensions are supported
bool Application::check_extension_support(std::vector<const char *> required_extensions, std::vector<VkExtensionProperties> supported_extensions)
{
	//list supported extensions
	info("Checking Extension Support...");
	std::set<std::string> required_extensions_set(required_extensions.begin(), required_extensions.end());

	for (const VkExtensionProperties supported_extension : supported_extensions)
	{
		std::string extension_output = (std::string("\t") + supported_extension.extensionName);
		required_extensions_set.erase(supported_extension.extensionName) ? succ(extension_output) : info(extension_output);
		;
#ifndef _DEBUG
		//if its not debugging, stop searching once its empty
		if (required_extensions_set.empty())
			break;
#endif
	}
	//if its empty all extensions are supported
	return required_extensions_set.empty();
}

//checks if the device extensions are supported
bool Application::check_device_extension_support(VkPhysicalDevice physical_device)
{
	info("Checking device extension support...");

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> supported_extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, supported_extensions.data());
	return check_extension_support(device_extensions, supported_extensions);
}

//checks if instance extensions are supported
bool Application::check_instance_extension_support(std::vector<const char *> required_extensions)
{
	info("Checking instance extension support...");
	uint32_t supported_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);
	//enumerate supported extensions
	std::vector<VkExtensionProperties> supported_extensions(supported_extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());
	return check_extension_support(required_extensions, supported_extensions);
}

//creates a shader module from the given bytecode
VkShaderModule Application::create_shader_module(const std::vector<char> &code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());
	VkShaderModule shader_module;
	if (vkCreateShaderModule(m_logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		throw std::runtime_error("ShaderModule creation failed");
	}
	return shader_module;
}

//returns the memory type flag that fulfills the properties needed
uint32_t Application::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(m_physical_device, &memory_properties);

	//work bit vodoo with flags
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		//return if the memory type fits the type filter and fulfills the needed properties
		if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	throw std::runtime_error("No suitable memory found");
}

//transitions between two image layouts, currently only for 2 cases
void Application::transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkImageMemoryBarrier image_memory_barrier = {};
	image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	image_memory_barrier.oldLayout = old_layout;
	image_memory_barrier.newLayout = new_layout;
	image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	image_memory_barrier.image = image;
	image_memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_memory_barrier.subresourceRange.baseMipLevel = 0;
	image_memory_barrier.subresourceRange.levelCount = 1;
	image_memory_barrier.subresourceRange.baseArrayLayer = 0;
	image_memory_barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = 0;
		image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition");
	}

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);

	end_single_time_commands(command_buffer);
}

//copies from a buffer to an image
void Application::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	info("Copying buffer to image...");
	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	end_single_time_commands(command_buffer);
	succ("Buffer copied to image");
}

//begins a single time command buffer recording
VkCommandBuffer Application::begin_single_time_commands()
{
	info("Beginning single time commands...");
	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandPool = m_command_pool;
	command_buffer_allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(m_logical_device, &command_buffer_allocate_info, &command_buffer);

	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

	succ("Single time commands begun");
	return command_buffer;
}

//ends the single time recording and submits the recorded commandbuffer
void Application::end_single_time_commands(VkCommandBuffer command_buffer)
{
	info("Ending single time commands...");
	vkEndCommandBuffer(command_buffer);
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_graphics_queue);

	vkFreeCommandBuffers(m_logical_device, m_command_pool, 1, &command_buffer);
	succ("Single time commands ended");
}

//create a buffer
void Application::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &buffer_memory)
{
	info("\tCreating buffer...");
	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_logical_device, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Buffer creation failed");
	}

	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(m_logical_device, buffer, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_logical_device, &memory_allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Buffer memory allocatiion failed");
	}

	vkBindBufferMemory(m_logical_device, buffer, buffer_memory, 0);
	succ("Buffer created successfully");
}

//creates an image
void Application::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VkDeviceMemory &image_memory)
{
	info("Creating image...");
	VkImageCreateInfo image_create_info = {};
	image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_create_info.imageType = VK_IMAGE_TYPE_2D;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1;
	image_create_info.mipLevels = 1;
	image_create_info.arrayLayers = 1;
	image_create_info.format = format;
	image_create_info.tiling = tiling;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.usage = usage;
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;

	if (vkCreateImage(m_logical_device, &image_create_info, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("Image creation failed");
	}

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(m_logical_device, image, &memory_requirements);

	VkMemoryAllocateInfo memory_allocate_info = {};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_logical_device, &memory_allocate_info, nullptr, &image_memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Memory allocation failed");
	}

	vkBindImageMemory(m_logical_device, image, image_memory, 0);
	succ("Image created");
}

//creates an image view
VkImageView Application::create_image_view(VkImage image, VkFormat format)
{
	info("Creating Image View...");
	VkImageViewCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	create_info.image = image;
	create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	create_info.format = format;
	create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	create_info.subresourceRange.baseMipLevel = 0;
	create_info.subresourceRange.levelCount = 1;
	create_info.subresourceRange.baseArrayLayer = 0;
	create_info.subresourceRange.layerCount = 1;

	VkImageView image_view;

	if (vkCreateImageView(m_logical_device, &create_info, nullptr, &image_view) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture image views");
	}
	succ("Image View Created");
	return image_view;
}

//copies buffer a to buffer b
void Application::copy_buffer(VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	info("Copying buffer...");

	VkCommandBuffer command_buffer = begin_single_time_commands();

	VkBufferCopy copy_region = {};
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

	end_single_time_commands(command_buffer);

	succ("Buffer copied");
}

//the debug callback used for the validation layers
VKAPI_ATTR VkBool32 VKAPI_CALL Application::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char *layerPrefix, const char *msg, void *userData)
{
	info(std::string("VALIDATION LAYERS:\n") + msg, console_colors_foreground::white, console_colors_background::red);

	return VK_FALSE;
}

//handles window resizing
void Application::on_window_resized(GLFWwindow *window, int width, int height)
{

	Application *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
	app->recreate_swapchain();
}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	////
	////							Selection Functions
	////
	/////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Selection Functions

//selects the most suitable swapchain surface format
VkSurfaceFormatKHR Application::choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
	info("Choosing surface format...");
	if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	for (const VkSurfaceFormatKHR &available_format : available_formats)
	{
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return available_format;
		}
	}
	return available_formats[0];
}

//selects the most suitable swapchain present mode
VkPresentModeKHR Application::choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> available_present_modes)
{
	info("Choosing present mode...");
	VkPresentModeKHR optimal_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const VkPresentModeKHR available_present_mode : available_present_modes)
	{
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			//best mode, return immediately
			return available_present_mode;
		}
		else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			optimal_mode = available_present_mode;
		}
	}
	return optimal_mode;
}

//selects the most suitable swapchain extent
VkExtent2D Application::choose_swapchain_extent(const VkSurfaceCapabilitiesKHR &capabilities)
{
	info("Choosing extent...");
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetWindowSize(m_window, &width, &height);
		VkExtent2D actual_extent = {width, height};
		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));
		return actual_extent;
	}
}
#pragma endregion

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	////
	////							Proxy Functions
	////
	/////////////////////////////////////////////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	////
	////							Cleanup
	////
	/////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Clean Up

//cleans up everything that depends ont he swapchain
void Application::clean_up_swapchain()
{
	info("Cleaning up swapchain...");
	for (auto framebuffer : m_swapchain_framebuffers)
	{
		vkDestroyFramebuffer(m_logical_device, framebuffer, nullptr);
	}
	vkFreeCommandBuffers(m_logical_device, m_command_pool, static_cast<uint32_t>(m_command_buffers.size()), m_command_buffers.data());

	vkDestroyPipeline(m_logical_device, m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(m_logical_device, m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_logical_device, m_render_pass, nullptr);
	for (VkImageView image_view : m_swapchain_image_views)
	{
		vkDestroyImageView(m_logical_device, image_view, nullptr);
	}
	vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
	succ("Swapchain cleaned successfully");
}

//destroys all vulkan objects and frees memory
//called when the application is closed
void Application::clean_up()
{
	info("Cleaning up...");
	clean_up_swapchain();

	vkDestroySampler(m_logical_device, m_texture_sampler, nullptr);
	vkDestroyImageView(m_logical_device, m_texture_image_view, nullptr);

	vkDestroyImage(m_logical_device, m_texture_image, nullptr);
	vkFreeMemory(m_logical_device, m_texture_image_memory, nullptr);

	vkDestroyDescriptorPool(m_logical_device, m_descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(m_logical_device, m_descriptor_set_layout, nullptr);
	vkDestroyBuffer(m_logical_device, m_uniform_buffer, nullptr);
	vkFreeMemory(m_logical_device, m_uniform_buffer_memory, nullptr);

	vkDestroyBuffer(m_logical_device, m_displacement_buffer, nullptr);
	vkFreeMemory(m_logical_device, m_displacement_memory, nullptr);

	vkDestroyBuffer(m_logical_device, m_index_buffer, nullptr);
	vkFreeMemory(m_logical_device, m_index_buffer_memory, nullptr);
	vkDestroyBuffer(m_logical_device, m_vertex_buffer, nullptr);
	vkFreeMemory(m_logical_device, m_vertex_buffer_memory, nullptr);
	vkDestroySemaphore(m_logical_device, m_render_finished_semaphore, nullptr);
	vkDestroySemaphore(m_logical_device, m_image_available_semaphore, nullptr);

	vkDestroyCommandPool(m_logical_device, m_command_pool, nullptr);

	vkDestroyDevice(m_logical_device, nullptr);
	DestroyDebugReportCallbackEXT(m_instance, callback, nullptr);
	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);
	glfwTerminate();
	succ("Cleanup complete");
}

#pragma endregion