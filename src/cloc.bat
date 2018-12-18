@echo off
rem c:\tools\cloc app/src/ buildsrc/src/ aFileChooser/src/ svg/ data/
rem c:\tools\cloc --vcs=git --by-file-by-lang
@echo on
c:\tools\cloc --by-file-by-lang --skip-uniqueness . ../build/assets/