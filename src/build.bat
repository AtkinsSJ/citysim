@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

pushd ..\build
cl ..\src\main.cpp -EHsc -FC -Zi -MT -DSFML_STATIC -I ..\include -link ..\lib\sfml-graphics-s.lib ..\lib\sfml-window-s.lib ..\lib\sfml-system-s.lib
popd