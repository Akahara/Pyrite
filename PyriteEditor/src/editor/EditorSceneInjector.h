#pragma once

#include "scene/Scene.h"
#include "views/Materials/widget_material.h"
#include "views/Lights/widget_lights.h"
#include "views/EditorUI.h"

#include "../editorPasses/PickerPass.h"
#include "../editorPasses/WorldHUDPass.h"

#include "views/Rendergraph/widget_rendergraph.h"

namespace pye
{
	class EditorSceneInjector
	{
	public:

	
		static void OnRender()
		{
			auto& ref = Get();
			ref.container.Render();
		}

		static void InjectEditorToolsToScene(pyr::Scene& scene)
		{
			auto& ref = Get();
			scene.SceneRenderGraph.addPass(&ref.m_editorHUD);
			scene.SceneRenderGraph.addPass(&ref.m_picker);
			scene.SceneRenderGraph.getResourcesManager().linkResource("depthBuffer", &ref.m_picker);
		}

	private:

		// -- Tools
		pye::widgets::LightCollectionWidget lightWidget;
		pye::widgets::MaterialWidget materialWidget;
		pye::widgets::RenderGraphWidget RenderGraphWidget;

		pye::widgets::WidgetsContainer container;

		// -- Passes
		pye::EditorPasses::WorldHUDPass     m_editorHUD;
		pye::EditorPasses::PickerPass       m_picker;

		EditorSceneInjector()
		{
			auto& tools_menu = pye::widgets::EditorUI::Get().GetMenu("Tools");

			pye::widgets::EditorUI::ImGuiItem lightWidgetItem;
			lightWidgetItem.item_name = "Light outliner";
			lightWidgetItem.UnderlyingWidget = &lightWidget;

			pye::widgets::EditorUI::ImGuiItem materialWidgetItem;
			materialWidgetItem.item_name = "Material displayer";
			materialWidgetItem.UnderlyingWidget = &materialWidget;


			pye::widgets::EditorUI::ImGuiItem rdgWidgetItem;
			rdgWidgetItem.item_name = "Render graph displayer";
			rdgWidgetItem.UnderlyingWidget = &RenderGraphWidget;

			tools_menu.push_back(std::move(lightWidgetItem));
			tools_menu.push_back(std::move(materialWidgetItem));
			tools_menu.push_back(std::move(rdgWidgetItem));

			container.widgets.push_back(&lightWidget);
			container.widgets.push_back(&materialWidget);
			container.widgets.push_back(&RenderGraphWidget);
		}

		static EditorSceneInjector& Get()
		{
			static EditorSceneInjector instance;
			return instance;
		}

	};



}