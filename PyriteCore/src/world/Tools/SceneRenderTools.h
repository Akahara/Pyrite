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


class SceneRenderTools
{
private:
	
	SceneRenderTools() = delete;

	struct DepthOnlyDrawer
	{
		RenderGraph graph;
		BuiltinPasses::DepthPrePass depthPass; // < note, works only for static meshes i think
		std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
		DepthOnlyDrawer() : depthPass(512, 512)
		{
			graph.addPass(&depthPass);
		}
	};



public:

	static Texture MakeSceneDepth(const RegisteredRenderableActorCollection& sceneDescription, const Camera& camera)
	{
		auto test = camera.getViewProjectionMatrix();
		static DepthOnlyDrawer drawer;

		pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
		pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

		drawer.pcameraBuffer->setData(pyr::CameraBuffer::data_t{
		   .mvp = camera.getViewProjectionMatrix(),
		   .pos = camera.getPosition()
			});

		drawer.depthPass.getDepthPassEffect()->bindConstantBuffer("CameraBuffer", drawer.pcameraBuffer);
		
		drawer.graph.execute(pyr::RenderContext{ sceneDescription, "Shadow Compute graph" });

		pyr::RenderProfiles::popDepthProfile();
		pyr::RenderProfiles::popRasterProfile();

		return drawer.depthPass.getOutputDepth();

	}

	static Cubemap MakeSceneDepthCubemapFromPoint(const RegisteredRenderableActorCollection& sceneDescription, const vec3& worldPositon, unsigned int resolution)
	{
		static constexpr std::array<vec3, 6> directions{
			{
				{1,0,0},
				{-1,0,0},
				{0,1,0},
				{0,-1,0},
				{0,0,-1},
				{0,0, 1},
		} };

		static std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
		static std::shared_ptr<ActorBuffer> pActorBuffer = std::make_shared<ActorBuffer>();
		static pyr::GraphicalResourceRegistry m_registry;
		static pyr::Effect* m_depthOnlyEffect = m_registry.loadEffect(
			L"res/shaders/depthOnly.fx",
			InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>(),
			{ pyr::Effect::define_t{.name = "LINEARIZE_DEPTH", .value = "1" } }
		);
		static pyr::Camera renderCamera{};
		renderCamera.setProjection(pyr::PerspectiveProjection{ .fovy = XM_PIDIV2, .aspect = 1.F,.zNear = 0.01f,  .zFar = 1000.F });
		renderCamera.setPosition(worldPositon);

		auto renderFn = [&sceneDescription, &worldPositon]() {
			m_depthOnlyEffect->setUniform<vec3>("u_sourcePosition", worldPositon);
			m_depthOnlyEffect->setUniform<float>("u_CameraFarPlane", std::get<PerspectiveProjection>(renderCamera.getProjection()).zFar);
			for (const StaticMesh* smesh : sceneDescription.meshes)
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

		static CubemapFramebuffer cubemapFBO{ resolution, FrameBuffer::Target::COLOR_0 };

		pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
		pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

		// -- Draw the 6 faces
		for (int faceID = 0; faceID < 6; faceID++)
		{
			renderCamera.lookAt(renderCamera.getPosition() + directions[faceID]);
			if (faceID == 2) renderCamera.rotate(0.f, 3.14159f, 0.f); // why ? it works
			if (faceID == 3) renderCamera.rotate(0.f, 3.14159f, 0.f); // why ? it works
			if (faceID == 4) renderCamera.rotate(0.f, 3.14159f, 0.f); // why ? it works
			if (faceID == 5) renderCamera.rotate(0.f, 3.14159f, 0.f); // why ? it works
			pcameraBuffer->setData(pyr::CameraBuffer::data_t{
				.mvp = renderCamera.getViewProjectionMatrix(),
				.pos = renderCamera.getPosition()
				});

			auto currentFace = static_cast<pyr::CubemapFramebuffer::Face>(faceID);
			cubemapFBO.clearFaceTargets(currentFace);
			cubemapFBO.bindFace(currentFace);
			m_depthOnlyEffect->bindConstantBuffer("CameraBuffer", pcameraBuffer);
			renderFn();
			m_depthOnlyEffect->unbindResources();
		}
		pyr::RenderProfiles::popDepthProfile();
		pyr::RenderProfiles::popRasterProfile();

		// -- Rebind to context the previous framebuffer, this is a flaw of the cubemap FBO implementation with the stack...
		FrameBuffer::getActiveFrameBuffer().bindToD3DContext();

		return cubemapFBO.getTargetAsCubemap(FrameBuffer::COLOR_0);
	}
}; 
}