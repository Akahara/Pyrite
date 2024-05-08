#pragma once

#include "scene/scene.h"

namespace pye
{
	
class EmptyEditorScene : public pyr::Scene
{
public:
	void update(double delta) override {}
	void render() override {}
};

}
