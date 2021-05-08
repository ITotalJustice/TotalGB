#include "menu.hpp"
#include "imgui/backends/mgb_helper.hpp"
#include "frontend/mgb.h"

#include <stdio.h>


namespace menu {

static void button_on_loadrom(struct mgb* mgb) {
    mgb_load_rom_filedialog(mgb);
}

static void button_on_core_settings(struct mgb* mgb) {
    (void)mgb;
}

static void button_on_gui_settings(struct mgb* mgb) {
    (void)mgb;
}

static void button_on_help(struct mgb* mgb) {
    (void)mgb;
}

static void button_on_quit(struct mgb* mgb) {
    mgb->running = false;
}

static const struct Button buttons[] = {
    /*[GuiMainMenuOptions_LOADROM] = */{
        /*.title = */"Loadrom",
        /*.callback = */button_on_loadrom
    },
    /*[GuiMainMenuOptions_CORE_SETTINGS] = */{
        /*.title = */"Core Settings",
        /*.callback = */button_on_core_settings
    },
    /*[GuiMainMenuOptions_GUI_SETTINGS] = */{
        /*.title = */"Gui Settings",
        /*.callback = */button_on_gui_settings
    },
    /*[GuiMainMenuOptions_HELP] = */{
        /*.title = */"Help",
        /*.callback = */button_on_help
    },
    /*[GuiMainMenuOptions_QUIT] = */{
        /*.title = */"Quit",
        /*.callback = */button_on_quit
    },
};

void Main(struct mgb* mgb) {
    const int w = im::Width();
    const int h = im::Height();

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    // ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetCursorPosY()));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(im::ScaleW(w), im::ScaleH(h)));

    ImGui::Begin("Menu", NULL, flags);

    ImGui::Text("TotalGB");

    // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
    // ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 20.0f);
    // ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 30.0f);
    // ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 5));
    // ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    // ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImVec2(0.0f, 0.0f));

    for (size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); ++i) {
        ImGui::SetCursorPosX(0.f);

        if (ImGui::Button(buttons[i].title, ImVec2(w, im::ScaleH(20)))) {
            buttons[i].callback(mgb);
        }
    }

    ImGui::PopStyleVar(1);

    ImGui::PushStyleColor(ImGuiCol_Text, (ImU32)ImColor(0x80, 0x80, 0x80, 120));
    ImGui::Text("Version 0.0.1");
    ImGui::PopStyleColor();

    ImGui::End();
}

} // namespace menu
