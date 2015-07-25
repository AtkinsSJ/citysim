@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

set COMPILER_FLAGS= -EHsc -MD -W4 -wd4201 -wd4127 -wd4996 -Zi -Od -I ..\include -FC
set LINKER_FLAGS= -SUBSYSTEM:WINDOWS -LIBPATH:..\lib\x86 -debug SDL2_image.lib SDL2main.lib SDL2.lib Shell32.lib opengl32.lib glew32.lib

pushd ..\build
cl ..\src\main.cpp %COMPILER_FLAGS% -link %LINKER_FLAGS%
popd