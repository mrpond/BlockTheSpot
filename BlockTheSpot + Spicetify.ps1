# BlockTheSpot + Spicetify Installation Script

# Set error action preference
$ErrorActionPreference = "Continue"

# Function to display section headers
function Write-SectionHeader {
    param([string]$Message)
    Write-Host "========================================" -ForegroundColor Blue
    Write-Host " $Message" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Blue
}

# Function to handle errors gracefully
function Invoke-SafeCommand {
    param(
        [scriptblock]$Command,
        [string]$ErrorMessage
    )
    
    try {
        & $Command
    }
    catch {
        Write-Host "$ErrorMessage $($_.Exception.Message)" -ForegroundColor Red
        Read-Host "Press Enter to continue anyway..."
    }
}

# Step 1: Installing BlockTheSpot
Write-SectionHeader "Step 1: Installing BlockTheSpot"

Invoke-SafeCommand -Command {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Write-Host "Downloading and running BlockTheSpot installer..." -ForegroundColor Green
    $installScript = Invoke-RestMethod -Uri "https://raw.githubusercontent.com/mrpond/BlockTheSpot/master/install.ps1" -UseBasicParsing
    Invoke-Expression "& { $installScript } -UninstallSpotifyStoreEdition"
} -ErrorMessage "Error installing BlockTheSpot:"

Write-Host ""

# Step 2: Installing Spicetify CLI
Write-SectionHeader "Step 2: Installing Spicetify CLI"

Invoke-SafeCommand -Command {
    Write-Host "Installing Spicetify CLI..." -ForegroundColor Green
    Invoke-RestMethod -Uri "https://raw.githubusercontent.com/spicetify/spicetify-cli/master/install.ps1" -UseBasicParsing | Invoke-Expression
} -ErrorMessage "Error installing Spicetify CLI:"

Write-Host ""

# Step 3: Configuring Spicetify
Write-SectionHeader "Step 3: Configuring Spicetify"

Invoke-SafeCommand -Command {
    Write-Host "Backing up and configuring Spicetify..." -ForegroundColor Green
    & spicetify backup
    & spicetify config inject_css 1 replace_colors 1
    Write-Host "Spicetify configured successfully!" -ForegroundColor Green
} -ErrorMessage "Error configuring Spicetify:"

Write-Host ""

# Step 4: Installing Spicetify Marketplace
Write-SectionHeader "Step 4: Installing Spicetify Marketplace"

Invoke-SafeCommand -Command {
    Write-Host "Installing Spicetify Marketplace..." -ForegroundColor Green
    Invoke-RestMethod -Uri "https://raw.githubusercontent.com/spicetify/spicetify-marketplace/main/resources/install.ps1" -UseBasicParsing | Invoke-Expression
} -ErrorMessage "Error installing Spicetify Marketplace:"

Write-Host ""

# Step 5: Final Application
Write-SectionHeader "Step 5: Final Application"

Invoke-SafeCommand -Command {
    Write-Host "Applying Spicetify modifications..." -ForegroundColor Green
    & spicetify apply
    Write-Host "Spicetify applied successfully!" -ForegroundColor Green
} -ErrorMessage "Error applying Spicetify:"

Write-Host ""

# Step 6: Re-applying BlockTheSpot
Write-SectionHeader "Step 6: Re-applying BlockTheSpot"

Invoke-SafeCommand -Command {
    Write-Host "Re-applying BlockTheSpot after Spicetify..." -ForegroundColor Green
    
    $SpotifyPath = "$env:APPDATA\Spotify\"
    
    # Stop Spotify processes
    Get-Process -Name "Spotify*" -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
    
    # Create temporary directory
    $tempDir = "$env:TEMP\BTS-$(Get-Date -Format 'HHmmss')"
    New-Item -ItemType Directory -Path $tempDir -Force | Out-Null
    
    # Download and extract BlockTheSpot
    $zipPath = Join-Path $tempDir "chrome_elf.zip"
    Invoke-WebRequest -Uri "https://github.com/mrpond/BlockTheSpot/releases/latest/download/chrome_elf.zip" -OutFile $zipPath -UseBasicParsing
    Expand-Archive -Path $zipPath -DestinationPath $tempDir -Force
    
    # Copy files to Spotify directory
    $dpapiSource = Join-Path $tempDir "dpapi.dll"
    $configSource = Join-Path $tempDir "config.ini"
    $dpapiDest = Join-Path $SpotifyPath "dpapi.dll"
    $configDest = Join-Path $SpotifyPath "config.ini"
    
    Copy-Item -Path $dpapiSource -Destination $dpapiDest -Force
    Copy-Item -Path $configSource -Destination $configDest -Force
    
    Write-Host "BlockTheSpot re-applied successfully!" -ForegroundColor Green
    
    # Clean up temporary directory
    Remove-Item -Path $tempDir -Recurse -Force
    
    # Start Spotify
    $spotifyExe = Join-Path $SpotifyPath "Spotify.exe"
    Start-Process -FilePath $spotifyExe
} -ErrorMessage "Error re-applying BlockTheSpot:"

