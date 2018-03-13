#ifndef HELPER_HPP
#define HELPER_HPP

#include <vulkan\vulkan.h>
//compares two VkExtensionProperties by name. Used to sort extensions
const bool compare_extensions(VkExtensionProperties &extensionA, VkExtensionProperties &extensionB) {
	return extensionA.extensionName > extensionB.extensionName;
}



DWORD enable_virtual_terminal() {
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

enum class console_colors_foreground { black = 30, red, green, yellow, blue, purple, cyan, white };
enum class console_colors_background { black = 40, red, green, yellow, blue, purple, cyan, white };
const char* esc_char = { "\x1b[" };
const char  esc_color_end_char = 'm';
const char	esc_color_reset_char = '0';

template <class T> void info(T value, console_colors_foreground foreground_color_code = console_colors_foreground::white, console_colors_background background_color_code = console_colors_background::black) {
	#ifdef _DEBUG
	//first, set the wanted colours
	std::cout << esc_char << (int)foreground_color_code << ';' << (int)background_color_code << esc_color_end_char; 
	//then display the message
	std::cout << value << std::endl;
	//reset the colors
	std::cout << esc_char << esc_color_reset_char << esc_color_end_char;

	#endif // !_DEBUG
}

template <class T> void warn(T value) {
	info(value, console_colors_foreground::yellow);
}

template <class T> void err(T value) {
	info(value, console_colors_foreground::red);
}

template <class T> void succ(T value) {
	info(value, console_colors_foreground::green);
}

#endif