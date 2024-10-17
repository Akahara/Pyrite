#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "Scene.h"

namespace pyr
{
using SceneSupplier = std::function<std::unique_ptr<Scene>()>;

class SceneManager
{
public:
  static SceneManager& getInstance() { 
      static SceneManager instance;
      return instance; 
  }
  static Scene* getActiveScene() { return getInstance().m_activeScene.get(); }
  static RegisteredRenderableActorCollection& GetCurrentSceneActors() { return getActiveScene()->SceneActors; }

public:

  template<class SceneType> requires std::is_base_of_v<Scene, SceneType>
  void registerScene(const std::string& name, auto&&... sceneArgs)
  {
    m_knownScenes[name] = make_scene_supplier<SceneType>(std::forward<decltype(sceneArgs)>(sceneArgs)...);
  }

  SceneSupplier getRegisteredScene(const std::string& sceneName)
  {
    auto it = m_knownScenes.find(sceneName);
    return it != m_knownScenes.end() ? it->second : SceneSupplier{};
  }

  void transitionToScene(SceneSupplier nextSceneSupplier);
  bool transitionToScene(const std::string& sceneName);
  void dispose();

  bool doSceneTransition();

  void update(double delta);
  void render();

  // creates a "scene supplier", a function that creates the scene when called
  // This is useful because scenes can be created at the right time, refer to the scene model
  template<class SceneType> requires std::is_base_of_v<Scene, SceneType>
  static SceneSupplier make_scene_supplier(auto &&...args)
  {
    return [...args = std::forward<decltype(args)>(args)]() mutable -> std::unique_ptr<Scene>
    {
      return std::make_unique<SceneType>(std::forward<decltype(args)>(args)...);
    };
  }



private:

  std::map<std::string, SceneSupplier> m_knownScenes;
  std::string m_activeSceneName;
  SceneSupplier m_nextScene;
  std::unique_ptr<Scene> m_activeScene;
};

}
