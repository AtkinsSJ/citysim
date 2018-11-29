@echo off

where /q cl.exe || call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

REM to send preprocessor output to a file, add this to the compiler flags:  -P -Fipreprocessed.output 

rem set COMPILER_FLAGS= -EHsc -MD -W4 -wd4201 -wd4996 -Zi -Od -DBUILD_DEBUG=1 -I ..\include -FC -P -Fipreprocessed.output 
set COMPILER_FLAGS= -EHsc -MD -W4 -wd4201 -wd4996 -Zi -Od -DBUILD_DEBUG=1 -I ..\include -FC
set LINKER_FLAGS= -SUBSYSTEM:WINDOWS -LIBPATH:..\lib\x86 -debug SDL2_image.lib SDL2main.lib SDL2.lib Shell32.lib opengl32.lib glew32.lib glu32.lib

pushd ..\build
cl ..\src\main.cpp %COMPILER_FLAGS% -link %LINKER_FLAGS%
popd