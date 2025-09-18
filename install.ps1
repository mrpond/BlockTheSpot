param (
  [Parameter()]
  [switch]
  $UninstallSpotifyStoreEdition = (Read-Host -Prompt 'Uninstall Spotify Windows Store edition if it exists (Y/N)') -eq 'y',
  [Parameter()]
  [switch]
  $UpdateSpotify,
  [Parameter()]
  [switch]
  $InstallSpicetify
)

# Ignore errors from `Stop-Process`
$PSDefaultParameterValues['Stop-Process:ErrorAction'] = [System.Management.Automation.ActionPreference]::SilentlyContinue

[System.Version] $minimalSupportedSpotifyVersion = '1.2.8.923'

function Get-File {
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

  if ($useBitTransfer) {
    Write-Information -MessageData 'Using a fallback BitTransfer method since you are running Windows PowerShell'
    try {
      Start-BitsTransfer -Source $Uri -Destination "$($TargetFile.FullName)"
    }
    catch {
      Write-Warning "BITS transfer failed, falling back to HTTP method: $_"
      $useBitTransfer = $false
    }
  }
  
  if (-not $useBitTransfer) {
    $request = [System.Net.HttpWebRequest]::Create($Uri)
    $request.set_Timeout($Timeout) #15 second timeout
    $response = $request.GetResponse()
    $totalLength = [System.Math]::Floor($response.get_ContentLength() / 1024)
    $responseStream = $response.GetResponseStream()
    $targetStream = New-Object -TypeName ([System.IO.FileStream]) -ArgumentList "$($TargetFile.FullName)", Create
    switch ($BufferUnit) {
      'KB' { $BufferSize = $BufferSize * 1024 }
      'MB' { $BufferSize = $BufferSize * 1024 * 1024 }
      Default { $BufferSize = 1024 * 1024 }
    }
    Write-Verbose -Message "Buffer size: $BufferSize B ($($BufferSize/("1$BufferUnit")) $BufferUnit)"
    $buffer = New-Object byte[] $BufferSize
    $count = $responseStream.Read($buffer, 0, $buffer.length)
    $downloadedBytes = $count
    $downloadedFileName = $Uri -split '/' | Select-Object -Last 1
    while ($count -gt 0) {
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

function Test-SpotifyVersion {
  param (
    [Parameter(Mandatory, ValueFromPipelineByPropertyName)]
    [ValidateNotNullOrEmpty()]
    [System.Version]
    $MinimalSupportedVersion,
    [Parameter(Mandatory, ValueFromPipeline, ValueFromPipelineByPropertyName)]
    [System.Version]
    $TestedVersion
  )

  process {
    return ($MinimalSupportedVersion.CompareTo($TestedVersion) -le 0)
  }
}

function Install-Spicetify {
  param (
    [Parameter(Mandatory)]
    [string]$SpotifyDirectory,
    [Parameter(Mandatory)]
    [string]$SpotifyExecutable
  )

  Write-Host "`n========================================" -ForegroundColor Blue
  Write-Host " Installing Spicetify CLI" -ForegroundColor Cyan
  Write-Host "========================================" -ForegroundColor Blue

  # First, ensure Spotify runs long enough for configuration files to be created
  try {
    Start-Process -WorkingDirectory $SpotifyDirectory -FilePath $SpotifyExecutable -WindowStyle Minimized
    Start-Sleep -Seconds 5
    Stop-Process -Name Spotify -Force -ErrorAction SilentlyContinue
    Stop-Process -Name SpotifyWebHelper -Force -ErrorAction SilentlyContinue
  }
  catch {
    Write-Warning "Could not initialize Spotify for configuration"
  }

  try {
    Write-Host "Downloading and installing Spicetify CLI..." -ForegroundColor Green
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $spicetifyInstallScript = Invoke-RestMethod -Uri "https://raw.githubusercontent.com/spicetify/spicetify-cli/master/install.ps1" -UseBasicParsing
    Invoke-Expression $spicetifyInstallScript
    Write-Host "Spicetify CLI installed successfully!" -ForegroundColor Green
  }
  catch {
    Write-Warning "Failed to install Spicetify CLI: $($_.Exception.Message)"
    return $false
  }

  Write-Host "`n========================================" -ForegroundColor Blue
  Write-Host " Installing Spicetify Marketplace" -ForegroundColor Cyan
  Write-Host "========================================" -ForegroundColor Blue

  try {
    Write-Host "Installing Spicetify Marketplace..." -ForegroundColor Green
    $marketplaceScript = Invoke-RestMethod -Uri "https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.ps1" -UseBasicParsing
    Invoke-Expression $marketplaceScript
    Write-Host "Spicetify Marketplace installed successfully!" -ForegroundColor Green
    return $true
  }
  catch {
    Write-Warning "Failed to install Spicetify Marketplace: $($_.Exception.Message)"
    return $false
  }
}

function Install-BlockTheSpotPatch {
  param (
    [Parameter(Mandatory)]
    [string]$SpotifyDirectory,
    [Parameter(Mandatory)]
    [bool]$Is64Bit,
    [string]$WorkingDirectory = $PWD
  )

  Write-Host "Downloading and applying BlockTheSpot patch..." -ForegroundColor Green
  
  # Download chrome_elf.zip
  $elfPath = Join-Path -Path $WorkingDirectory -ChildPath 'chrome_elf.zip'
  try {
    if ($Is64Bit) {
      $uri = 'https://github.com/mrpond/BlockTheSpot/releases/latest/download/chrome_elf.zip'
    }
    else {
      $uri = 'https://github.com/mrpond/BlockTheSpot/releases/download/2023.5.20.80/chrome_elf.zip'
    }

    Write-Host "Attempting to download from: $uri"
    Get-File -Uri $uri -TargetFile "$elfPath"
    
    if (-not (Test-Path "$elfPath")) {
      throw "Download failed - file does not exist at $elfPath"
    }
    
    Write-Host "Download successful: $elfPath"
  }
  catch {
    Write-Output "Download error: $_"
    Write-Host "Attempting alternative download method..."
    try {
      if ($Is64Bit) {
        $uri = 'https://github.com/mrpond/BlockTheSpot/releases/latest/download/chrome_elf.zip'
      }
      else {
        $uri = 'https://github.com/mrpond/BlockTheSpot/releases/download/2023.5.20.80/chrome_elf.zip'
      }
      Invoke-WebRequest -Uri $uri -OutFile "$elfPath" -UseBasicParsing
      Write-Host "Alternative download successful"
    }
    catch {
      Write-Output "Alternative download also failed: $_"
      Write-Host "Note: This could be caused by antivirus software. Check your antivirus settings."
      Start-Sleep -Seconds 5
      Read-Host 'Press any key to exit...'
      exit
    }
  }

  # Extract and apply patch
  Expand-Archive -Force -LiteralPath "$elfPath" -DestinationPath $WorkingDirectory
  Remove-Item -LiteralPath "$elfPath" -Force

  Write-Host 'Patching Spotify...'
  $patchFiles = (Join-Path -Path $WorkingDirectory -ChildPath 'dpapi.dll'), (Join-Path -Path $WorkingDirectory -ChildPath 'config.ini')
  Copy-Item -LiteralPath $patchFiles -Destination "$SpotifyDirectory" -Force
  Remove-Item -LiteralPath (Join-Path -Path $SpotifyDirectory -ChildPath 'blockthespot_settings.json') -Force -ErrorAction SilentlyContinue
  
  Write-Host 'Patching Complete!' -ForegroundColor Green
}

Write-Host @'
========================================
Authors: @Nuzair46, @KUTlime, @O-ElAlami
========================================
'@

$spotifyDirectory = Join-Path -Path $env:APPDATA -ChildPath 'Spotify'
$spotifyExecutable = Join-Path -Path $spotifyDirectory -ChildPath 'Spotify.exe'

[System.Version] $actualSpotifyClientVersion = (Get-ChildItem -LiteralPath $spotifyExecutable -ErrorAction:SilentlyContinue).VersionInfo.ProductVersionRaw

Write-Host "Stopping Spotify...`n"
Stop-Process -Name Spotify
Stop-Process -Name SpotifyWebHelper

if ($PSVersionTable.PSVersion.Major -ge 7) {
  Import-Module Appx -UseWindowsPowerShell -WarningAction:SilentlyContinue
}

if (Get-AppxPackage -Name SpotifyAB.SpotifyMusic) {
  Write-Host "The Microsoft Store version of Spotify has been detected which is not supported.`n"

  if ($UninstallSpotifyStoreEdition) {
    Write-Host "Uninstalling Spotify.`n"
    Get-AppxPackage -Name SpotifyAB.SpotifyMusic | Remove-AppxPackage
  }
  else {
    Read-Host "Exiting...`nPress any key to exit..."
    exit
  }
}

Push-Location -LiteralPath $env:TEMP
try {
  # Unique directory name based on time
  New-Item -Type Directory -Name "BlockTheSpot-$(Get-Date -UFormat '%Y-%m-%d_%H-%M-%S')" |
  Convert-Path |
  Set-Location
}
catch {
  Write-Output $_
  Read-Host 'Press any key to exit...'
  exit
}

$spotifyInstalled = Test-Path -LiteralPath $spotifyExecutable

if (-not $spotifyInstalled) {
  $unsupportedClientVersion = $true
}
else {
  $unsupportedClientVersion = ($actualSpotifyClientVersion | Test-SpotifyVersion -MinimalSupportedVersion $minimalSupportedSpotifyVersion) -eq $false
}

if (-not $UpdateSpotify -and $unsupportedClientVersion) {
  if ((Read-Host -Prompt 'In order to install Block the Spot, your Spotify client must be updated. Do you want to continue? (Y/N)') -ne 'y') {
    exit
  }
}

if (-not $spotifyInstalled -or $UpdateSpotify -or $unsupportedClientVersion) {
  Write-Host 'Downloading the latest Spotify full setup, please wait...'
  $spotifySetupFilePath = Join-Path -Path $PWD -ChildPath 'SpotifyFullSetup.exe'
  try {
    if ([Environment]::Is64BitOperatingSystem) {
      # Check if the computer is running a 64-bit version of Windows
      $uri = 'https://download.scdn.co/SpotifyFullSetupX64.exe'
    }
    else {
      $uri = 'https://download.scdn.co/SpotifyFullSetup.exe'
    }
    Get-File -Uri $uri -TargetFile "$spotifySetupFilePath"
  }
  catch {
    Write-Output $_
    Read-Host 'Press any key to exit...'
    exit
  }
  New-Item -Path $spotifyDirectory -ItemType:Directory -Force | Write-Verbose

  [System.Security.Principal.WindowsPrincipal] $principal = [System.Security.Principal.WindowsIdentity]::GetCurrent()
  $isUserAdmin = $principal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)
  Write-Host 'Running installation...'
  if ($isUserAdmin) {
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
  else {
    Start-Process -FilePath "$spotifySetupFilePath"
  }

  while ($null -eq (Get-Process -Name Spotify -ErrorAction SilentlyContinue)) {
    # Waiting until installation complete
    Start-Sleep -Milliseconds 100
  }


  Write-Host 'Stopping Spotify...Again'

  Stop-Process -Name Spotify
  Stop-Process -Name SpotifyWebHelper
  if ([Environment]::Is64BitOperatingSystem) {
    # Check if the computer is running a 64-bit version of Windows
    Stop-Process -Name SpotifyFullSetupX64
  }
  else {
    Stop-Process -Name SpotifyFullSetup
  }
}

# Install Spicetify if requested
$spicetifyInstalled = $false
if ($InstallSpicetify) {
  Write-Host "`nStopping Spotify before Spicetify installation..."
  Stop-Process -Name Spotify -ErrorAction SilentlyContinue
  Stop-Process -Name SpotifyWebHelper -ErrorAction SilentlyContinue
  Start-Sleep -Seconds 2

  $spicetifyInstalled = Install-Spicetify -SpotifyDirectory $spotifyDirectory -SpotifyExecutable $spotifyExecutable
  
  if ($spicetifyInstalled) {
    Write-Host "`nSpicetify installation completed successfully!" -ForegroundColor Green
  }
  else {
    Write-Warning "Spicetify installation encountered issues, but continuing with BlockTheSpot installation..."
  }
}

# Detect if Spotify is 64-bit
$bytes = [System.IO.File]::ReadAllBytes($spotifyExecutable)
$peHeader = [System.BitConverter]::ToUInt16($bytes[0x3C..0x3D], 0)
$is64Bit = $bytes[$peHeader + 4] -eq 0x64

# Apply BlockTheSpot patch
Install-BlockTheSpotPatch -SpotifyDirectory $spotifyDirectory -Is64Bit $is64Bit

$tempDirectory = $PWD
Pop-Location

Remove-Item -LiteralPath $tempDirectory -Recurse

Write-Host 'Patching Complete!' -ForegroundColor Green

# Start Spotify only if Spicetify is not being installed (otherwise handle in re-application section)
if (-not $InstallSpicetify) {
  Write-Host 'Starting Spotify...'
  Start-Process -WorkingDirectory $spotifyDirectory -FilePath $spotifyExecutable
}

# Re-apply BlockTheSpot if Spicetify was installed (Spicetify can break BlockTheSpot)
if ($InstallSpicetify -and $spicetifyInstalled) {
  Write-Host "`n========================================" -ForegroundColor Blue
  Write-Host " Re-applying BlockTheSpot" -ForegroundColor Cyan
  Write-Host "========================================" -ForegroundColor Blue
  Write-Host "Ensuring BlockTheSpot works with Spicetify..." -ForegroundColor Yellow
  
  # Ensure Spotify is completely stopped
  Write-Host "Stopping Spotify processes..."
  Get-Process -Name "Spotify*" -ErrorAction SilentlyContinue | Stop-Process -Force
  Start-Sleep -Seconds 5
  
  # Clear Spotify cache to prevent conflicts
  try {
    $cacheDir = Join-Path $spotifyDirectory "Cache"
    if (Test-Path $cacheDir) {
      Write-Host "Clearing Spotify cache..."
      Remove-Item -Path $cacheDir -Recurse -Force -ErrorAction SilentlyContinue
    }
  }
  catch {
    Write-Warning "Could not clear cache: $($_.Exception.Message)"
  }
  
  try {
    # Create a temporary directory for re-application
    $reapplyTempDir = Join-Path $env:TEMP "BlockTheSpot-Reapply-$(Get-Date -UFormat '%Y-%m-%d_%H-%M-%S')"
    New-Item -Type Directory -Path $reapplyTempDir | Out-Null
    
    # Remove existing BlockTheSpot files from Spotify directory to ensure clean re-application
    Write-Host "Removing existing BlockTheSpot files..." -ForegroundColor Yellow
    $filesToRemove = @(
      (Join-Path -Path $spotifyDirectory -ChildPath 'dpapi.dll'),
      (Join-Path -Path $spotifyDirectory -ChildPath 'config.ini'),
      (Join-Path -Path $spotifyDirectory -ChildPath 'blockthespot_settings.json')
    )
    
    foreach ($file in $filesToRemove) {
      if (Test-Path $file) {
        Remove-Item -LiteralPath $file -Force -ErrorAction SilentlyContinue
        Write-Host "Removed: $(Split-Path $file -Leaf)" -ForegroundColor Yellow
      }
    }
    
    # Force re-apply BlockTheSpot patch with fresh files
    Install-BlockTheSpotPatch -SpotifyDirectory $spotifyDirectory -Is64Bit $is64Bit -WorkingDirectory $reapplyTempDir
    
    # Clean up temp directory
    Remove-Item -LiteralPath $reapplyTempDir -Recurse -Force
    
    # Wait before starting Spotify to ensure all patches are applied
    Start-Sleep -Seconds 2
    
    Write-Host "BlockTheSpot re-applied successfully!" -ForegroundColor Green
    Write-Host "Starting Spotify..." -ForegroundColor Green
    Start-Process -WorkingDirectory $spotifyDirectory -FilePath $spotifyExecutable
  }
  catch {
    Write-Warning "Failed to re-apply BlockTheSpot: $($_.Exception.Message)"
    Write-Host "Starting Spotify anyway..." -ForegroundColor Yellow
    Start-Process -WorkingDirectory $spotifyDirectory -FilePath $spotifyExecutable
  }
}
else {
  # Start Spotify normally if no Spicetify was installed
  if (-not $InstallSpicetify) {
    Start-Process -WorkingDirectory $spotifyDirectory -FilePath $spotifyExecutable
  }
}

Write-Host 'Done.'
