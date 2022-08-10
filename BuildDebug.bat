@ECHO OFF

ECHO Generate Project

REM Generate Project
CALL GenerateVS2022.bat

ECHO Build Project: DEBUG

REM Build Debug
cmake --build ./Build --config DEBUG

PAUSE
