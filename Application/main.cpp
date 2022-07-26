#include "gui.h"

#include <thread>

int __stdcall WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR arguments, int commandShow) {
	gui::CreateHWindow("PShell IDE", "pshellide");
	gui::CreateDevice();
	gui::CreateImGui();

	while (gui::exit) {
		gui::BeginRender();
		gui::Render();
		gui::EndRender();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return EXIT_SUCCESS;
}