Write-Host ""

# Step 7: Final Verification Check
Write-SectionHeader "Step 7: Verifying Installation"

function Test-InstallationStatus {
    $verificationResults = @{
        SpotifyInstalled     = $false
        BlockTheSpotFiles    = $false
        SpicetifyInstalled   = $false
        SpicetifyConfigured  = $false
        MarketplaceInstalled = $false
    }
    
    $SpotifyPath = "$env:APPDATA\Spotify\"
    $SpicetifyPath = "$env:APPDATA\spicetify\"
    
    Write-Host "Checking installation status..." -ForegroundColor Green
    
    # Check if Spotify is installed
    $spotifyExe = Join-Path $SpotifyPath "Spotify.exe"
    if (Test-Path $spotifyExe) {
        $verificationResults.SpotifyInstalled = $true
        Write-Host "✓ Spotify found at: $spotifyExe" -ForegroundColor Green
    }
    else {
        Write-Host "✗ Spotify not found at expected location" -ForegroundColor Red
    }
    
    # Check BlockTheSpot files
    $dpapiDll = Join-Path $SpotifyPath "dpapi.dll"
    $configIni = Join-Path $SpotifyPath "config.ini"
    if ((Test-Path $dpapiDll) -and (Test-Path $configIni)) {
        $verificationResults.BlockTheSpotFiles = $true
        Write-Host "✓ BlockTheSpot files found (dpapi.dll, config.ini)" -ForegroundColor Green
    }
    else {
        Write-Host "✗ BlockTheSpot files missing" -ForegroundColor Red
        if (-not (Test-Path $dpapiDll)) { Write-Host "  - Missing: dpapi.dll" -ForegroundColor Yellow }
        if (-not (Test-Path $configIni)) { Write-Host "  - Missing: config.ini" -ForegroundColor Yellow }
    }
    
    # Check if Spicetify is installed
    try {
        $spicetifyVersion = & spicetify --version 2>$null
        if ($spicetifyVersion) {
            $verificationResults.SpicetifyInstalled = $true
            Write-Host "✓ Spicetify CLI installed: $spicetifyVersion" -ForegroundColor Green
        }
    }
    catch {
        Write-Host "✗ Spicetify CLI not found in PATH" -ForegroundColor Red
    }
    
    # Check Spicetify configuration
    $spicetifyConfig = Join-Path $SpicetifyPath "config-xpui.ini"
    if (Test-Path $spicetifyConfig) {
        $configContent = Get-Content $spicetifyConfig -ErrorAction SilentlyContinue
        $injectCss = $configContent | Select-String "inject_css\s*=\s*1"
        $replaceColors = $configContent | Select-String "replace_colors\s*=\s*1"
        
        if ($injectCss -and $replaceColors) {
            $verificationResults.SpicetifyConfigured = $true
            Write-Host "✓ Spicetify properly configured (inject_css=1, replace_colors=1)" -ForegroundColor Green
        }
        else {
            Write-Host "✗ Spicetify configuration incomplete" -ForegroundColor Red
        }
    }
    else {
        Write-Host "✗ Spicetify configuration file not found" -ForegroundColor Red
    }
    
    # Check for Spicetify Marketplace
    $marketplacePath = Join-Path $SpicetifyPath "CustomApps\marketplace"
    if (Test-Path $marketplacePath) {
        $verificationResults.MarketplaceInstalled = $true
        Write-Host "✓ Spicetify Marketplace installed" -ForegroundColor Green
    }
    else {
        Write-Host "✗ Spicetify Marketplace not found" -ForegroundColor Red
    }
    
    return $verificationResults
}

$results = Test-InstallationStatus

# Summary
Write-Host ""
Write-Host "Installation Summary:" -ForegroundColor Cyan
$successCount = ($results.Values | Where-Object { $_ -eq $true }).Count
$totalCount = $results.Count

if ($successCount -eq $totalCount) {
    Write-Host "🎉 All components successfully installed and configured! ($successCount/$totalCount)" -ForegroundColor Green
    Write-Host "You can now enjoy ad-free Spotify with Spicetify customizations!" -ForegroundColor Green
}
elseif ($successCount -ge 3) {
    Write-Host "⚠️  Most components installed successfully ($successCount/$totalCount)" -ForegroundColor Yellow
    Write-Host "Some optional features may not be available, but core functionality should work." -ForegroundColor Yellow
}
else {
    Write-Host "❌ Installation incomplete ($successCount/$totalCount)" -ForegroundColor Red
    Write-Host "Please review the errors above and consider running the script again." -ForegroundColor Red
}

Write-Host ""
Write-Host "Opening Spotify now..." -ForegroundColor Green

# Final Spotify launch (backup in case the previous one failed)
try {
    $spotifyPath = Join-Path $env:APPDATA "Spotify\Spotify.exe"
    Start-Process -FilePath $spotifyPath -ErrorAction SilentlyContinue
}
catch {
    Write-Host "Could not start Spotify automatically. Please start it manually." -ForegroundColor Yellow
}

# Pause equivalent
Write-Host "Press any key to continue..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
