#include "user_inputs.h"

#include <algorithm>
#include <unordered_map>

#define DIRECTINPUT_VERSION 0x0800
#define NOMINMAX
#include <dinput.h>

#include "engine/d3ddevice.h"
#include "engine/directxlib.h"

namespace pyr
{

static constexpr size_t LISTENED_TO_KEYS_COUNT = 256;
using KeyboardRawState = char[LISTENED_TO_KEYS_COUNT];
using KeyboardState = bool[LISTENED_TO_KEYS_COUNT];

struct MouseRawState {
  long deltaX = 0, deltaY = 0, deltaScroll = 0;
  bool buttons[2]{};
};

struct GameControllerState
{
  DIJOYSTATE2 state;
  static const int NUM_BUTTONS = 32;
  bool buttons[NUM_BUTTONS];
  int dpadDirection;

  void updateFromDIState()
  {
    for (int i = 0; i < NUM_BUTTONS; ++i)
      buttons[i] = (state.rgbButtons[i] & 0x80) != 0;

    if (state.rgdwPOV[0] != -1)
      dpadDirection = state.rgdwPOV[0];
    else
      dpadDirection = -1;
  }
};

static_assert(sizeof(MouseRawState::buttons) == sizeof(MouseState::buttons));

static struct
{
  IDirectInput8       *directInput          = nullptr;
  IDirectInputDevice8 *keyboardHandle       = nullptr;
  IDirectInputDevice8 *mouseHandle          = nullptr;
  IDirectInputDevice8 *gameControllerHandle = nullptr;

  KeyboardState keyboardKeystatesBuffer{};
  KeyboardState consumableKeyboardEvents{};
  MouseState mouseState{};
  MouseState consumableMouseEvents{};

  HWND hWnd = nullptr;
  HINSTANCE hInstance = nullptr;

  bool cursorLockedInWindow = false;
  bool rewireInputsFlag = true;
  bool inputsAquiredFlag = false;

  GameControllerState gameControllerState{};
  GameControllerState consumableGameControllerEvents{};

