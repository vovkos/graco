#pragma once

#include "axl_g_Pch.h"

//.............................................................................

// LUA

extern "C" {

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

} // extern "C" {

//.............................................................................

// AXL

#include "axl_lex_RagelLexer.h"
#include "axl_io_MappedFile.h"
#include "axl_lua_StringTemplate.h"
#include "axl_rtl_StringHashTable.h"
#include "axl_rtl_ArrayList.h"
#include "axl_rtl_BitMap.h"
#include "axl_io_FilePathUtils.h"

using namespace axl;

//.............................................................................

