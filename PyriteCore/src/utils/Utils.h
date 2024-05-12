#pragma once

template<class ...T>
struct Overloaded : T... {
  using T::operator()...;
};