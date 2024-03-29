@echo off

REM vcvarsall.bat is incredibly slow, like 30 seconds slow, so we avoid it if we can.
REM In reality, we run it inside a shell, then just build from there, but it's here just in case.
where /q cl.exe || call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

REM to send preprocessor output to a file, add this to the compiler flags:  -P -Fipreprocessed.output 

set COMPILER_FLAGS= -EHsc -MD -W4 -wd4201 -Zi -Od -Oi -std:c++latest -DBUILD_DEBUG=1 -I ..\include -FC 
set LINKER_FLAGS= -SUBSYSTEM:WINDOWS -LIBPATH:..\lib\x64 -debug SDL2_image.lib SDL2main.lib SDL2.lib Shell32.lib opengl32.lib glew32.lib glu32.lib

pushd build
cl ..\src\main.cpp %COMPILER_FLAGS% -link %LINKER_FLAGS%
popd