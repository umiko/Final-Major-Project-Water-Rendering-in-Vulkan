#include "application.hpp"

int main() {
	Application application;
#ifdef COLORMODE
	enable_virtual_terminal();
#endif // DEBUG

	try {
		application.run();
	}
	catch (const std::runtime_error &error) {
		err(error.what());
		//return EXIT_FAILURE;
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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	m_window = glfwCreateWindow(WIDTH, HEIGHT, WINDOW_TITLE, nullptr, nullptr);

	glfwSetWindowUserPointer(m_window, this);
	glfwSetWindowSizeCallback(m_window, Application::on_window_resized);
}

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
	create_graphics_pipeline();
	create_framebuffers();
	create_command_pool();
	create_command_buffers();
	create_semaphores();
	succ("Vulkan Initialized");
}

void Application::main_loop()
{
	while (!glfwWindowShouldClose(m_window)) {
		glfwPollEvents();
		//TODO: update goes here
		draw_frame();
	}
	vkDeviceWaitIdle(m_logical_device);
}

#pragma region Initialization

void Application::create_instance()
{
	info("Creating Vulkan instance...");
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
	info("Checking validation layer support...");
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
	info("Picking physical device...");
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
		throw std::runtime_error("Your GPU is not suitable");
	}
}

void Application::create_surface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
		throw std::runtime_error("Window Surface creation failed");
	}
}

void Application::create_logical_device()
{
	info("Creating logical device...");
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
	//Set device features that need to be enabled
	VkPhysicalDeviceFeatures device_features = {};
	//TODO: Wireframe happens here
	device_features.fillModeNonSolid = VK_TRUE;

	VkDeviceCreateInfo logical_device_create_info = {};
	logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_informations.size());
	logical_device_create_info.pQueueCreateInfos = queue_create_informations.data();
	logical_device_create_info.pEnabledFeatures = &device_features;
	logical_device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	logical_device_create_info.ppEnabledExtensionNames = device_extensions.data();
	logical_device_create_info.enabledLayerCount = 0;
	//if validation layers are enabled overwrite the just set enabledLayerCount
	if (enableValidationLayers) {
		logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		logical_device_create_info.ppEnabledLayerNames = validation_layers.data();
	}

	if (vkCreateDevice(m_physical_device, &logical_device_create_info, nullptr, &m_logical_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}
	succ("Logical Device creation Successful!");
	//single queue, therefore index 0
	vkGetDeviceQueue(m_logical_device, indices.graphics_family, 0, &m_graphics_queue);
	//single queue, therefore index 0
	vkGetDeviceQueue(m_logical_device, indices.presentation_family, 0, &m_presentation_queue);
}

void Application::create_swapchain()
{
	info("Creating Swapchain...");
	SwapChainSupportDetails swapchain_support = query_swapchain_support(m_physical_device);

	VkSurfaceFormatKHR surface_format = choose_swapchain_surface_format(swapchain_support.formats);
	VkPresentModeKHR present_mode = choose_swapchain_present_mode(swapchain_support.present_modes);
	VkExtent2D extent = choose_swapchain_extent(swapchain_support.capabilities);
	//do triple buffering
	uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
	if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
		image_count = swapchain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = find_queue_families(m_physical_device);
	uint32_t queue_family_indices[] = { (uint32_t)indices.graphics_family, (uint32_t)indices.presentation_family };
	if (indices.graphics_family != indices.presentation_family) {
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}

	create_info.preTransform = swapchain_support.capabilities.currentTransform;

	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	create_info.presentMode = present_mode;

	create_info.clipped = VK_TRUE;

	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_logical_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Swapchain creation failed");
	}
	vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, nullptr);
	m_swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_logical_device, m_swapchain, &image_count, m_swapchain_images.data());

	m_swapchain_image_format = surface_format.format;
	m_swapchain_extent = extent;
	succ("Swapchain created successfully");
}

void Application::create_image_views()
{
	info("Creating image views...");
	m_swapchain_image_views.resize(m_swapchain_images.size());
	for (size_t i = 0; i < m_swapchain_images.size(); i++) {
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = m_swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = m_swapchain_image_format;

		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_logical_device, &create_info, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views");
		}
	}
	succ("Image Views created");
}

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

	VkSubpassDependency subpass_dependency = {};
	subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpass_dependency.dstSubpass = 0;
	subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask = 0;
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

	if (vkCreateRenderPass(m_logical_device, &render_pass_create_info, nullptr, &m_render_pass) != VK_SUCCESS) {
		throw std::runtime_error("Render Pass creation failed");
	}

	succ("Render Pass Created");
}

