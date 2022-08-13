@ECHO OFF

cd ..

REM Configure
CALL cmake -S . -B .\Build -G %1