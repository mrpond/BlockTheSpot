# Ignore errors from `Stop-Process`
$PSDefaultParameterValues['Stop-Process:ErrorAction'] = [System.Management.Automation.ActionPreference]::SilentlyContinue
function Get-File
{
  param (
    [Parameter(Mandatory, ValueFromPipelineByPropertyName)]
    [ValidateNotNullOrEmpty()]
    [System.Uri]
    $Uri,
    [Parameter(Mandatory, ValueFromPipelineByPropertyName)]
    [ValidateNotNullOrEmpty()]
    [System.IO.FileInfo]
    $TargetFile,
    [Parameter(ValueFromPipelineByPropertyName)]
    [ValidateNotNullOrEmpty()]
    [Int32]
    $BufferSize = 1,
    [Parameter(ValueFromPipelineByPropertyName)]
    [ValidateNotNullOrEmpty()]
    [ValidateSet('KB, MB')]
    [String]
    $BufferUnit = 'MB',
    [Parameter(ValueFromPipelineByPropertyName)]
    [ValidateNotNullOrEmpty()]
    [ValidateSet('KB, MB')]
    [Int32]
    $Timeout = 10000
  )

  [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

  $useBitTransfer = $null -ne (Get-Module -Name BitsTransfer -ListAvailable) -and ($PSVersionTable.PSVersion.Major -le 5) -and ((Get-Service -Name BITS).StartType -ne [System.ServiceProcess.ServiceStartMode]::Disabled)

  if ($useBitTransfer)
  {
    Write-Information -MessageData 'Using a fallback BitTransfer method since you are running Windows PowerShell'
    Start-BitsTransfer -Source $Uri -Destination "$($TargetFile.FullName)"
  }
  else
  {
    $request = [System.Net.HttpWebRequest]::Create($Uri)
    $request.set_Timeout($Timeout) #15 second timeout
    $response = $request.GetResponse()
    $totalLength = [System.Math]::Floor($response.get_ContentLength() / 1024)
    $responseStream = $response.GetResponseStream()
    $targetStream = New-Object -TypeName ([System.IO.FileStream]) -ArgumentList "$($TargetFile.FullName)", Create
    switch ($BufferUnit)
    {
      'KB' { $BufferSize = $BufferSize * 1024 }
      'MB' { $BufferSize = $BufferSize * 1024 * 1024 }
      Default { $BufferSize = 1024 * 1024 }
    }
    Write-Verbose -Message "Buffer size: $BufferSize B ($($BufferSize/("1$BufferUnit")) $BufferUnit)"
    $buffer = New-Object byte[] $BufferSize
    $count = $responseStream.Read($buffer, 0, $buffer.length)
    $downloadedBytes = $count
    $downloadedFileName = $Uri -split '/' | Select-Object -Last 1
    while ($count -gt 0)
    {
      $targetStream.Write($buffer, 0, $count)
      $count = $responseStream.Read($buffer, 0, $buffer.length)
      $downloadedBytes = $downloadedBytes + $count
      Write-Progress -Activity "Downloading file '$downloadedFileName'" -Status "Downloaded ($([System.Math]::Floor($downloadedBytes/1024))K of $($totalLength)K): " -PercentComplete ((([System.Math]::Floor($downloadedBytes / 1024)) / $totalLength) * 100)
    }

    Write-Progress -Activity "Finished downloading file '$downloadedFileName'"

    $targetStream.Flush()
    $targetStream.Close()
    $targetStream.Dispose()
    $responseStream.Dispose()
  }
}

Write-Host @'
*****************
@mrpond message:
#Thailand #ThaiProtest #ThailandProtest #freeYOUTH
Please retweet these hashtag, help me stop dictator government!
*****************
'@

Write-Host @'
*****************
Authors: @Nuzair46, @KUTlime
*****************
'@

$spotifyDirectory = Join-Path -Path $env:APPDATA -ChildPath 'Spotify'
$spotifyExecutable = Join-Path -Path $spotifyDirectory -ChildPath 'Spotify.exe'
$spotifyApps = Join-Path -Path $spotifyDirectory -ChildPath 'Apps'

Write-Host "Stopping Spotify...`n"
Stop-Process -Name Spotify
Stop-Process -Name SpotifyWebHelper

if ($PSVersionTable.PSVersion.Major -ge 7)
{
  Import-Module Appx -UseWindowsPowerShell
}

if (Get-AppxPackage -Name SpotifyAB.SpotifyMusic)
{
  Write-Host "The Microsoft Store version of Spotify has been detected which is not supported.`n"

  $ch = Read-Host -Prompt 'Uninstall Spotify Windows Store edition (Y/N)'
  if ($ch -eq 'y')
  {
    Write-Host "Uninstalling Spotify.`n"
    Get-AppxPackage -Name SpotifyAB.SpotifyMusic | Remove-AppxPackage
  }
  else
  {
    Read-Host "Exiting...`nPress any key to exit..."
    exit
  }
}

Push-Location -LiteralPath $env:TEMP
try
{
  # Unique directory name based on time
  New-Item -Type Directory -Name "BlockTheSpot-$(Get-Date -UFormat '%Y-%m-%d_%H-%M-%S')" |
  Convert-Path |
  Set-Location
}
catch
{
  Write-Output $_
  Read-Host 'Press any key to exit...'
  exit
}

Write-Host "Downloading latest patch (chrome_elf.zip)...`n"
$elfPath = Join-Path -Path $PWD -ChildPath 'chrome_elf.zip'
try
{
  $uri = 'https://github.com/mrpond/BlockTheSpot/releases/latest/download/chrome_elf.zip'
  Get-File -Uri $uri -TargetFile "$elfPath"
}
catch
{
  Write-Output $_
  Start-Sleep
}

Expand-Archive -Force -LiteralPath "$elfPath" -DestinationPath $PWD
Remove-Item -LiteralPath "$elfPath" -Force

$spotifyInstalled = Test-Path -LiteralPath $spotifyExecutable
$update = $false
if ($spotifyInstalled)
{
  $ch = Read-Host -Prompt 'Optional - Update Spotify to the latest version. (Might already be updated). (Y/N)'
  if ($ch -eq 'y')
  {
    $update = $true
  }
  else
  {
    Write-Host 'Won''t try to update Spotify.'
  }
}
else
{
  Write-Host 'Spotify installation was not detected.'
}
if (-not $spotifyInstalled -or $update)
{
  Write-Host 'Downloading the latest Spotify full setup, please wait...'
  $spotifySetupFilePath = Join-Path -Path $PWD -ChildPath 'SpotifyFullSetup.exe'
  try
  {
    $uri = 'https://download.scdn.co/SpotifyFullSetup.exe'
    Get-File -Uri $uri -TargetFile "$spotifySetupFilePath"
  }
  catch
  {
    Write-Output $_
    Read-Host 'Press any key to exit...'
    exit
  }
  New-Item -Path $spotifyDirectory -ItemType:Directory -Force | Write-Verbose

  [System.Security.Principal.WindowsPrincipal] $principal = [System.Security.Principal.WindowsIdentity]::GetCurrent()
  $isUserAdmin = $principal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)
  Write-Host 'Running installation...'
  if ($isUserAdmin)
  {
    Write-Host
    Write-Host 'Creating scheduled task...'
    $apppath = 'powershell.exe'
    $taskname = 'Spotify install'
    $action = New-ScheduledTaskAction -Execute $apppath -Argument "-NoLogo -NoProfile -Command & `'$spotifySetupFilePath`'"
    $trigger = New-ScheduledTaskTrigger -Once -At (Get-Date)
    $settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -WakeToRun
    Register-ScheduledTask -Action $action -Trigger $trigger -TaskName $taskname -Settings $settings -Force | Write-Verbose
    Write-Host 'The install task has been scheduled. Starting the task...'
    Start-ScheduledTask -TaskName $taskname
    Start-Sleep -Seconds 2
    Write-Host 'Unregistering the task...'
    Unregister-ScheduledTask -TaskName $taskname -Confirm:$false
    Start-Sleep -Seconds 2
  }
  else
  {
    Start-Process -FilePath "$spotifySetupFilePath"
  }

  while ($null -eq (Get-Process -Name Spotify -ErrorAction SilentlyContinue))
  {
    # Waiting until installation complete
    Start-Sleep -Milliseconds 100
  }

  # Create a Shortcut to Spotify in %APPDATA%\Microsoft\Windows\Start Menu\Programs and Desktop
  # (allows the program to be launched from search and desktop)
  $wshShell = New-Object -ComObject WScript.Shell
  
  $desktopShortcutPath = "$env:USERPROFILE\Desktop\Spotify.lnk"
  if ((Test-Path $desktopShortcutPath) -eq $false)
  {
    $desktopShortcut = $wshShell.CreateShortcut($desktopShortcutPath)
    $desktopShortcut.TargetPath = "$env:APPDATA\Spotify\Spotify.exe"
    $desktopShortcut.Save()
  }

  $startMenuShortcutPath = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Spotify.lnk"
  if ((Test-Path $startMenuShortcutPath) -eq $false)
  {
    $startMenuShortcut = $wshShell.CreateShortcut($startMenuShortcutPath)
    $startMenuShortcut.TargetPath = "$env:APPDATA\Spotify\Spotify.exe"
    $startMenuShortcut.Save()
  }
  

  Write-Host 'Stopping Spotify...Again'

  Stop-Process -Name Spotify
  Stop-Process -Name SpotifyWebHelper
  Stop-Process -Name SpotifyFullSetup
}
$elfDllBackFilePath = Join-Path -Path $spotifyDirectory -ChildPath 'chrome_elf_bak.dll'
$elfBackFilePath = Join-Path -Path $spotifyDirectory -ChildPath 'chrome_elf.dll'
if ((Test-Path $elfDllBackFilePath) -eq $false)
{
  Move-Item -LiteralPath "$elfBackFilePath" -Destination "$elfDllBackFilePath" | Write-Verbose
}

