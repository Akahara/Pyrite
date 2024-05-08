#pragma once

#include "engine/Directxlib.h"
#include "ScanCodes.h"

namespace pyr
{

using scancode_t = keys::scancode_t;

struct MouseState
{
  using button_t = uint8_t;

  static constexpr button_t
	BUTTON_PRIMARY = 0,
	BUTTON_SECONDARY = 1,
	BUTTON_COUNT = 2;

  long deltaX = 0, deltaY = 0, deltaScroll = 0;
  bool buttons[BUTTON_COUNT]{};
  long screenX = 0, screenY = 0;
};

struct ScreenPoint {
  float x = 0, y = 0;
};

struct ScreenRegion
{
  static constexpr float SCREEN_HEIGHT = 1.f;
  static inline float SCREEN_WIDTH{};
};

/*
User inputs consumer model:

Raw inputs states can be retrieved using "isKeyPressed" or "getMouseState", one-time 
events such as clicks and keydown events can be consumed using "consumeXX" methods.
If the user presses the A key for one second, only the first call to consumeKeyPress 
will return true, while isKeyPressed will return true for the press duration.
When changing scene, unconsumed events can be discarded using consumeAllEvents.
*/
class UserInputs
{
public:
  // suboptimal design, but creating a windows-only UserInputs subclass is overdoing it for this project
  static void setWindowsHandle(HINSTANCE hInstance, HWND hWnd);
  static void loadGlobalResources();
  static void unloadGlobalResources();

  static void pollEvents();
  static void setNextPollRewireInputs();
  static void setInputsNotAquired();
  static void setCursorLocked(bool locked);
  static void consumeAllEvents();

  static bool isKeyPressed(scancode_t scanCode);
  static bool consumeKeyPress(scancode_t scanCode);
  static const MouseState &getMouseState();
  static ScreenPoint getMousePosition();
  static bool consumeClick(MouseState::button_t button);
  static bool isClickPressed(MouseState::button_t button);
  static MouseState consumeMouseEvents();
  static bool isButtonPressed(scancode_t scanCode);
  static bool consumeButtonPress(scancode_t scanCode);
  static bool isDPadPressed(int scanCode);
  static bool consumeDPadPress(int scanCode);
  static float getLeftStickXAxis();
  static float getLeftStickYAxis();
  static float getZAxis(); //is L2 R2 axis

  static bool hasController();
};

} // namespace pyr
