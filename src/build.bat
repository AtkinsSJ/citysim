@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

rem Copy assets!
xcopy ..\assets\*.png ..\build /C /D /Y
xcopy ..\assets\*.fnt ..\build /C /D /Y
xcopy ..\assets\*.gl ..\build /C /D /Y

set COMPILER_FLAGS= -EHsc -MD -W4 -wd4201 -wd4996 -Zi -Od -I ..\include -FC
set LINKER_FLAGS= -SUBSYSTEM:WINDOWS -LIBPATH:..\lib\x86 -debug SDL2_image.lib SDL2main.lib SDL2.lib Shell32.lib opengl32.lib glew32.lib glu32.lib

pushd ..\build
cl ..\src\main.cpp %COMPILER_FLAGS% -link %LINKER_FLAGS%
popd