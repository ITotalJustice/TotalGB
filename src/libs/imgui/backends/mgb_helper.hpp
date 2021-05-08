#pragma once

#include "../imgui.h"

namespace im {

ImVec2 ScaleVec2(int x, int y);
ImVec4 ScaleVec4(int x, int y, int w, int h);

int Width();
int Height();

float ScaleW(int w);
float ScaleH(int h);

float ScaleW();
float ScaleH();

} // namespace im
