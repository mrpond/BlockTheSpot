@echo off
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/thororen1234/BlockTheSpot/master/nointeractioninstall.ps1' | Invoke-Expression} -eq y"
powershell -Command "& {iwr -useb https://raw.githubusercontent.com/spicetify/spicetify-cli/master/install.ps1 | iex}"
powershell -Command "& {iwr -useb https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.ps1 | iex}"
exit /b