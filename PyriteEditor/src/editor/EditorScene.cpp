#include "EditorScene.h"

#include "json.h"

namespace pye
{

static constexpr const char *FILE_VERSION = "v0.0.1";

EditorScene::EditorScene(std::filesystem::path sceneFile)
  : m_sceneFile(std::move(sceneFile))
{

}

void EditorScene::update(double delta)
{
}

void EditorScene::render()
{
}

void EditorScene::loadSceneFile(const std::filesystem::path &sceneFile)
{
  try {
    JsonValue val = json::parseFile(sceneFile);
    if (val["version"].asString() != FILE_VERSION)
      throw json_parser_error(Logger::concat("Parser version mismatch, expected ", FILE_VERSION, ", got ", val["version"].asString()));
    
    const std::vector<JsonValue>& serializedActors = val["actors"].asArray();


  } catch (const json_parser_error& e) {
    
  }
}

void EditorScene::writeSceneFile(const std::filesystem::path& sceneFile)
{
  JsonObject serializedScene;
  serializedScene.set("version", FILE_VERSION);
  std::vector<JsonValue> serializedActors;
  
  serializedScene.set("actors", std::move(serializedActors));
}

}
