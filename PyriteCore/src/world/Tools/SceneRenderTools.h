#pragma once

#include "CommonConstantBuffers.h"
#include "utils/math.h"
#include "display/texture.h"

#include "scene/scene.h"
#include "world/camera.h"

#include "display/RenderGraph/RenderGraph.h"
#include "display/RenderGraph/BuiltinPasses/DepthPrePass.h"
#include "display/RenderProfiles.h"

namespace pyr
{


namespace SceneRenderTools
{
	static Texture MakeSceneDepth(const Scene* scene, const Camera& orthographicCamera)
	{
		if (!PYR_ENSURE(std::holds_alternative<OrthographicProjection>(orthographicCamera.getProjection())))
		{
			//PYR_LOG()
			return {};
		}

		// Static render graph that takes in the scene context ?
		struct DepthOnlyDrawer
		{
			RenderGraph graph;
			BuiltinPasses::DepthPrePass depthPass; // < note, works only for static meshes i think
			std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
			DepthOnlyDrawer()
			{
				graph.addPass(&depthPass);
			}
		};


		// To get the output depth : drawer.depthPass.getOutputDepth()
		static DepthOnlyDrawer drawer;

		pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
		pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

		drawer.pcameraBuffer->setData(pyr::CameraBuffer::data_t{
		   .mvp = orthographicCamera.getViewProjectionMatrix(),
		   .pos = orthographicCamera.getPosition()
			});

		drawer.depthPass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", drawer.pcameraBuffer);
		
		drawer.graph.execute(pyr::RenderContext{ scene->SceneActors, "Shadow Compute graph" });

		pyr::RenderProfiles::popDepthProfile();
		pyr::RenderProfiles::popRasterProfile();

		return drawer.depthPass.getOutputDepth();

	}

	static Cubemap MakeSceneDepthCubemapFromPoint(const Scene* scene, const vec3& worldPositon, unsigned int resolution)
	{
		static constexpr std::array<vec3, 6> directions{ 
			{
				{-1,0,0},
				{1,0,0},
				{0,1,0},
				{0,-1,0},
				{0,0,-1},
				{0,0, 1},
		} };
		static pyr::Camera renderCamera{};
		renderCamera.setProjection(pyr::PerspectiveProjection{ .fovy = 3.141592f / 2.f, .aspect = 1.F });
		renderCamera.setPosition(worldPositon);

		// Todo : find a way to cache ?
		struct DepthOnlyCubeDrawer
		{
			std::array<pyr::FrameBuffer, 6> framebuffers;

			RenderGraph graph;
			BuiltinPasses::DepthPrePass depthPass; // < note, works only for static meshes i think
			std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
			DepthOnlyCubeDrawer(unsigned int resolution)
			{
				graph.addPass(&depthPass);
				for (int faceID = 0; faceID < 6; faceID++)
				{
					framebuffers[faceID] = pyr::FrameBuffer{ resolution, resolution, pyr::FrameBuffer::DEPTH_STENCIL };
				}
			}
		};
		static DepthOnlyCubeDrawer cubeDrawer{resolution};


		std::array<pyr::Texture, 6> textures;

		static  pyr::GraphicalResourceRegistry m_registry;
		static pyr::Effect* m_depthOnlyEffect = m_registry.loadEffect(
			L"res/shaders/depthOnly.fx",
			InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>()
		);

		static std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();


		auto renderFn = [scene]() {
			for (const StaticMesh* smesh : scene->SceneActors.meshes)
			{

				smesh->bindModel();

				pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->GetTransform().getWorldMatrix() });
				m_depthOnlyEffect->bindConstantBuffer("ActorBuffer", pActorBuffer);
				m_depthOnlyEffect->bind();

				std::span<const SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
				for (auto& submesh : submeshes)
				{
					Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
				}
			}
		};

		pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::CULLBACK_RASTERIZER);
		pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

		// -- Draw the 6 faces
		for (int faceID = 0; faceID < 6; faceID++)
		{
			renderCamera.lookAt(renderCamera.getPosition() + directions[faceID]);
			cubeDrawer.pcameraBuffer->setData(pyr::CameraBuffer::data_t{
				.mvp = renderCamera.getViewProjectionMatrix(),
				.pos = renderCamera.getPosition()
				});

			cubeDrawer.framebuffers[faceID].clearTargets();
			cubeDrawer.framebuffers[faceID].bind();
			m_depthOnlyEffect->bindConstantBuffer("CameraBuffer", cubeDrawer.pcameraBuffer);
			renderFn();
			m_depthOnlyEffect->unbindResources();
			cubeDrawer.framebuffers[faceID].unbind();
			textures[faceID] = cubeDrawer.framebuffers[faceID].getTargetAsTexture(pyr::FrameBuffer::DEPTH_STENCIL);
		}

		pyr::RenderProfiles::popDepthProfile();
		pyr::RenderProfiles::popRasterProfile();

		pyr::Cubemap finalCubemap = pyr::CubemapBuilder::MakeCubemapFromTextures(textures, true);

		return finalCubemap;
	}


}
}