void Application::create_graphics_pipeline()
{
	info("Creating graphics pipeline...");
	std::vector<char> vert_shader_code = read_file("shaders/vert.spv");
	std::vector<char> frag_shader_code = read_file("shaders/frag.spv");

	VkShaderModule vert_shader_module;
	VkShaderModule frag_shader_module;

	vert_shader_module = create_shader_module(vert_shader_code);
	frag_shader_module = create_shader_module(frag_shader_code);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	auto vertex_binding_descriptions = Vertex::get_binding_description();
	auto vertex_attribute_descriptions = Vertex::get_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_create_info = {};
	vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_create_info.vertexBindingDescriptionCount = 1;	
	vertex_input_create_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size());

	vertex_input_create_info.pVertexBindingDescriptions = &vertex_binding_descriptions;
	vertex_input_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapchain_extent.width;
	viewport.height = (float)m_swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = m_swapchain_extent;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = &viewport;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	//TODO: this is where you wireframe, REQUIRES A GPU FEATURE
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_LINE;
	rasterization_state_create_info.lineWidth = 1.0f;
	rasterization_state_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterization_state_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.0f; // Optional
	rasterization_state_create_info.depthBiasClamp = 0.0f; // Optional
	rasterization_state_create_info.depthBiasSlopeFactor = 0.0f; // Optional

	//TODO:Depth stencil goes here

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state_create_info.minSampleShading = 1.0f; // Optional
	multisample_state_create_info.pSampleMask = nullptr; // Optional
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE; // Optional
	multisample_state_create_info.alphaToOneEnable = VK_FALSE; // Optional

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

	VkDynamicState dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = 2;
	dynamic_state_create_info.pDynamicStates = dynamic_states;

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 0;
	pipeline_layout_create_info.pSetLayouts = nullptr;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(m_logical_device, &pipeline_layout_create_info, nullptr, &m_pipeline_layout) != VK_SUCCESS) {
		throw std::runtime_error("Pipeline layout creation failed");
	}

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = 2;
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

	if (vkCreateGraphicsPipelines(m_logical_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Graphics Pipeline creation failed");
	}

	vkDestroyShaderModule(m_logical_device, frag_shader_module, nullptr);
	vkDestroyShaderModule(m_logical_device, vert_shader_module, nullptr);

	succ("Created graphics pipeline");
}

