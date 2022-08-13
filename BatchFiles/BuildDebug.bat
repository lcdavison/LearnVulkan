@ECHO OFF

ECHO Generate Project
CALL GenerateVS2022.bat

ECHO Build Project: DEBUG
CALL cmake --build ./Build --config DEBUG

PAUSE
