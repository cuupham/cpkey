#pragma once
#include "common.h"

class Tray {
  public:
	static void create(const HINSTANCE& hIns);
	static void refresh();
	static void remove();
};
