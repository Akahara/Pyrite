#pragma once

namespace pyr
{

class Scene
{
public:
  virtual ~Scene() = default;
  
  virtual void update(float delta) = 0;
  virtual void render() = 0;
};

class EmptyScene : public Scene
{
public:
  void update(float delta) override {}
  void render() override {}
};

}