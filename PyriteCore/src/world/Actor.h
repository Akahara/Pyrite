#pragma once

#include <cstdint>
#include "world/Transform.h"

namespace pyr
{

	class Actor
	{
	public:
		using id_t = uint32_t;
		id_t GetActorID()				const	{ return m_actorId; }
		const Transform& GetTransform()	const	{ return m_actorTransform; } 
		Transform& GetTransform()				{ return m_actorTransform; }

	public:
		virtual ~Actor() = default;

	private:
		static inline id_t NextID = 1;
		id_t m_actorId = NextID++;

		Transform m_actorTransform;
	};



}