#pragma once



#include "imgui.h"
#include "imguizmo/ImGuizmo.h"

#include "display/texture.h"
#include "display/FrameBuffer.h"	
#include "display/RenderGraph/BuiltinPasses/ForwardPass.h"	

#include "world/Mesh/StaticMesh.h"
#include "world/Mesh/Model.h"
#include "world/Mesh/MeshImporter.h"
#include "world/Tools/CommonConstantBuffers.h"

#include "editor/bridges/Lights/pf_Light.h"
#include "editor/views/widget.h"
#include "editor/Editor.h"
#include "editor/EditorEvents.h"

#include <utils/math.h>
#include <utils/Debug.h>

#include <variant>
#include <utility>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>

namespace pye
{
	namespace widgets
	{

		// The editor ui is a special kind of widget
		// It is meant to be a singleton, and everything will be public to bind onto like menu bars and stuff ?
		class EditorUI : public Widget
		{

		public:

			struct ImGuiItem
			{
				std::string item_name;
				std::string shortcut;
				pye::Widget* UnderlyingWidget = nullptr;

				ImGuiItem* parent = nullptr;
				std::vector<ImGuiItem*> children;
			};

			using imgui_menu_t = std::vector<ImGuiItem>;

			std::unordered_map<std::string, imgui_menu_t> menus;

			static EditorUI& Get()
			{
				static EditorUI ui;
				return ui;
			}

private:
			EditorUI() 
			{
				menus["File"];
				menus["Tools"];
				menus["Settings"];
				menus["About"];
				bDisplayWidget = true;
			}
public:

			imgui_menu_t& GetMenu(const char* name)
			{
				PYR_ENSURE(menus.contains(name), "The requested menu has not been found ! Will create a new one... oops");
				return menus[name];
			}

			virtual void display() override
			{
				std::function<void(ImGuiItem& item)> display_item;
				display_item = [&display_item](ImGuiItem& item)
				{
					if (item.children.empty())
					{
						static bool a, b, c;
						//if (!PYR_ENSURE(item.UnderlyingWidget), "The action has no associated widget !") return;
						ImGui::MenuItem(item.item_name.c_str(), item.shortcut.c_str(), &item.UnderlyingWidget->bDisplayWidget, true);
						
					}
					else {
						for (ImGuiItem* child : item.children)
						{
							ImGui::BeginMenu(item.item_name.c_str());
							display_item(*child);
							ImGui::EndMenu();
						}
					}
				};

				if (ImGui::BeginMainMenuBar())
				{

					for (auto& [name, items] : menus)
					{
						if (ImGui::BeginMenu(name.c_str()))
						{
							for (ImGuiItem& item : items)
							{
								display_item(item);
							}
							ImGui::EndMenu();
						}
					}

					ImGui::EndMainMenuBar();
				}

			};

		private:


		};

		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
		
		struct WidgetsContainer
		{
			std::vector<Widget*> widgets;
			void Render()
			{
				EditorUI& ui = pye::widgets::EditorUI::Get();

				ui.display();

				for (Widget* widget : widgets)
					if (widget->bDisplayWidget) widget->display();
			}
		};

	}

}