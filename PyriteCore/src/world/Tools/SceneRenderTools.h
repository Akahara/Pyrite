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

	struct DepthDrawer
	{
		pyr::Effect* depthOnlyEffect = nullptr;
		pyr::Camera camera;
		pyr::GraphicalResourceRegistry registry;
		struct Buffers
		{
			std::shared_ptr<pyr::CameraBuffer>  pcameraBuffer = std::make_shared<pyr::CameraBuffer>();
			std::shared_ptr<ActorBuffer>		pActorBuffer = std::make_shared<ActorBuffer>();
		} buffers;

		enum RenderType { Texture2D, TextureCube };
		DepthDrawer(RenderType type)
		{
			std::vector<pyr::Effect::define_t> defines{};
		
			if (type == TextureCube) defines.emplace_back("LINEARIZE_DEPTH", "1");

			depthOnlyEffect = registry.loadEffect(
				L"res/shaders/depthOnly.fx",
				InputLayout::MakeLayoutFromVertex<pyr::RawMeshData::mesh_vertex_t>(),
				defines);

		}

		void Render(const RegisteredRenderableActorCollection& sceneDescription)
		{
			for (const StaticMesh* smesh : sceneDescription.meshes)
			{
				smesh->bindModel();
				buffers.pActorBuffer->setData(ActorBuffer::data_t{ .modelMatrix = smesh->GetTransform().getWorldMatrix() });
				depthOnlyEffect->bindConstantBuffer("ActorBuffer", buffers.pActorBuffer);
				depthOnlyEffect->bind();
				std::span<const SubMesh> submeshes = smesh->getModel()->getRawMeshData()->getSubmeshes();
				for (auto& submesh : submeshes)
				{
					Engine::d3dcontext().DrawIndexed(static_cast<UINT>(submesh.getIndexCount()), submesh.startIndex, 0);
				}
			}
		}

	};

public:

	static Texture MakeSceneDepth(const RegisteredRenderableActorCollection& sceneDescription, const Camera& camera, pyr::FrameBuffer& outFramebuffer)
	{
		static DepthDrawer depthDrawer2D{ DepthDrawer::Texture2D };

		outFramebuffer.bind();
		pyr::Engine::d3dcontext().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		pyr::RenderProfiles::pushRasterProfile(pyr::RasterizerProfile::NOCULL_RASTERIZER);
		pyr::RenderProfiles::pushDepthProfile(pyr::DepthProfile::TESTWRITE_DEPTH);

		depthDrawer2D.buffers.pcameraBuffer->setData(pyr::CameraBuffer::data_t{
				.mvp = camera.getViewProjectionMatrix(),
				.pos = camera.getPosition()
		});

		depthDrawer2D.depthOnlyEffect->bindConstantBuffer("CameraBuffer", depthDrawer2D.buffers.pcameraBuffer);
		depthDrawer2D.Render(sceneDescription);
		depthDrawer2D.depthOnlyEffect->unbindResources();


		pyr::RenderProfiles::popDepthProfile();
		pyr::RenderProfiles::popRasterProfile();
		outFramebuffer.unbind();

		return outFramebuffer.getTargetAsTexture(pyr::FrameBuffer::DEPTH_STENCIL);

	}

	static Cubemap MakeSceneDepthCubemapFromPoint(const RegisteredRenderableActorCollection& sceneDescription, const vec3& worldPositon, pyr::CubemapFramebuffer& outFramebuffer)
	{
		static DepthDrawer depthDrawer3D{ DepthDrawer::TextureCube };

		static constexpr std::array<vec3, 6> directions{
			{
				{1,0,0},
				{-1,0,0},
				{0,1,0},
				{0,-1,0},
				{0,0,-1},
				{0,0, 1},
		} };

		static pyr::Camera renderCamera{};
		renderCamera.setProjection(pyr::PerspectiveProjection{ .fovy = XM_PIDIV2, .aspect = 1.F,.zNear = 0.01f,  .zFar = 1000.F });
		renderCamera.setPosition(worldPositon);

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

			depthDrawer3D.buffers.pcameraBuffer->setData(pyr::CameraBuffer::data_t{
					.mvp = renderCamera.getViewProjectionMatrix(),
					.pos = renderCamera.getPosition()
				});

			auto currentFace = static_cast<pyr::CubemapFramebuffer::Face>(faceID);
			outFramebuffer.clearFaceTargets(currentFace);
			outFramebuffer.bindFace(currentFace);
			depthDrawer3D.depthOnlyEffect->bindConstantBuffer("CameraBuffer", depthDrawer3D.buffers.pcameraBuffer);
			depthDrawer3D.depthOnlyEffect->setUniform("u_sourcePosition", worldPositon);
			depthDrawer3D.Render(sceneDescription);
			depthDrawer3D.depthOnlyEffect->unbindResources();
		}
		pyr::RenderProfiles::popDepthProfile();
		pyr::RenderProfiles::popRasterProfile();

		// -- Rebind to context the previous framebuffer, this is a flaw of the cubemap FBO implementation with the stack...
		FrameBuffer::getActiveFrameBuffer().bindToD3DContext();

		return outFramebuffer.getTargetAsCubemap(FrameBuffer::COLOR_0);
	}
}; 
}