@echo off
echo #pragma once > engine\Actors.h
echo. >> engine\Actors.h
echo // Auto-generated - includes all actor headers for registration >> engine\Actors.h
echo. >> engine\Actors.h
for %%f in (actors\*.h) do (
    set "file=%%f"
    set "file=!file:\=/!"
    echo #include "../!file!" >> engine\Actors.h
)