@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

pushd ..\build
cl ..\src\main.cpp -EHsc -MD -W4 -Zi -Od -I ..\include -link -SUBSYSTEM:CONSOLE -LIBPATH:..\lib\x86 -debug SDL2_image.lib SDL2main.lib SDL2.lib
popd