@ECHO OFF

ECHO Generate Project
CALL GenerateVS2022.bat

ECHO Build Project: RELEASE
CALL cmake --build ./Build --config RELEASE

PAUSE