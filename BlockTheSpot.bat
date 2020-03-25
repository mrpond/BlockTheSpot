@echo off
setlocal enableextensions
taskkill /f /im Spotify.exe 2> NUL
taskkill /f /im spotifywebhelper.exe 2> NUL
powershell Get-AppxPackage -Name "SpotifyAB.SpotifyMusic" | findstr "PackageFullName" > NUL
if %errorlevel% EQU 0 (
	echo.
	echo The Microsoft Store version of Spotify has been detected which is not supported.
	echo Please uninstall it first, and Run this file again.
	echo.
	echo To uninstall, search for Spotify in the start menu and right-click on the result and click Uninstall.
	echo.
	exit /b
)
for /f delims^=^"^ tokens^=2 %%A in ('reg query HKCR\spotify\shell\open\command') do (
	if exist "%%~dpA\Spotify.exe" set p=%%~dpA
)
if defined p (
	echo Removing auto update for spotify...
	icacls "%localappdata%\Spotify\Update" /reset /T > NUL 2>&1
	rd /s /q "%localappdata%\Spotify\Update" > NUL 2>&1
	mkdir "%localappdata%\Spotify\Update" > NUL 2>&1
	icacls "%localappdata%\Spotify\Update" /deny "%username%":W > NUL 2>&1
	echo Patching Spotify
	move "%APPDATA%\Spotify\chrome_elf.dll" "%APPDATA%\Spotify\chrome_elf.dll.bak" > NUL 2>&1
	copy chrome_elf.dll "%APPDATA%\Spotify\" > NUL 2>&1
	copy config.ini "%APPDATA%\Spotify\" > NUL 2>&1
) else (
	echo Spotify installation was not detected. Downloading Latest Spotify full setup. 
	echo Please wait..
	curl https://download.scdn.co/SpotifyFullSetup.exe -O SpotifyFullSetup.exe
	if not exist "%appdata%\Spotify\" mkdir "%appdata%\Spotify" > NUL 2>&1
	echo Running installation...
	SpotifyFullSetup.exe
	powershell -ExecutionPolicy Bypass -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%userprofile%\Desktop\Spotify.lnk'); $S.TargetPath = '%appdata%\Spotify\Spotify.exe'; $S.Save()" > NUL 2>&1
	start "" "%appdata%\Spotify\Spotify.exe"
	del "SpotifyFullSetup.exe"
)
pause