void Application::create_framebuffers()
{
	info("Creating framebuffers...");
	m_swapchain_framebuffers.resize(m_swapchain_image_views.size());

	for (size_t i = 0; i < m_swapchain_image_views.size(); i++) {
		VkImageView attachments[] = {
			m_swapchain_image_views[i]
		};

		VkFramebufferCreateInfo framebuffer_create_info = {};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = m_render_pass;
		framebuffer_create_info.attachmentCount = 1;
		framebuffer_create_info.pAttachments = attachments;
		framebuffer_create_info.width = m_swapchain_extent.width;
		framebuffer_create_info.height = m_swapchain_extent.height;
		framebuffer_create_info.layers = 1;

		if (vkCreateFramebuffer(m_logical_device, &framebuffer_create_info, nullptr, &m_swapchain_framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Framebuffer creation failed");
		}
	}
	succ("Framebuffers created");
}

void Application::create_command_pool()
{
	info("Creating Command Pool");
	QueueFamilyIndices queue_family_indices = find_queue_families(m_physical_device);

	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.queueFamilyIndex = queue_family_indices.graphics_family;
	command_pool_create_info.flags = 0;

	if (vkCreateCommandPool(m_logical_device, &command_pool_create_info, nullptr, &m_command_pool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool");
	}
	succ("Command Pool created");
}

void Application::create_command_buffers()
{
	info("Creating Command Buffers...");
	m_command_buffers.resize(m_swapchain_framebuffers.size());

	VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
	command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_allocate_info.commandPool = m_command_pool;
	command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_allocate_info.commandBufferCount = (uint32_t)m_command_buffers.size();

	if (vkAllocateCommandBuffers(m_logical_device, &command_buffer_allocate_info, m_command_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Commandbuffer allocation failed");
	}
	succ("Command buffer allocated");
	info("Recording Command Buffers...");
	for (size_t i = 0; i < m_command_buffers.size(); i++) {
		VkCommandBufferBeginInfo command_buffer_begin_info = {};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		command_buffer_begin_info.pInheritanceInfo = nullptr;

		vkBeginCommandBuffer(m_command_buffers[i], &command_buffer_begin_info);

		VkRenderPassBeginInfo render_pass_begin_info = {};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = m_render_pass;
		render_pass_begin_info.framebuffer = m_swapchain_framebuffers[i];
		render_pass_begin_info.renderArea.offset = { 0,0 };
		render_pass_begin_info.renderArea.extent = m_swapchain_extent;

		VkClearValue clear_value = { 0.0f,0.0f,0.0f,1.0f };
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_value;

		vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);

		vkCmdDraw(m_command_buffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_command_buffers[i]);

		if (vkEndCommandBuffer(m_command_buffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Command Buffer Recording failed.");
		}
	}
	succ("Command Buffers recorded.");
}

void Application::create_semaphores()
{
	info("Creating semaphores...");
	VkSemaphoreCreateInfo semaphore_create_info = {};
	semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(m_logical_device, &semaphore_create_info, nullptr, &m_image_available_semaphore) != VK_SUCCESS || vkCreateSemaphore(m_logical_device, &semaphore_create_info, nullptr, &m_render_finished_semaphore) != VK_SUCCESS) {
		throw std::runtime_error("Semaphore creation failed");
	}
	succ("Semaphores created");
}

#pragma endregion

void Application::draw_frame()
{
	uint32_t image_index;
	VkResult drawing_result = vkAcquireNextImageKHR(m_logical_device, m_swapchain, std::numeric_limits<uint64_t>::max(), m_image_available_semaphore, VK_NULL_HANDLE, &image_index);

	if (drawing_result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swapchain();
		return;
	}
	else if (drawing_result != VK_SUCCESS && drawing_result == VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed acquiring swapchain image");
	}

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { m_image_available_semaphore };

	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[image_index];

	VkSemaphore signal_semaphores[] = { m_render_finished_semaphore };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;
	if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("Draw Command submission failed");
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	drawing_result = vkQueuePresentKHR(m_presentation_queue, &present_info);
	if (drawing_result == VK_ERROR_OUT_OF_DATE_KHR || drawing_result == VK_SUBOPTIMAL_KHR) {
		recreate_swapchain();
	}
	else if (drawing_result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swapchain image");
	}

	vkQueueWaitIdle(m_presentation_queue);
}

void Application::recreate_swapchain()
{

	int width, height;
	glfwGetWindowSize(m_window, &width, &height);
	if (width == 0 || height == 0) return;

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

QueueFamilyIndices Application::find_queue_families(VkPhysicalDevice physical_device)
{
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

	info("Looking for Queues...");
	int queue_index = 0;
	for (const auto& queue_family : queue_families) {
		//Can it render???
		if (queue_family.queueCount > 0 && queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = queue_index;
			info(std::string("\tQueue ") + std::to_string(queue_index) + " has a graphics bit");
		}
		//Can it present???
		VkBool32 presentation_support = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_index, m_surface, &presentation_support);

		if (queue_family.queueCount > 0 && presentation_support != VK_FALSE) {
			indices.presentation_family = queue_index;
			info(std::string("\tQueue ") + std::to_string(queue_index) + " is able to present");
		}

		if (indices.isComplete()) {
			succ("\tQueues are suitable, aborting search");
			break;
		}

		queue_index++;
	}
	return indices;
}

SwapChainSupportDetails Application::query_swapchain_support(VkPhysicalDevice device)
{
	info("Querying swapchain support...");
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, nullptr);
	if (format_count) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &present_mode_count, nullptr);
	if (present_mode_count) {
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

std::vector<const char*> Application::get_required_instance_extensions()
{
	info("Getting required instance extensions...");
	uint32_t glfw_required_extension_count = 0;
	const char** glfw_extension_names;

	glfw_extension_names = glfwGetRequiredInstanceExtensions(&glfw_required_extension_count);

	std::vector<const char*> final_required_extensions(glfw_extension_names, glfw_extension_names + glfw_required_extension_count);

	if (enableValidationLayers)
		final_required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	return final_required_extensions;
}

int Application::evaluate_physical_device_capabilities(VkPhysicalDevice physical_device) {
	info("Evaluating Physical devices...");
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
	if (device_features.fillModeNonSolid) {
		warn("Wireframe not supported, switching to wireframe will not be available.");
		//TODO: dont allow wireframe switching
	}

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
	//further tests that require extensions
	else {
		bool swapchain_adequate = false;
		SwapChainSupportDetails swapchain_support = query_swapchain_support(physical_device);
		swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
		if (!swapchain_adequate) {
			score = 0;
			warn(std::string("\t") + device_properties.deviceName + " does not offer sufficient swapchain support and is therefore scoring " + std::to_string(score));
		}
	}

	info(std::string("\t") + device_properties.deviceName + std::string(" is scoring ") + std::to_string(score));

	return score;
}

bool Application::check_extension_support(std::vector<const char*> required_extensions, std::vector<VkExtensionProperties> supported_extensions) {
	//list supported extensions
	info("Checking Extension Support...");
	std::set<std::string> required_extensions_set(required_extensions.begin(), required_extensions.end());

	for (const VkExtensionProperties supported_extension : supported_extensions) {
		std::string extension_output = (std::string("\t") + supported_extension.extensionName);
		required_extensions_set.erase(supported_extension.extensionName) ? succ(extension_output) : info(extension_output);;
#ifndef _DEBUG
		//if its not debugging, stop searching once its empty
		if (required_extensions_set.empty())
			break;
#endif
	}
	//if its empty all extensions are supported
	return required_extensions_set.empty();
}

bool Application::check_device_extension_support(VkPhysicalDevice physical_device)
{
	info("Checking device extension support...");

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> supported_extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensionCount, supported_extensions.data());
	return check_extension_support(device_extensions, supported_extensions);
}

bool Application::check_instance_extension_support(std::vector<const char*> required_extensions)
{
	info("Checking instance extension support...");
	uint32_t supported_extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);
	//enumerate supported extensions
	std::vector<VkExtensionProperties> supported_extensions(supported_extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());
	return check_extension_support(required_extensions, supported_extensions);
}



