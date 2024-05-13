#pragma once

#include "scene/Scene.h"

namespace pye
{
	
class EmptyEditorScene : public pyr::Scene
{
public:
	void update(float delta) override {}
	void render() override {}
};

}
