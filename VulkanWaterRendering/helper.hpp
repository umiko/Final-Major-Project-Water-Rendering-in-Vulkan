#pragma once

#include <fstream>
#include <string>
#include <vector>

#include <vulkan\vulkan.h>

#include "logger.hpp"
//compares two VkExtensionProperties by name. Used to sort extensions
static const bool compare_extensions(VkExtensionProperties &extensionA, VkExtensionProperties &extensionB) {
	return extensionA.extensionName > extensionB.extensionName;
}

//this all goes to hell if not build for windows, because linux already supports that stuff

//this will read a file to a buffer and return this buffer
static std::vector<char> read_file(const std::string &filename) {
	info("Reading file...");
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file");
	}
	size_t file_size = (size_t)file.tellg();
	std::vector<char> buffer(file_size);
	file.seekg(0);
	file.read(buffer.data(), file_size);

	file.close();
	info(std::string("File read, size: ") + std::to_string(file_size));
	return buffer;
}

static float random(float max = 1) {
	float rdm = rand();
	return (rdm / RAND_MAX) * max;
}