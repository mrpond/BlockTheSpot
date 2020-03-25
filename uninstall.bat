@echo off
echo Author: @rednek46
echo Enabling auto update for spotify...
icacls "%localappdata%\Spotify\Update" /reset /T > NUL 2>&1
echo Removing Patch
del /s /q "%APPDATA%\Spotify\chrome_elf.dll" > NUL 2>&1
move "%APPDATA%\Spotify\chrome_elf.dll.bak" "%APPDATA%\Spotify\chrome_elf.dll" > NUL 2>&1
pause