VkShaderModule Application::create_shader_module(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule shader_module;
	if (vkCreateShaderModule(m_logical_device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw std::runtime_error("ShaderModule creation failed");
	}
	return shader_module;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Application::debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char * layerPrefix, const char * msg, void * userData)
{
	info(std::string("VALIDATION LAYERS:\n") + msg, console_colors_foreground::white, console_colors_background::red);

	return VK_FALSE;
}

void Application::on_window_resized(GLFWwindow* window, int width, int height)
{
	Application *app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->recreate_swapchain();	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Selection Functions
////
/////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Selection Functions
VkSurfaceFormatKHR Application::choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats)
{
	info("Choosing surface format...");
	if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED) {
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const VkSurfaceFormatKHR &available_format : available_formats) {
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return available_format;
		}
	}
	return available_formats[0];
}

VkPresentModeKHR Application::choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> available_present_modes)
{
	info("Choosing present mode...");
	VkPresentModeKHR optimal_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const VkPresentModeKHR available_present_mode : available_present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			//best mode, return immediately
			return available_present_mode;
		}
		else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			optimal_mode = available_present_mode;
		}
	}
	return optimal_mode;
}

VkExtent2D Application::choose_swapchain_extent(const VkSurfaceCapabilitiesKHR & capabilities)
{
	info("Choosing extent...");
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetWindowSize(m_window, &width, &height);
		VkExtent2D actual_extent = { width, height };
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

#pragma region Vulkan Proxy Function


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

#pragma endregion

/////////////////////////////////////////////////////////////////////////////////////////////////////
////
////							Cleanup
////
/////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma region Clean Up


void Application::clean_up_swapchain()
{
	info("Cleaning up swapchain...");
	for (auto framebuffer : m_swapchain_framebuffers) {
		vkDestroyFramebuffer(m_logical_device, framebuffer, nullptr);
	}
	vkFreeCommandBuffers(m_logical_device, m_command_pool, static_cast<uint32_t>(m_command_buffers.size()), m_command_buffers.data());

	vkDestroyPipeline(m_logical_device, m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(m_logical_device, m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_logical_device, m_render_pass, nullptr);
	for (VkImageView image_view : m_swapchain_image_views) {
		vkDestroyImageView(m_logical_device, image_view, nullptr);
	}
	vkDestroySwapchainKHR(m_logical_device, m_swapchain, nullptr);
	succ("Swapchain cleaned successfully");
}



void Application::clean_up()
{
	info("Cleaning up...");
	clean_up_swapchain();
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
