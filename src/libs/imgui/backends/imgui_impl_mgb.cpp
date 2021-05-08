#include "mgb_helper.hpp"
#include "imgui_impl_mgb.h"
#include "imgui_impl_opengl2.h"
#include "frontend/video/interface.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>


static uint8_t MOUSE_BUTTON_MAP[VideoInterfaceMouseButton_MAX] = {0};
static uint16_t KEY_MAP[VideoInterfaceKey_MAX] = {0};
static uint8_t CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_MAX] = {0};

static int g_w, g_h;
static int g_display_w, g_display_h;
static bool g_MousePressed[3] = { false, false, false };
static bool g_MouseHeld[3] = { false, false, false };
static bool g_window_hidden = false;
static float g_scale_w = 1.f;
static float g_scale_h = 1.f;


namespace im {

ImVec2 ScaleVec2(int x, int y) {
    const ImVec2 v(ScaleW(x), ScaleH(y));
    return v;
}

ImVec4 ScaleVec4(int x, int y, int w, int h) {
    const ImVec4 v(ScaleW(x), ScaleH(y), ScaleW(w), ScaleH(h));
    return v;
}

int Width() {
    return g_w;
}

int Height() {
    return g_h;
}

float ScaleW(int w) {
    return (float)w * ScaleW();
}

float ScaleH(int h) {
    return (float)h * ScaleH();
}

float ScaleW() {
    return g_scale_w;
}

float ScaleH() {
    return g_scale_h;
}

} // namespace im

static void update_scale() {
    const float w = (float)g_w;
    const float h = (float)g_h;

    g_scale_w = w / 160.f;
    g_scale_h = h / 144.f;
}

static void ImGui_ImplMGB_UpdateMousePosAndButtons() {
    ImGuiIO& io = ImGui::GetIO();

    io.MouseDown[0] = g_MousePressed[0] || g_MouseHeld[0];
    io.MouseDown[1] = g_MousePressed[1] || g_MouseHeld[1];
    io.MouseDown[2] = g_MousePressed[2] || g_MouseHeld[2];
    g_MousePressed[0] = g_MousePressed[1] = g_MousePressed[2] = false;
}

static void on_mbutton(const struct VideoInterfaceEventDataMouseButton* e) {
	// If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame
	g_MousePressed[MOUSE_BUTTON_MAP[e->button]] |= e->down;
	g_MouseHeld[MOUSE_BUTTON_MAP[e->button]] = e->down;
}

static void on_mmotion(const struct VideoInterfaceEventDataMouseMotion* e) {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2((float)e->x, (float)e->y);
}

static void on_key(const struct VideoInterfaceEventDataKey* e) {
	ImGuiIO& io = ImGui::GetIO();

	if (KEY_MAP[e->key]) {
		io.KeysDown[KEY_MAP[e->key]] = e->down;
	}

	io.KeyShift = ((e->mod & VideoInterfaceKeyMod_SHIFT) != 0);
	io.KeyCtrl = ((e->mod & VideoInterfaceKeyMod_CTRL) != 0);
	io.KeyAlt = ((e->mod & VideoInterfaceKeyMod_ALT) != 0);
}

static void on_cbutton(const struct VideoInterfaceEventDataControllerButton* e) {
	ImGuiIO& io = ImGui::GetIO();

	if (CONTROLLER_BUTTON_MAP[e->button]) {
		io.NavInputs[CONTROLLER_BUTTON_MAP[e->button]] = e->down;
	}
}

static void on_resize(const struct VideoInterfaceEventDataResize* e) {
	g_w = e->w;
	g_h = e->h;
	g_display_w = e->display_w;
	g_display_h = e->display_h;
    update_scale();
}

static void on_hidden(const struct VideoInterfaceEventDataHidden* e) {
    (void)e;

    g_window_hidden = true;
}

static void on_shown(const struct VideoInterfaceEventDataShown* e) {
    (void)e;

    g_window_hidden = false;
}


bool ImGui_ImplMGB_Event(const union VideoInterfaceEvent* e) {
    switch (e->type) {
		case VideoInterfaceEventType_FILE_DROP:
			break;

    	case VideoInterfaceEventType_MBUTTON:
    		on_mbutton(&e->mbutton);
    		return true;

    	case VideoInterfaceEventType_MMOTION:
    		on_mmotion(&e->mmotion);
    		return true;

    	case VideoInterfaceEventType_KEY:
    		on_key(&e->key);
    		return true;

    	case VideoInterfaceEventType_CBUTTON:
    		on_cbutton(&e->cbutton);
    		return true;

    	case VideoInterfaceEventType_CAXIS:
    		break;

    	case VideoInterfaceEventType_RESIZE:
    		on_resize(&e->resize);
    		break;

        case VideoInterfaceEventType_HIDDEN:
            on_hidden(&e->hidden);
            break;

        case VideoInterfaceEventType_SHOWN:
            on_shown(&e->shown);
            break;

    	case VideoInterfaceEventType_QUIT:
    		break;
    }

    return false;
}

