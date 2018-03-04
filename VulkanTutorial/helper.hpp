#ifndef HELPER_HPP
#define HELPER_HPP

#include <vulkan\vulkan.h>
//compares two VkExtensionProperties by name. Used to sort extensions
const bool compare_extensions(VkExtensionProperties &extensionA, VkExtensionProperties &extensionB) {
	return extensionA.extensionName > extensionB.extensionName;
}

#endif