#pragma once

// https://users.soe.ucsc.edu/~pang/160/s13/proposal/mijallen/proposal/media/p203-dachsbacher.pdf

#include "display/texture.h"
#include "display/FrameBuffer.h"

#include "world/Lights/Light.h"

#include <variant>

namespace pyr
{
	/// A RSM is essentially a glorified shadow map that stores information like a G-Buffer. 
	/// The only tricky target is the Flux, which is the kind of the amount of light with albedo that a pixel receives. 

	class ReflectiveShadowMap
	{

	private:

		enum Targets
		{
			Depth		= pyr::FrameBuffer::DEPTH_STENCIL,
			WorldPos	= pyr::FrameBuffer::COLOR_0,
			Normal		= pyr::FrameBuffer::COLOR_1,
			Flux		= pyr::FrameBuffer::COLOR_2,

		};

		// No support for point light currently as cubemap framebuffers dont have multiple targets
		pyr::BaseLight* m_sourceLight;
		pyr::FrameBuffer m_framebuffer;

	public:

		ReflectiveShadowMap(unsigned int width, unsigned int height, pyr::BaseLight* light)
			: m_framebuffer{width, height, Depth | WorldPos | Normal | Flux }
			, m_sourceLight(light)
		{
			PYR_ASSERT(dynamic_cast<pyr::PointLight*>(light) == nullptr, "Cubemaps multi-target framebuffers are not implemented yet.");
		}

		pyr::FrameBuffer& GetFramebuffer() { return m_framebuffer; }

	};

}
