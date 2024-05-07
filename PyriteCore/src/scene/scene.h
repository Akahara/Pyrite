#pragma once

namespace pyr
{

class Scene
{
public:
  virtual ~Scene() = default;
  
  virtual void update(double delta) = 0;
  virtual void render() = 0;
};

class EmptyScene : public Scene
{
public:
  void update(double delta) override {}
  void render() override {}
};

}