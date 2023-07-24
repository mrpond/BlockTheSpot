#pragma once

#define _CRT_SECURE_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <Windows.h>
#include <array>
#include <string_view>

#ifndef NDEBUG
#include <include/capi/cef_urlrequest_capi.h>
#endif

#pragma comment(lib, "BasicUtils.lib")

#include <Utils.h>
#include <Logger.h>
#include <PatternScanner.h>
#include <Memory.h>
#include <Hooking.h>
#include <Console.h>

using namespace Console;