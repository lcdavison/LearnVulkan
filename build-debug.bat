@ECHO OFF

ECHO Generate Project

REM Generate Project
cmake -S . -B ./Build

ECHO Build Project: DEBUG

REM Build Debug
cmake --build ./Build --config DEBUG

PAUSE
