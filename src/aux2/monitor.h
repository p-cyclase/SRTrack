#pragma once

#include <windows.h>

struct HOST_APP_TABLE;

void initialize_monitor(HINSTANCE instance);
void register_monitor(HOST_APP_TABLE* host);
