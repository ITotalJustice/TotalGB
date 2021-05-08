// my custom wrapper, simpiler to the sdl2 impl
#pragma once


#ifdef __cplusplus
extern "C" {
#endif

union VideoInterfaceEvent;

bool ImGui_ImplMGB_Init(int w, int h, int display_w, int display_h);
void ImGui_ImplMGB_Shutdown();
void ImGui_ImplMGB_RenderBegin();
void ImGui_ImplMGB_RenderEnd();
void ImGui_ImplMGB_RenderDemo();
bool ImGui_ImplMGB_Event(const union VideoInterfaceEvent* e);

#ifdef __cplusplus
}
#endif
