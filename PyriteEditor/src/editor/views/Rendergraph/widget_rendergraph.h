#pragma once

#include "imgui.h"

#include "imguizmo/ImGuizmo.h"
#include "imNodesFlow/imnodes.h"

#include "display/texture.h"
#include "display/FrameBuffer.h"	
#include "display/RenderGraph/BuiltinPasses/ForwardPass.h"	
#include "display/RenderGraph/RenderPass.h"	
#include "display/RenderGraph/RenderGraph.h"	

#include "world/Mesh/StaticMesh.h"
#include "world/Mesh/Model.h"
#include "world/Mesh/MeshImporter.h"
#include "world/Tools/CommonConstantBuffers.h"

#include "editor/bridges/Lights/pf_Light.h"
#include "editor/views/widget.h"
#include "editor/Editor.h"
#include "editor/EditorEvents.h"

#include <utils/math.h>
#include <ranges>



namespace pye
{
	namespace widgets
	{

		struct RenderGraphWidget : public Widget
		{

		private:

			static constexpr unsigned int INPUT_PIN_COLOR = IM_COL32(81, 204, 148, 255);
			static constexpr unsigned int OUTPUT_PIN_COLOR = IM_COL32(148, 81, 72, 255);
			int id = 0;
			std::unordered_map<std::string, int> resourceToInputMap;
			std::unordered_map<int, std::vector<int>> inputToOutputsId;
			std::unordered_map<int, pyr::RenderGraphResourceManager::PassResources> nodeIDToResources;
			std::unordered_map<pyr::RenderPass*, bool> bIsDebugWindowOpened;

		public:

			RenderGraphWidget()
			{

			}

			virtual void display() override
			{
				ImGui::Begin("Render graph editor");

				ImNodes::BeginNodeEditor();

				BuildSceneRenderGraph();
		
				ImNodes::MiniMap(); 
				ImNodes::EndNodeEditor();

				int out = 0;
				for (auto [name, output_id] : resourceToInputMap)
				{
					if (ImNodes::IsPinHovered(&out))
					{
						if (out == output_id )
						{
							ImGui::BeginTooltip();
							auto& rdgResources = pyr::SceneManager::getActiveScene()->SceneRenderGraph.getResourcesManager();
							for (const auto& res : rdgResources.GetAllResources() | std::ranges::views::values)
							{
								if (res.producedResources.contains(name))
								{
									ImGui::Image((void*)res.producedResources.at(name).res.getRawTexture(), ImVec2{ 128,128 });
									break;
								}
							}
							ImGui::EndTooltip();
						}
					}
				}
				ImGui::End();
			};

			void BuildNode(pyr::RenderPass* pass, const pyr::RenderGraphResourceManager::PassResources& resources)
			{
				int node_id = id++;
				ImNodes::BeginNode(node_id);
				nodeIDToResources[node_id] = resources;


				ImNodes::BeginNodeTitleBar();
				ImGui::TextUnformatted(pass->displayName.c_str());
				ImNodes::EndNodeTitleBar();

				bool bCanBeDebugged = pass->HasDebugWindow();
				if (bCanBeDebugged == false)
				{
					ImGui::BeginDisabled();
				}
				
				ImGui::Checkbox(bCanBeDebugged ? "Debug this !" : "No debug window found.", &bIsDebugWindowOpened[pass]);

				if (bIsDebugWindowOpened[pass])
				{
					pass->OpenDebugWindow();
				}
				if (pass->HasDebugWindow() == false)
				{
					ImGui::EndDisabled();
				}

				ImGui::Dummy({ 0,0 });

				ImNodes::PushColorStyle(ImNodesCol_Pin, INPUT_PIN_COLOR);
				for (const auto& [name, ref] : resources.incomingResources)
				{
					int incomingResourceId = resourceToInputMap[name];
					inputToOutputsId[incomingResourceId].push_back(id);
					ImNodes::BeginInputAttribute(id++);
					ImGui::Text(name.c_str());
					ImNodes::EndOutputAttribute();
				}

				ImNodes::PopColorStyle();
				ImNodes::PushColorStyle(ImNodesCol_Pin, OUTPUT_PIN_COLOR);

				for (const auto& [name, ref] : resources.producedResources)
				{
					resourceToInputMap[name] = id;
					ImNodes::BeginOutputAttribute(id);
					ImGui::Text(name.c_str());
					ImNodes::EndOutputAttribute();
					id++;
				}
				ImNodes::PopColorStyle();
				ImNodes::EndNode();
			}

			void BuildSceneRenderGraph()
			{
				resourceToInputMap.clear();
				inputToOutputsId.clear();
				nodeIDToResources.clear();
				id = 0;
				auto& rdgResources = pyr::SceneManager::getActiveScene()->SceneRenderGraph.getResourcesManager();

				// -- Build the nodes
				for (auto& [pass, resources] : rdgResources.GetAllResources())
				{
					BuildNode(pass, resources);
				}

				// -- Link the pins
				for (auto& [in, outs] : inputToOutputsId)
				{
					for (int out : outs)
					{
						ImNodes::Link(id++, in, out);
					}
				}

				// -- Place the nodes
				static bool once = true;
				if (!once) return;

				std::unordered_map<int, std::pair<int,int>> nodeIdToDepth;

				int depth = 0;
				int height = 0;
				std::set<std::string> producedFounds{};
				while (nodeIdToDepth.size() != nodeIDToResources.size() && depth < 10)
				{
					height = 0;
					// -- First check 
					for (auto& [node_id, resources] : nodeIDToResources)
					{
						if (nodeIdToDepth.contains(node_id)) continue;
						auto filtered = resources.incomingResources 
										| std::ranges::views::keys
										| std::ranges::views::filter([&](const std::string& name) { return !producedFounds.contains(name); });

						if (filtered.empty())
						{
							nodeIdToDepth[node_id] = { depth, height++ };
						}
					}
					// -- Then add produced outputs of everyone
					for (auto& [node_id, resources] : nodeIDToResources)
					{
						// if the node has been solved the say that its produced resources are recorded
						if (nodeIdToDepth.contains(node_id) && nodeIdToDepth.at(node_id).first == depth)
							std::ranges::for_each(resources.producedResources | std::ranges::views::keys, [&](const std::string& name) { producedFounds.insert(name); });
					}

					depth++;
				}

				for (auto [node_id, pos] : nodeIdToDepth)
				{
					ImNodes::SetNodeGridSpacePos(node_id, { pos.first * 200.F, -pos.second * 200.F });
				}
				once = false;

			}

		private:

		
		};
	}
}