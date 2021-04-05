@echo off
REM this is a copy of the batch file I use to start working on this.

explorer P:\citysim
cd P:\citysim
REM This is VS2017
start .\build\citysim.sln
start "" "C:\Program Files\Sublime Text 3\sublime_text.exe" -p P:\citysim\citysim.sublime-project

start "C:\Program Files\PuTTY\pageant.exe" C:\Users\msn\Documents\ssh\privatekey.ppk
start "" "C:\Program Files\Sublime Merge\sublime_merge.exe"

wt new-tab -p "C++ Dev" -d P:\citysim --title CitySim