  bool consumableDPadInput{};

} g_inputs;


void UserInputs::setWindowsHandle(HINSTANCE hInstance, HWND hWnd)
{
  g_inputs.hInstance = hInstance;
  g_inputs.hWnd = hWnd;
}

void UserInputs::loadGlobalResources()
{
  DXTry(
	DirectInput8Create(
	  g_inputs.hInstance,
	  DIRECTINPUT_VERSION,
	  IID_IDirectInput8,
	  reinterpret_cast<LPVOID*>(&g_inputs.directInput),
	  nullptr),
	"Could not create DirectX inputs");

  DXTry(g_inputs.directInput->CreateDevice(GUID_SysKeyboard, &g_inputs.keyboardHandle, nullptr), "Could not initialize keyboard inputs");
  DXTry(g_inputs.keyboardHandle->SetDataFormat(&c_dfDIKeyboard), "Could not initialize keyboard inputs");
  DXTry(g_inputs.keyboardHandle->SetCooperativeLevel(g_inputs.hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE), "Could not initialize keyboard inputs");
  DXTry(g_inputs.keyboardHandle->Acquire(), "Could not aquire keyboard inputs");

  DXTry(g_inputs.directInput->CreateDevice(GUID_SysMouse, &g_inputs.mouseHandle, nullptr), "Could not initialize mouse inputs");
  DXTry(g_inputs.mouseHandle->SetDataFormat(&c_dfDIMouse), "Could not initialize mouse inputs");
  DXTry(g_inputs.mouseHandle->SetCooperativeLevel(g_inputs.hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE), "Could not initialize mouse inputs");

  if (g_inputs.directInput->CreateDevice(GUID_Joystick, &g_inputs.gameControllerHandle, nullptr) == 0) {
    DXTry(g_inputs.gameControllerHandle->SetDataFormat(&c_dfDIJoystick2), "Could not set data format for game controller");
    DXTry(g_inputs.gameControllerHandle->SetCooperativeLevel(g_inputs.hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE), "Could not set cooperative level for game controller");
    DXTry(g_inputs.gameControllerHandle->Acquire(), "Could not acquire game controller inputs");
  }
}

void UserInputs::unloadGlobalResources()
{
  g_inputs.keyboardHandle->Unacquire();
  g_inputs.keyboardHandle->Release();
  g_inputs.mouseHandle->Unacquire();
  g_inputs.mouseHandle->Release();
  g_inputs.directInput->Release();
  if (g_inputs.gameControllerHandle) {
    g_inputs.gameControllerHandle->Unacquire();
    g_inputs.gameControllerHandle->Release();
  }
}

void UserInputs::pollEvents()
{
  using namespace std;

  if (g_inputs.rewireInputsFlag) {
	  g_inputs.keyboardHandle->Unacquire();
	  g_inputs.mouseHandle->Unacquire();
	  if (g_inputs.keyboardHandle->Acquire() == DI_OK && g_inputs.mouseHandle->Acquire() == DI_OK) {
	    g_inputs.rewireInputsFlag = false;
	    g_inputs.inputsAquiredFlag = true;
	    setCursorLocked(g_inputs.cursorLockedInWindow);
	  }
  }

  if (!g_inputs.inputsAquiredFlag)
	return;

  // poll actual inputs states
  KeyboardRawState keyboardRawSnapshot;
  MouseRawState mouseRawSnapshot{};
  KeyboardState keyboardSnapshot{};
  if(g_inputs.keyboardHandle->GetDeviceState(sizeof(KeyboardState), reinterpret_cast<LPVOID>(&keyboardRawSnapshot)) != S_OK ||
	  g_inputs.mouseHandle->GetDeviceState(sizeof(MouseRawState), reinterpret_cast<LPVOID>(&mouseRawSnapshot)) != S_OK)
  {
	  g_inputs.inputsAquiredFlag = false;
	  g_inputs.rewireInputsFlag = true;
    throw std::runtime_error("Failed to read devices");
  }
  ranges::transform(keyboardRawSnapshot, begin(keyboardSnapshot),
    [](unsigned char rawState) -> bool { return (rawState & 0x80) != 0; });
  POINT cursorPosition;
  WinTry(GetCursorPos(&cursorPosition));
  WinTry(ScreenToClient(g_inputs.hWnd, &cursorPosition));
  
  // update keyboard diff/state
  for (size_t i = 0; i < sizeof(KeyboardState); i++)
	  if (keyboardSnapshot[i] != g_inputs.keyboardKeystatesBuffer[i])
	    g_inputs.consumableKeyboardEvents[i] = keyboardSnapshot[i];
  ranges::copy(keyboardSnapshot, begin(g_inputs.keyboardKeystatesBuffer));

  // update mouse diff/state
  g_inputs.consumableMouseEvents.deltaX += mouseRawSnapshot.deltaX - g_inputs.mouseState.deltaX;
  g_inputs.consumableMouseEvents.deltaY += mouseRawSnapshot.deltaY - g_inputs.mouseState.deltaY;
  g_inputs.consumableMouseEvents.deltaScroll += mouseRawSnapshot.deltaScroll - g_inputs.mouseState.deltaScroll;
  for (size_t i = 0; i < sizeof(mouseRawSnapshot.buttons); i++)
	  if (mouseRawSnapshot.buttons[i] != g_inputs.mouseState.buttons[i])
	    g_inputs.consumableMouseEvents.buttons[i] = mouseRawSnapshot.buttons[i];
  g_inputs.mouseState.deltaX = mouseRawSnapshot.deltaX;
  g_inputs.mouseState.deltaY = mouseRawSnapshot.deltaY;
  g_inputs.mouseState.deltaScroll = mouseRawSnapshot.deltaScroll;
  ranges::copy(mouseRawSnapshot.buttons, begin(g_inputs.mouseState.buttons));
  g_inputs.mouseState.screenX = cursorPosition.x;
  g_inputs.mouseState.screenY = cursorPosition.y;

  // Polling de la manette
  if (!g_inputs.gameControllerHandle) return;
  HRESULT hr = g_inputs.gameControllerHandle->Poll();
  if (FAILED(hr)) {
    // La manette a perdu son acquisition, ou l'appel a �chou� pour une autre raison
    hr = g_inputs.gameControllerHandle->Acquire();
    while (hr == DIERR_INPUTLOST) {
      hr = g_inputs.gameControllerHandle->Acquire();
    }

    // Si l'acquisition �choue toujours, sortir de la fonction
    if ((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED)) {
      // G�rer les erreurs sp�cifiques ici
      return;
    }

    // Si la manette est d�connect�e (par exemple, d�branch�e)
    if (hr == DIERR_NOTACQUIRED || hr == DIERR_OTHERAPPHASPRIO) {
      // G�rer la d�connexion ici
      return;
    }
  }

  GameControllerState gameControllerSnapshot{};

  if (g_inputs.gameControllerHandle->GetDeviceState(sizeof(DIJOYSTATE2), &gameControllerSnapshot) == DI_OK) {
    gameControllerSnapshot.updateFromDIState();
	for (size_t i = 0; i < sizeof(gameControllerSnapshot.buttons); i++) {
		if (gameControllerSnapshot.buttons[i] != g_inputs.gameControllerState.buttons[i])
		{
			g_inputs.consumableGameControllerEvents.buttons[i] = gameControllerSnapshot.buttons[i];
		}
	}
	ranges::copy(gameControllerSnapshot.buttons, begin(g_inputs.gameControllerState.buttons));
	g_inputs.gameControllerState.state = gameControllerSnapshot.state;
	g_inputs.consumableDPadInput = (g_inputs.gameControllerState.dpadDirection != gameControllerSnapshot.dpadDirection);
	g_inputs.gameControllerState.dpadDirection = gameControllerSnapshot.dpadDirection;
  } else {
    // G�rer les erreurs de r�cup�ration de l'�tat
    return;
  }
}

void UserInputs::setNextPollRewireInputs()
{
  g_inputs.rewireInputsFlag = true;
}

void UserInputs::setInputsNotAquired()
{
  g_inputs.inputsAquiredFlag = false;
}

void UserInputs::setCursorLocked(bool locked)
{
  g_inputs.cursorLockedInWindow = locked;
  if (locked) {
	  RECT clientRect;
	  GetClientRect(g_inputs.hWnd, &clientRect);
	  POINT ul{ clientRect.left, clientRect.top };
	  POINT br{ clientRect.right, clientRect.bottom };
	  MapWindowPoints(g_inputs.hWnd, nullptr, &ul, 1);
	  MapWindowPoints(g_inputs.hWnd, nullptr, &br, 1);
	  RECT clipRect{ ul.x, ul.y, br.x, br.y };
	  ClipCursor(&clipRect);
	  while(ShowCursor(false) >= 0);
  } else {
	  ClipCursor(nullptr);
	  while(ShowCursor(true) < 0);
  }
}

void UserInputs::consumeAllEvents()
{
  using namespace std;
  ranges::fill(g_inputs.consumableKeyboardEvents, false);
  ranges::fill(g_inputs.consumableMouseEvents.buttons, false);
  ranges::fill(g_inputs.consumableGameControllerEvents.buttons, false);
}

bool UserInputs::isKeyPressed(scancode_t scanCode)
{
  return g_inputs.keyboardKeystatesBuffer[scanCode];
}

bool UserInputs::consumeKeyPress(scancode_t scanCode)
{
  return std::exchange(g_inputs.consumableKeyboardEvents[scanCode], false);
}

const MouseState &UserInputs::getMouseState()
{
  return g_inputs.mouseState;
}

ScreenPoint UserInputs::getMousePosition()
{
  float winWidth = static_cast<float>(Device::getWinWidth());
  float winHeight = static_cast<float>(Device::getWinHeight());
  return {
	  (float)g_inputs.mouseState.screenX / winWidth * ScreenRegion::SCREEN_WIDTH,
	  (winHeight - (float)g_inputs.mouseState.screenY) / winHeight * ScreenRegion::SCREEN_HEIGHT
  };
}

bool UserInputs::consumeClick(MouseState::button_t button)
{
  return std::exchange(g_inputs.consumableMouseEvents.buttons[button], false);
}

bool UserInputs::isClickPressed(MouseState::button_t button)
{
  return g_inputs.mouseState.buttons[button];
}

MouseState UserInputs::consumeMouseEvents()
{
  return std::exchange(g_inputs.consumableMouseEvents, {});
}

bool UserInputs::isButtonPressed(scancode_t scanCode)
{
  if (!g_inputs.gameControllerHandle) return false;
  return g_inputs.gameControllerState.buttons[scanCode];
}

bool UserInputs::consumeButtonPress(scancode_t scanCode)
{
	return std::exchange(g_inputs.consumableGameControllerEvents.buttons[scanCode], false);
}

bool UserInputs::isDPadPressed(int scanCode)
{
  if (!g_inputs.gameControllerHandle) return false;
  return g_inputs.gameControllerState.dpadDirection == scanCode;
}

bool UserInputs::consumeDPadPress(int scanCode)
{
	return g_inputs.consumableDPadInput && scanCode == g_inputs.gameControllerState.dpadDirection;
}

float UserInputs::getLeftStickXAxis()
{
	return (g_inputs.gameControllerState.state.lX / 65535.f - 0.5f) * 2.f;
}

float UserInputs::getLeftStickYAxis()
{
	return -(g_inputs.gameControllerState.state.lY / 65535.f - 0.5f) * 2.f;
}

float UserInputs::getZAxis()
{
	return -(g_inputs.gameControllerState.state.lZ / 65535.f - 0.5f) * 2.f;
}

bool UserInputs::hasController()
{
	return !(!g_inputs.gameControllerHandle);
}

} // namespace pyr
