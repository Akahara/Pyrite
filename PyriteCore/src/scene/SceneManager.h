#pragma once

#include <functional>
#include <memory>

#include "Scene.h"

namespace pyr
{

class SceneManager
{
public:
  using SceneSupplier = std::function<std::unique_ptr<Scene>()>;

  static SceneManager &getInstance() { return s_singleton; }

  void setInitialScene(std::unique_ptr<Scene> &&scene);
  Scene* getActiveScene() const { return m_activeScene.get(); }
  void transitionToScene(SceneSupplier nextSceneSupplier);
  void dispose();

  bool doSceneTransition();

  // creates a "scene supplier", a function that creates the scene when called
  // This is useful because scenes can be created at the right time, refer to the scene model
  template<class SceneType>
  static SceneSupplier make_supplier(auto &&...args)
  {
    return [...args = std::forward<decltype(args)>(args)]() mutable -> std::unique_ptr<Scene>
    {
      return std::make_unique<SceneType>(std::forward<decltype(args)>(args)...);
    };
  }

  void update(double delta) const;
  void render() const;

private:
  static SceneManager s_singleton;
  SceneSupplier m_nextScene;
  std::unique_ptr<Scene> m_activeScene;
};

}
