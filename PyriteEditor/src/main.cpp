#include <algorithm>
#include <tchar.h>
#include <Windows.h>

#include "utils/debug.h"
#include "engine/windowsengine.h"
#include "engine/d3ddevice.h"

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#ifndef PYR_ISDEBUG
  try
  {
#endif
    pyr::EngineSettings settings;
    settings.appTitle = "PyriteEditor";
    pyr::Engine engine{ hInstance, std::move(settings) };
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
