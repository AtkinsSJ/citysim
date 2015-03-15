@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

pushd ..\build
cl ..\src\main.cpp /DSFML_STATIC=1 /FC /Zi /I ..\include /Zl /link ..\lib\sfml-system-s.lib
popd