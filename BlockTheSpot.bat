@echo off
echo Author: @rednek46
echo Removing auto update for spotify...
icacls "%localappdata%\Spotify\Update" /reset /T > NUL 2>&1
rd /s /q "%localappdata%\Spotify\Update" > NUL 2>&1
mkdir "%localappdata%\Spotify\Update" > NUL 2>&1
icacls "%localappdata%\Spotify\Update" /deny "%username%":W > NUL 2>&1
echo Patching Spotify
move "%APPDATA%\Spotify\chrome_elf.dll" "%APPDATA%\Spotify\chrome_elf.dll.bak" > NUL 2>&1
copy chrome_elf.dll "%APPDATA%\Spotify\" > NUL 2>&1
copy config.ini "%APPDATA%\Spotify\" > NUL 2>&1
pause