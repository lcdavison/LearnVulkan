@ECHO OFF

CD ..

REM Configure
CALL cmake -S . -B .\Build -G %1

CD .\BatchFiles