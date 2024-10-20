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
												// v v v maybe have a scene description and a render function here ?
	static Cubemap MakeSceneDepthCubemapFromPoint(const Scene* scene, const vec3 worldPositon);
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



}
}