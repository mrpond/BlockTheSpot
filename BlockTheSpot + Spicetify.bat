@echo off
powershell -Command "& {iwr -useb https://raw.githubusercontent.com/spicetify/spicetify-cli/master/install.ps1 | iex}"
powershell -Command "& {iwr -useb https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.ps1 | iex}"
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1' | Invoke-Expression}"
pause
exit /b