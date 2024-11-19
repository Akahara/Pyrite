#pragma once

#include "RenderableActorCollection.h"
#include "display/RenderGraph/RenderGraph.h"
#include <vector>

namespace pyr
{

class Scene
{
public:
  virtual ~Scene() = default;
  
  virtual void update(float delta) = 0;
  virtual void render() = 0;

  RegisteredRenderableActorCollection SceneActors;
  class pyr::RenderGraph SceneRenderGraph;
};

class EmptyScene : public Scene
{
public:
  void update(float delta) override {}
  void render() override {}
};

}