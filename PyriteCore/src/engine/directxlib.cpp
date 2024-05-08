#include "Directxlib.h"

#include <string>

void WinTry(int result)
{
  if(result == 0)
	throw windows_error("Win error: " + std::to_string(GetLastError()));
}
