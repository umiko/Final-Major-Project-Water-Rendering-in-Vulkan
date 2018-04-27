#pragma once
#include <array>

#define GLM_FORCE_RADIANS
#include <glm/vec3.hpp>

#include <vulkan/vulkan.h>

struct Displacement {
	glm::vec3 displacement;

	static VkVertexInputBindingDescription get_binding_description()
	{
		VkVertexInputBindingDescription vertex_input_binding_description = {};
		vertex_input_binding_description.binding = 1;
		vertex_input_binding_description.stride = sizeof(Displacement);
		vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return vertex_input_binding_description;
	}

	static std::array<VkVertexInputAttributeDescription, 1> get_attribute_descriptions()
	{
		std::array<VkVertexInputAttributeDescription, 1> vertex_input_attribute_descriptions = {};
		vertex_input_attribute_descriptions[0].binding = 1;
		vertex_input_attribute_descriptions[0].location = 3;
		vertex_input_attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_input_attribute_descriptions[0].offset = offsetof(Displacement, displacement);

		return vertex_input_attribute_descriptions;
	}
};
