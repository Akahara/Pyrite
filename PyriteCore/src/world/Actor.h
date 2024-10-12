#pragma once

#include <cstdint>
#include "world/Transform.h"

namespace pyr
{

	class Actor
	{
	public:
		using id_t = uint32_t;
		id_t GetActorID()			const { return m_actorId; }
		Transform GetTransform()	const { return m_actorTransform; } 
		Transform& GetTransform()		  { return m_actorTransform; }

	private:
		id_t m_actorId = NextID++;
		static inline id_t NextID = 1;
		Transform m_actorTransform;
	};



}