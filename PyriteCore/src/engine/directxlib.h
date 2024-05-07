#pragma once

#include <stdexcept>

#define NOMINMAX
#define _XM_NO_INTRINSICS_
#include <d3d11.h>
#include <Effects11/d3dx11effect.h>

struct directx_error : std::runtime_error {
  explicit directx_error(const std::string &message) : std::runtime_error(message) {}
};
struct windows_error : std::runtime_error {
  explicit windows_error(const std::string &message) : std::runtime_error(message) {}
};

template<class T, class M>
  requires std::is_convertible_v<M, std::string>
void DXTry(const T& result, const M &errorMessage) {
  if (result != 0)
		throw directx_error(errorMessage);
}

template<class T>
void DXRelease(T& ptr) noexcept {
  if (ptr != nullptr) {
	  ptr->Release();
	  ptr = nullptr;
  }
}

void WinTry(int result);
