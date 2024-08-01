::..............................................................................
::
::  This file is part of the AXL library.
::
::  AXL is distributed under the MIT license.
::  For details see accompanying license.txt file,
::  the public copy of which is also available at:
::  http://tibbo.com/downloads/archive/axl/license.txt
::
::..............................................................................

@echo off

set DOWNLOAD_DIR=c:\downloads
set DOWNLOAD_DIR_CMAKE=%DOWNLOAD_DIR:\=/%

:: . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

:: wget

echo Installing wget...

choco install wget --no-progress || exit

:: . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

:: Ragel

echo Downloading %RAGEL_DOWNLOAD_URL%...

mkdir %DOWNLOAD_DIR%
wget -nv %RAGEL_DOWNLOAD_URL% -O %DOWNLOAD_DIR%\ragel.exe || exit

echo set (RAGEL_EXE %DOWNLOAD_DIR_CMAKE%/ragel.exe) >> paths.cmake

:: . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

:: Lua (CMake-based)

echo Downloading %LUA_DOWNLOAD_URL%...

mkdir %DOWNLOAD_DIR%\lua
wget -nv %LUA_DOWNLOAD_URL% -O %DOWNLOAD_DIR%\lua\lua.zip || exit
7z x -y %DOWNLOAD_DIR%\lua\lua.zip || exit
ren lua-%LUA_VERSION% lua || exit

::..............................................................................
