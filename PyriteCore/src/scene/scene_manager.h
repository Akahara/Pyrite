#pragma once

#include <functional>
#include <memory>

#include "scene.h"

namespace pyr
{

class SceneManager
{
public:
  using SceneSupplier = std::function<std::unique_ptr<Scene>()>;

  static SceneManager &getInstance() { return s_singleton; }

  // BEWARE! this method cannot be called from an active scene, use disposeAllScenes() and pushLayeredScene() instead
  void setActiveScene(std::unique_ptr<Scene> &&scene);

  void disposeAllScenes();

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
  std::unique_ptr<Scene> m_activeScene;
};

}
