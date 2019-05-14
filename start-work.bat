@echo off
REM this is a copy of the batch file I use to start working on this.

explorer C:\Users\msn\Documents\under-london
cd C:\Users\msn\Documents\under-london
start .\build\under-london.sln
start "" "C:\Program Files\Sublime Text 3\sublime_text.exe" -p C:\Users\msn\Documents\under-london\under-london.sublime-project

REM cd "C:\Program Files (x86)\GitExtensions"
REM gitex openrepo "C:\Users\msn\Documents\under-london"

start "C:\Program Files\PuTTY\pageant.exe" C:\Users\msn\Documents\ssh\privatekey.ppk
start "" "C:\Program Files\Sublime Merge\sublime_merge.exe"

cmd.exe /k "call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x64"