#include "ImGuiExt.h"

namespace ImGui
{
	
bool ColoredButton(const char* label, const ImVec4& color)
{
	float static constexpr HoverF = 1.1f, ActiveF = 1.2f;
	PushStyleColor(ImGuiCol_Button, color);
	PushStyleColor(ImGuiCol_ButtonActive, { color.x*ActiveF, color.y*ActiveF, color.z*ActiveF, color.w });
	PushStyleColor(ImGuiCol_ButtonHovered, { color.x*HoverF, color.y*HoverF, color.z*HoverF, color.w });
	bool bPressed = Button(label);
	PopStyleColor(3);
	return bPressed;
}

}
