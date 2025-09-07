@echo off
echo ========================================
echo  BlockTheSpot + Spicetify Installer
echo ========================================
echo.
echo This script will install:
echo - Spotify
echo - Spicetify
echo - BlockTheSpot
echo.

set /p UserInput="Do you want to continue with the installation? (y/n): "
if /i "%UserInput%"=="y" (
    echo.
    echo Downloading and running PowerShell installation script...
    powershell -ExecutionPolicy Bypass -Command "& {try { Invoke-RestMethod -Uri 'https://raw.githubusercontent.com/Othmane-ElAlami/BlockTheSpot/master/BlockTheSpot%%20+%%20Spicetify.ps1' -UseBasicParsing | Invoke-Expression } catch { Write-Host 'Error downloading script:' $_.Exception.Message; Read-Host 'Press Enter to exit' }}"
) else (
    echo.
    echo Installation cancelled.
    pause
    exit /b
)