Write-Host 'Patching Spotify...'
$patchFiles = (Join-Path -Path $PWD -ChildPath 'chrome_elf.dll'), (Join-Path -Path $PWD -ChildPath 'config.ini')

Copy-Item -LiteralPath $patchFiles -Destination "$spotifyDirectory"

$ch = Read-Host -Prompt 'Optional - Remove ad placeholder and upgrade button. (Y/N)'
if ($ch -eq 'y')
{
  $xpuiBundlePath = Join-Path -Path $spotifyApps -ChildPath 'xpui.spa'
  $xpuiUnpackedPath = Join-Path -Path (Join-Path -Path $spotifyApps -ChildPath 'xpui') -ChildPath 'xpui.js'
  $fromZip = $false

  # Try to read xpui.js from xpui.spa for normal Spotify installations, or
  # directly from Apps/xpui/xpui.js in case Spicetify is installed.
  if (Test-Path $xpuiBundlePath)
  {
    Add-Type -Assembly 'System.IO.Compression.FileSystem'
    Copy-Item -Path $xpuiBundlePath -Destination "$xpuiBundlePath.bak"

    $zip = [System.IO.Compression.ZipFile]::Open($xpuiBundlePath, 'update')
    $entry = $zip.GetEntry('xpui.js')

    # Extract xpui.js from zip to memory
    $reader = New-Object System.IO.StreamReader($entry.Open())
    $xpuiContents = $reader.ReadToEnd()
    $reader.Close()

    $fromZip = $true
  }
  elseif (Test-Path $xpuiUnpackedPath)
  {
    Copy-Item -LiteralPath $xpuiUnpackedPath -Destination "$xpuiUnpackedPath.bak"
    $xpuiContents = Get-Content -LiteralPath $xpuiUnpackedPath -Raw

    Write-Host 'Spicetify detected - You may need to reinstall BTS after running "spicetify apply".';
  }
  else
  {
    Write-Host 'Could not find xpui.js, please open an issue on the BlockTheSpot repository.'
  }

  if ($xpuiContents)
  {
    # Replace ".ads.leaderboard.isEnabled" + separator - '}' or ')'
    # With ".ads.leaderboard.isEnabled&&false" + separator
    $xpuiContents = $xpuiContents -replace '(\.ads\.leaderboard\.isEnabled)(}|\))', '$1&&false$2'

    # Delete ".createElement(XX,{(spec:X),?onClick:X,className:XX.X.UpgradeButton}),X()"
    $xpuiContents = $xpuiContents -replace '\.createElement\([^.,{]+,{(?:spec:[^.,]+,)?onClick:[^.,]+,className:[^.]+\.[^.]+\.UpgradeButton}\),[^.(]+\(\)', ''

    if ($fromZip)
    {
      # Rewrite it to the zip
      $writer = New-Object System.IO.StreamWriter($entry.Open())
      $writer.BaseStream.SetLength(0)
      $writer.Write($xpuiContents)
      $writer.Close()

      $zip.Dispose()
    }
    else
    {
      Set-Content -LiteralPath $xpuiUnpackedPath -Value $xpuiContents
    }
  }
}
else
{
  Write-Host "Won't remove ad placeholder and upgrade button.`n"
}

$tempDirectory = $PWD
Pop-Location

Remove-Item -LiteralPath $tempDirectory -Recurse

Write-Host 'Patching Complete, starting Spotify...'

Start-Process -WorkingDirectory $spotifyDirectory -FilePath $spotifyExecutable
Write-Host 'Done.'

Write-Host @'
*****************
@mrpond message:
#Thailand #ThaiProtest #ThailandProtest #freeYOUTH
Please retweet these hashtag, help me stop dictator government!
*****************
'@

exit
