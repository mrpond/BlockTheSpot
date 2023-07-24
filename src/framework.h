#pragma once

#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <array>
#include <string_view>

#ifndef NDEBUG
#include <include/capi/cef_urlrequest_capi.h>
#endif

#include "BasicUtils/Utils.h"
#include "BasicUtils/Logger.h"
#include "BasicUtils/PatternScanner.h"
#include "BasicUtils/Memory.h"
#include "BasicUtils/Hooking.h"
#include "BasicUtils/Console.h"

using namespace Console;