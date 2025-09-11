@echo off
echo ========================================
echo  BlockTheSpot + Spicetify Installer
echo ========================================
echo.

set /p UserInput="Spicetify will be installed. If you don't agree, use the BlockTheSpot script. Do you want to continue with the installation? (y/n): "
if /i "%UserInput%"=="y" (
    echo.
    echo Installing BlockTheSpot with Spicetify...
    powershell -ExecutionPolicy Bypass -Command "try { $installScript = Invoke-RestMethod -Uri 'https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1' -UseBasicParsing; Invoke-Expression \"& { $installScript } -UninstallSpotifyStoreEdition -InstallSpicetify\" } catch { Write-Host 'Error downloading script:' $_.Exception.Message; Read-Host 'Press Enter to exit' }"
) else (
    echo.
    echo Installation cancelled.
    pause
    exit /b
)
