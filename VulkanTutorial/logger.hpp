#pragma once
#define COLORMODE

#ifdef COLORMODE
//windows likes to redefine standard functions. bad dog
#define NOMINMAX
#include <Windows.h>
//this enables cmd to use virtual terminal stuff, like color codes and such
inline DWORD enable_virtual_terminal() {
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
#endif

//enumerations conveniently starting at the numbers where the color codes begin
enum class console_colors_foreground { black = 30, red, green, yellow, blue, purple, cyan, white };
enum class console_colors_background { black = 40, red, green, yellow, blue, purple, cyan, white };
//the escape character to use those codes
inline const char* esc_char = { "\x1b[" };
const char  esc_color_end_char = 'm';
const char	esc_color_reset_char = '0';

//console output, if no color info is submitted it will assume default colors
template <class T> void info(T value, console_colors_foreground foreground_color_code = console_colors_foreground::white, console_colors_background background_color_code = console_colors_background::black) {
#ifdef _DEBUG
#ifdef COLORMODE
	//first, set the wanted colours
	std::cout << esc_char << (int)foreground_color_code << ';' << (int)background_color_code << esc_color_end_char;
#endif
	//then display the message
	std::cout << value << std::endl;
#ifdef COLORMODE
	//reset the colors
	std::cout << esc_char << esc_color_reset_char << esc_color_end_char;
#endif
#endif // !_DEBUG
}

//this will use the info function, but give it a predefined color
template <class T> void warn(T value) {
	info(value, console_colors_foreground::yellow);
}

//this will use the info function, but give it a predefined color
template <class T> void err(T value) {
	info(value, console_colors_foreground::red);
}

//this will use the info function, but give it a predefined color
template <class T> void succ(T value) {
	info(value, console_colors_foreground::green);
}
