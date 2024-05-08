#include <algorithm>
#include <tchar.h>
#include <Windows.h>

#include "editorscene.h"
#include "TriangleScene.h"
#include "utils/debug.h"
#include "engine/windowsengine.h"
#include "engine/d3ddevice.h"
#include "scene/scene_manager.h"

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#ifndef PYR_ISDEBUG
  try
  {
#endif
    pyr::EngineSettings settings;
    settings.appTitle = "PyriteEditor";
    settings.bHasTitleBar = true;
    pyr::Engine engine{ hInstance, std::move(settings) };
    pyr::SceneManager::getInstance().setInitialScene(std::make_unique<pye::TriangleScene>());
    engine.run();
    return 0;
#ifndef PYR_ISDEBUG
  }
  catch (const std::exception& e)
  {
    wchar_t message[512];
    mbstowcs_s(nullptr, message, e.what(), std::size(message) - 1);
    MessageBox(nullptr, message, L"Fatal error!", MB_ICONWARNING);
    return -1;
  }
#endif
}
