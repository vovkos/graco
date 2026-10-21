:: .............................................................................
::
::  This file is part of the AXL library.
::
::  AXL is distributed under the MIT license.
::  For details see accompanying license.txt file,
::  the public copy of which is also available at:
::  http://tibbo.com/downloads/archive/axl/license.txt
::
:: .............................................................................

@echo off

:loop

if "%1" == "" goto :finalize

if /i "%1" == "amd64" goto :amd64
if /i "%1" == "x86_64" goto :amd64
if /i "%1" == "x64" goto :amd64
if /i "%1" == "x86" goto :x86
if /i "%1" == "i386" goto :x86
if /i "%1" == "arm64" goto :arm64
if /i "%1" == "aarch64" goto :arm64
if /i "%1" == "msvc17" goto :msvc17
if /i "%1" == "msvc18" goto :msvc18
if /i "%1" == "v142" goto :toolset
if /i "%1" == "v143" goto :toolset
if /i "%1" == "v145" goto :toolset

echo Invalid argument: '%1'
exit -1

:: . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

:: CPU

:x86
set TARGET_CPU=x86
set CMAKE_ARCH_OPTIONS=-A Win32
shift
goto :loop

:amd64
set TARGET_CPU=amd64
set CMAKE_ARCH_OPTIONS=-A x64
shift
goto :loop

:arm64
set TARGET_CPU=arm64
set CMAKE_ARCH_OPTIONS=-A ARM64
shift
goto :loop

:: . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

:: Toolchain

:msvc17
set TOOLCHAIN=msvc17
set CMAKE_GENERATOR=Visual Studio 17 2022
shift
goto :loop

:msvc18
set TOOLCHAIN=msvc18
set CMAKE_GENERATOR=Visual Studio 18 2026
shift
goto :loop

:toolset
set CMAKE_TOOLSET_OPTIONS=-T %1
shift
goto :loop

:: . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

:finalize

if "%TARGET_CPU%" == "" goto :amd64
if "%TOOLCHAIN%" == "" goto :msvc17
if "%CONFIGURATION%" == "" (set CONFIGURATION=Release)

set LUA_VERSION=5.4.7
set LUA_DOWNLOAD_URL=https://github.com/walterschell/Lua/archive/refs/tags/v%LUA_VERSION%.zip

set RAGEL_DOWNLOAD_URL=https://github.com/eloraiby/ragel-windows/raw/master/ragel.exe

set CMAKE_CONFIGURE_FLAGS=-G "%CMAKE_GENERATOR%" %CMAKE_ARCH_OPTIONS% %CMAKE_TOOLSET_OPTIONS%

set CMAKE_BUILD_FLAGS= ^
	--config %CONFIGURATION% ^
	-- ^
	/nologo ^
	/verbosity:minimal ^
	/consoleloggerparameters:Summary

echo ---------------------------------------------------------------------------
echo LUA_DOWNLOAD_URL:     %LUA_DOWNLOAD_URL%
echo RAGEL_DOWNLOAD_URL:   %RAGEL_DOWNLOAD_URL%
echo ---------------------------------------------------------------------------
