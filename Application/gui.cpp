#define _CRT_SECURE_NO_WARNINGS

#include "gui.h"

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_dx9.h"
#include "../ImGui/imgui_impl_win32.h"

#include <string>
#include <fstream>

#include "../ImGui/imfilebrowser.h"
#include "../ImGui/TextEditor.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter);

long __stdcall WindowProcess(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter) {

	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message) {
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED) {
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = HIWORD(longParameter);
			gui::ResetDevice();
		}
	} return 0;
	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU)
			return 0;
	} break;
	case WM_DESTROY: {
		PostQuitMessage(0);
	} return 0;
	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter);
	} return 0;
	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON) {
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{};

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 && gui::position.x <= gui::WIDTH && gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(gui::window, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER);
		}
	} return 0;

	}

	return DefWindowProcA(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(const char* windowName, const char* className) noexcept {
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(className, windowName, WS_POPUP, 100, 100, WIDTH, HEIGHT, 0, 0, windowClass.hInstance, 0);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept {
	DestroyWindow(window);
	UnregisterClassA(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept {
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParameters, &device) < 0) return false;

	return true;
}

void gui::ResetDevice() noexcept {
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept {
	if (device) {
		device->Release();
		device = nullptr;
	}

	if (d3d) {
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::CreateImGui() noexcept {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;
	io.Fonts->AddFontFromFileTTF("C:\\windows\\fonts\\SegoeUI.ttf", 22);

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept {
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void UIColors() {
	ImGuiStyle& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_WindowBg] = ImColor(35, 35, 35);

	style.Colors[ImGuiCol_TitleBg] = ImColor(122, 0, 0);
	style.Colors[ImGuiCol_TitleBgActive] = ImColor(122, 0, 0);

	style.Colors[ImGuiCol_FrameBg] = ImColor(40, 40, 40, 0);
	style.Colors[ImGuiCol_FrameBgActive] = ImColor(40, 40, 40, 0);

	style.Colors[ImGuiCol_Button] = ImColor(122, 0, 0);
	style.Colors[ImGuiCol_ButtonActive] = ImColor(180, 0, 0);
	style.Colors[ImGuiCol_ButtonHovered] = ImColor(150, 0, 0);

	style.Colors[ImGuiCol_ScrollbarBg] = ImColor(0, 0, 0, 0);

	style.Colors[ImGuiCol_MenuBarBg] = ImColor(122, 0, 0);
	
}

void gui::BeginRender() noexcept {
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	UIColors();
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept {
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

static char BuildName[UCHAR_MAX] = "PShellIDE_Build.exe";
static char ConsoleOutput[UCHAR_MAX] = "";
static char SelectedFile[UCHAR_MAX] = "";

ImGui::FileBrowser ImGuiFileDialog;
TextEditor ScriptEditor;

void gui::console::WriteLine(const char* text)
{
	strcpy(ConsoleOutput, (std::string(text) + "\n" + std::string(ConsoleOutput)).c_str());
}

void gui::console::Clear() {
	strcpy(ConsoleOutput, "");
}

void gui::Render() noexcept {
	
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("PShell IDE")) {
			if (ImGui::MenuItem("Open File")) {
				ImGuiFileDialog.Open();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save        (CTRL+S)")) {

			}
			if (ImGui::MenuItem("Save All   (CTRL+SHIFT+S)")) {

			}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit          (ALT+F4)")) {
				gui::exit = false;
			}
			ImGui::EndMenu();
		}

		ImGui::Text("|  v1.0.0.0  |");

		if (ImGui::BeginMenu("test")) {
			if (ImGui::MenuItem("testtest")) {

			}
			ImGui::EndMenu();
		}

		ImGui::Text("|");

		if (ImGui::BeginMenu("Build")) {
			if (ImGui::MenuItem("Build (x86)")) {

			}
			if (ImGui::MenuItem("Build (x64)")) {

			}
			if (ImGui::BeginMenu("Build Settings")) {
				ImGui::InputText("Build Name", BuildName, IM_ARRAYSIZE(BuildName));
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGui::SetNextWindowPos({ 0, 28 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT - 28 });
	ImGui::Begin("PShell IDE", &exit, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

	ImGuiFileDialog.SetTitle("PShell IDE | Select Script File");
	ImGuiFileDialog.SetTypeFilters({".ps1", ".psd1", ".psm1"});

	auto lang = TextEditor::LanguageDefinition::PowerShell();
	ScriptEditor.SetLanguageDefinition(lang);

	{ // SCRIPT EDITOR
		ImGui::SetNextWindowPos({ 8, 36 });
		ImGui::BeginChild("Script Editor", ImVec2(WIDTH - 15, HEIGHT - 200), true);
		ScriptEditor.Render("Script Editor");
		ImGui::EndChild();
	}

	{ // CONSOLE
		ImGui::SetNextWindowPos({ 8, HEIGHT - 158 });
		ImGui::BeginChild("Console", ImVec2(WIDTH - 15, 150), true);
		ImGui::BeginDisabled();
		ImGui::InputTextMultiline("", ConsoleOutput, IM_ARRAYSIZE(ConsoleOutput), ImVec2(WIDTH - 22, 134));
		ImGui::EndDisabled();
		ImGui::EndChild();
	}

	ImGui::End();

	ImGuiFileDialog.Display();

	if (ImGuiFileDialog.HasSelected()) {
		strcpy(SelectedFile, ImGuiFileDialog.GetSelected().string().c_str());

		std::ifstream ifs(SelectedFile);
		std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
		ScriptEditor.SetText(content);

		ImGuiFileDialog.ClearSelected();
	}
}