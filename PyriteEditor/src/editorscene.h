#pragma once

#include "scene/Scene.h"

namespace pye
{
	
class EmptyEditorScene : public pyr::Scene
{
public:
	void update(double delta) override {}
	void render() override {}
};

}