static ImFont* g_font_stash[6];

bool ImGui_ImplMGB_Init(int w, int h, int display_w, int display_h) {
	// Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.BackendPlatformName = "imgui_impl_mgb";

    io.SetClipboardTextFn = NULL;
    io.GetClipboardTextFn = NULL;
    io.ClipboardUserData = NULL;

    // g_font_stash[0] = io.Fonts->AddFontFromFileTTF("res/fonts/Retro Gaming.ttf", 14);
    // g_font_stash[1] = io.Fonts->AddFontFromFileTTF("res/fonts/Retro Gaming.ttf", 18);
    // g_font_stash[2] = io.Fonts->AddFontFromFileTTF("res/fonts/Retro Gaming.ttf", 24);
    g_font_stash[3] = io.Fonts->AddFontFromFileTTF("res/fonts/Retro Gaming.ttf", 28);
    // g_font_stash[4] = io.Fonts->AddFontFromFileTTF("res/fonts/Retro Gaming.ttf", 34);
    // g_font_stash[5] = io.Fonts->AddFontFromFileTTF("res/fonts/Retro Gaming.ttf", 38);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    const ImVec4 black = ImColor(50, 50, 50, 0);
    const ImVec4 col_selected = ImColor(82, 156, 255, 100);

    colors[ImGuiCol_WindowBg]               = ImColor(0x1F, 0x08, 0x1F, 0xFF);
    colors[ImGuiCol_FrameBgHovered]         = col_selected;
    colors[ImGuiCol_FrameBgActive]          = col_selected;
    colors[ImGuiCol_TitleBgActive]          = col_selected;
    colors[ImGuiCol_TitleBgCollapsed]       = col_selected;
    colors[ImGuiCol_ScrollbarGrab]          = col_selected;
    colors[ImGuiCol_ScrollbarGrabHovered]   = col_selected;
    colors[ImGuiCol_SliderGrab]             = col_selected;
    colors[ImGuiCol_SliderGrabActive]       = col_selected;
    colors[ImGuiCol_Button]                 = black;
    colors[ImGuiCol_ButtonHovered]          = col_selected;
    colors[ImGuiCol_ButtonActive]           = col_selected;
    colors[ImGuiCol_HeaderHovered]          = col_selected;
    colors[ImGuiCol_HeaderActive]           = col_selected;
    colors[ImGuiCol_SeparatorHovered]       = col_selected;
    colors[ImGuiCol_SeparatorActive]        = col_selected;
    colors[ImGuiCol_ResizeGripHovered]      = col_selected;
    colors[ImGuiCol_ResizeGripActive]       = col_selected;
    colors[ImGuiCol_TabHovered]             = col_selected;
    colors[ImGuiCol_TabActive]              = col_selected;

	g_w = w;
	g_h = h;
	g_display_w = display_w;
	g_display_h = display_h;

    MOUSE_BUTTON_MAP[VideoInterfaceMouseButton_LEFT]    = 0;
    MOUSE_BUTTON_MAP[VideoInterfaceMouseButton_MIDDLE]  = 1;
    MOUSE_BUTTON_MAP[VideoInterfaceMouseButton_RIGHT]   = 2;

    KEY_MAP[VideoInterfaceKey_UP]           = ImGuiKey_UpArrow;
    KEY_MAP[VideoInterfaceKey_DOWN]         = ImGuiKey_DownArrow;
    KEY_MAP[VideoInterfaceKey_LEFT]         = ImGuiKey_LeftArrow;
    KEY_MAP[VideoInterfaceKey_RIGHT]        = ImGuiKey_RightArrow;
    KEY_MAP[VideoInterfaceKey_TAB]          = ImGuiKey_Tab;
    KEY_MAP[VideoInterfaceKey_DELETE]       = ImGuiKey_Delete;
    KEY_MAP[VideoInterfaceKey_BACKSPACE]    = ImGuiKey_Backspace;
    KEY_MAP[VideoInterfaceKey_SPACE]        = ImGuiKey_Space;
    KEY_MAP[VideoInterfaceKey_ENTER]        = ImGuiKey_Enter;
    KEY_MAP[VideoInterfaceKey_ESCAPE]       = ImGuiKey_Enter;
    KEY_MAP[VideoInterfaceKey_A]            = ImGuiKey_A;
    KEY_MAP[VideoInterfaceKey_C]            = ImGuiKey_C;
    KEY_MAP[VideoInterfaceKey_V]            = ImGuiKey_V;
    KEY_MAP[VideoInterfaceKey_X]            = ImGuiKey_X;
    KEY_MAP[VideoInterfaceKey_Y]            = ImGuiKey_Y;
    KEY_MAP[VideoInterfaceKey_Z]            = ImGuiKey_Z;

    io.KeyMap[ImGuiKey_UpArrow] = KEY_MAP[VideoInterfaceKey_UP];
    io.KeyMap[ImGuiKey_DownArrow] = KEY_MAP[VideoInterfaceKey_DOWN];
    io.KeyMap[ImGuiKey_LeftArrow] = KEY_MAP[VideoInterfaceKey_LEFT];
    io.KeyMap[ImGuiKey_RightArrow] = KEY_MAP[VideoInterfaceKey_RIGHT];
    io.KeyMap[ImGuiKey_Tab] = KEY_MAP[VideoInterfaceKey_TAB];
    io.KeyMap[ImGuiKey_Delete] = KEY_MAP[VideoInterfaceKey_DELETE];
    io.KeyMap[ImGuiKey_Backspace] = KEY_MAP[VideoInterfaceKey_BACKSPACE];
    io.KeyMap[ImGuiKey_Space] = KEY_MAP[VideoInterfaceKey_SPACE];
    io.KeyMap[ImGuiKey_Enter] = KEY_MAP[VideoInterfaceKey_ENTER];
    io.KeyMap[ImGuiKey_Enter] = KEY_MAP[VideoInterfaceKey_ESCAPE];
    io.KeyMap[ImGuiKey_A] = KEY_MAP[VideoInterfaceKey_A];
    io.KeyMap[ImGuiKey_C] = KEY_MAP[VideoInterfaceKey_C];
    io.KeyMap[ImGuiKey_V] = KEY_MAP[VideoInterfaceKey_V];
    io.KeyMap[ImGuiKey_X] = KEY_MAP[VideoInterfaceKey_X];
    io.KeyMap[ImGuiKey_Y] = KEY_MAP[VideoInterfaceKey_Y];
    io.KeyMap[ImGuiKey_Z] = KEY_MAP[VideoInterfaceKey_Z];

    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_A] = ImGuiNavInput_Activate;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_B] = ImGuiNavInput_Cancel;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_X] = ImGuiNavInput_Menu;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_Y] = ImGuiNavInput_Input;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_L1] = ImGuiNavInput_FocusPrev;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_R1] = ImGuiNavInput_FocusNext;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_UP] = ImGuiNavInput_DpadUp;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_DOWN] = ImGuiNavInput_DpadRight;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_LEFT] = ImGuiNavInput_DpadLeft;
    CONTROLLER_BUTTON_MAP[VideoInterfaceControllerButton_RIGHT] = ImGuiNavInput_DpadRight;

	ImGui_ImplOpenGL2_Init();

	return true;
}

void ImGui_ImplMGB_Shutdown() {
	ImGui_ImplOpenGL2_Shutdown();
    ImGui::DestroyContext();
}

static void ImGui_ImplMGB_NewFrame() {
	ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer backend. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup display size (every frame to accommodate for window resizing)
    int w = g_w;
    int h = g_h;
    const int display_w = g_display_w;
    const int display_h = g_display_h;

    if (g_window_hidden) {
        w = h = 0;
    }

    io.DisplaySize = ImVec2((float)w, (float)h);

    if (w > 0 && h > 0) {
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
    }

    // Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
    // static Uint64 frequency = SDL_GetPerformanceFrequency();
    // Uint64 current_time = SDL_GetPerformanceCounter();
    // io.DeltaTime = g_Time > 0 ? (float)((double)(current_time - g_Time) / frequency) : (float)(1.0f / 60.0f);
    // g_Time = current_time;
    io.DeltaTime = 0.016666f;

    ImGui_ImplMGB_UpdateMousePosAndButtons();
}

void ImGui_ImplMGB_RenderBegin() {
	ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplMGB_NewFrame();
    ImGui::NewFrame();
}

void ImGui_ImplMGB_RenderEnd() {
	ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}
