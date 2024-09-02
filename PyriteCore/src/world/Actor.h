#pragma once
#include <cstdint>

namespace pyr
{

	class Actor
	{
	public:
		using id_t = uint32_t;
		id_t GetActorID() const { return m_actorId; }

	private:
		id_t m_actorId = NextID++;
		static inline id_t NextID = 0;
	};



}