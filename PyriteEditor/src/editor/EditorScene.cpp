#include "EditorScene.h"

#include "json.h"

namespace pye
{

static constexpr const char *FILE_VERSION = "v0.0.1";

EditorScene::EditorScene(const std::filesystem::path &sceneFile)
  : m_sceneFile(sceneFile)
{

}

void EditorScene::loadSceneFile(const std::filesystem::path &sceneFile)
{
  //using namespace json;
  try {
    JsonValue val = json::parseFile(sceneFile);
    if (val["version"].asString() != FILE_VERSION)
      throw json_parser_error(Logger::concat("Parser version mismatch, expected ", FILE_VERSION, ", got ", val["version"].asString()));

  } catch (const json_parser_error& e) {
    
  }
}

  

}
