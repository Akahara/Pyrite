#pragma once

#include "utils/math.h"
#include "world/Transform.h"
#include "world/Actor.h"

namespace pyr
{
	class Billboard : public Actor
	{
		Transform transform;
		bool bAutoFace = true;
	};
}