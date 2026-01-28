#Requires -Version 5.1

<#
.SYNOPSIS
    BlockTheSpotKlim - Direct xpui.spa Patcher for Spotify 1.2.81+

.DESCRIPTION
    Bypasses Spotify 1.2.81's DLL locking by directly patching xpui.spa files.
    Uses signature-based patches from blockthespot_settings.json.

.NOTES
    Author: BlockTheSpotKlim
    Version: 1.0.0
    Spotify Version: 1.2.81.264+
#>

param(
    [switch]$SkipBackup,
    [switch]$Verbose
)

# Error handling
$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# Configuration
$SpotifyPath = "$env:APPDATA\Spotify"
$XpuiSpaPath = "$SpotifyPath\Apps\xpui.spa"
$SettingsPath = "$PSScriptRoot\blockthespot_settings.json"
$BackupDir = "$SpotifyPath\Apps\backups"
$TempDir = "$env:TEMP\xpui-patching-$(Get-Random)"

# Colors for output
function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Write-Success { Write-ColorOutput $args[0] "Green" }
function Write-Error { Write-ColorOutput $args[0] "Red" }
function Write-Warning { Write-ColorOutput $args[0] "Yellow" }
function Write-Info { Write-ColorOutput $args[0] "Cyan" }

# Banner
Clear-Host
Write-ColorOutput @"
╔═══════════════════════════════════════════════════════════╗
║          BlockTheSpotKlim - xpui.spa Patcher             ║
║              Spotify 1.2.81.264+ Support                 ║
╚═══════════════════════════════════════════════════════════╝
"@ "Magenta"
Write-Host ""

# Pre-flight checks
Write-Info "[1/8] Running pre-flight checks..."

if (-not (Test-Path $SpotifyPath)) {
    Write-Error "ERROR: Spotify directory not found at $SpotifyPath"
    exit 1
}

if (-not (Test-Path $XpuiSpaPath)) {
    Write-Error "ERROR: xpui.spa not found at $XpuiSpaPath"
    exit 1
}

if (-not (Test-Path $SettingsPath)) {
    Write-Error "ERROR: blockthespot_settings.json not found at $SettingsPath"
    Write-Warning "Make sure this script is in the BlockTheSpotKlim directory."
    exit 1
}

Write-Success "[OK] All files found"

# Check and stop Spotify
Write-Info "[2/8] Checking for running Spotify processes..."
$spotifyProcesses = Get-Process -Name "Spotify" -ErrorAction SilentlyContinue

if ($spotifyProcesses) {
    Write-Warning "Spotify is running. Stopping processes..."
    $spotifyProcesses | Stop-Process -Force
    Start-Sleep -Seconds 2
    Write-Success "[OK] Spotify stopped"
} else {
    Write-Success "[OK] Spotify not running"
}

# Create backup
if (-not $SkipBackup) {
    Write-Info "[3/8] Creating backup..."

    if (-not (Test-Path $BackupDir)) {
        New-Item -ItemType Directory -Path $BackupDir -Force | Out-Null
    }

    $timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
    $backupPath = "$BackupDir\xpui.spa.backup.$timestamp"
    Copy-Item $XpuiSpaPath $backupPath -Force

    # Keep only last 5 backups
    Get-ChildItem $BackupDir -Filter "xpui.spa.backup.*" |
        Sort-Object LastWriteTime -Descending |
        Select-Object -Skip 5 |
        Remove-Item -Force

    Write-Success "[OK] Backup created: xpui.spa.backup.$timestamp"
} else {
    Write-Warning "[3/8] Skipping backup (not recommended)"
}

# Load settings
Write-Info "[4/8] Loading patch settings..."
try {
    $settings = Get-Content $SettingsPath -Raw | ConvertFrom-Json
    $zipReaderPatches = $settings.'Zip Reader'

    if ($null -eq $zipReaderPatches) {
        throw "Zip Reader section not found in settings"
    }

    $fileCount = ($zipReaderPatches | Get-Member -MemberType NoteProperty).Count
    Write-Success "[OK] Settings loaded: $fileCount files to patch"
} catch {
    Write-Error "ERROR: Failed to parse blockthespot_settings.json: $_"
    exit 1
}

# Extract xpui.spa
Write-Info "[5/8] Extracting xpui.spa..."
try {
    if (Test-Path $TempDir) {
        Remove-Item $TempDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $TempDir -Force | Out-Null

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [System.IO.Compression.ZipFile]::ExtractToDirectory($XpuiSpaPath, $TempDir)

    $fileCount = (Get-ChildItem $TempDir -Recurse -File).Count
    Write-Success "[OK] Extracted $fileCount files"
} catch {
    Write-Error "ERROR: Failed to extract xpui.spa: $_"
    exit 1
}

# Apply patches
Write-Info "[6/8] Applying patches..."
$patchResults = @{
    Success = 0
    Failed = 0
    Skipped = 0
}

foreach ($fileName in ($zipReaderPatches | Get-Member -MemberType NoteProperty | Select-Object -ExpandProperty Name)) {
    $filePath = Join-Path $TempDir $fileName

    if (-not (Test-Path $filePath)) {
        Write-Warning "  [SKIP] $fileName (file not found)"
        $patchResults.Skipped++
        continue
    }

    # Read file content
    $content = [System.IO.File]::ReadAllText($filePath, [System.Text.Encoding]::UTF8)
    $originalContent = $content
    $filePatched = $false

    # Get patches for this file
    $filePatches = $zipReaderPatches.$fileName

    foreach ($patchName in ($filePatches | Get-Member -MemberType NoteProperty | Select-Object -ExpandProperty Name)) {
        $patch = $filePatches.$patchName
        $signature = $patch.Signature
        $value = $patch.Value
        $offset = $patch.Offset

        # Find signature in content
        $index = $content.IndexOf($signature)

        if ($index -ge 0) {
            if ($offset -eq 0) {
                # Replace entire signature
                $content = $content.Replace($signature, $value)
                if ($Verbose) {
                    Write-Host "    [OK] Applied: $patchName" -ForegroundColor DarkGray
                }
            } else {
                # Replace at offset from signature
                $targetIndex = $index + $offset
                if ($targetIndex -lt $content.Length) {
                    $valueLength = $value.Length
                    $before = $content.Substring(0, $targetIndex)
                    $after = $content.Substring($targetIndex + $valueLength)
                    $content = $before + $value + $after

                    if ($Verbose) {
                        Write-Host "    [OK] Applied: $patchName (offset $offset)" -ForegroundColor DarkGray
                    }
                }
            }
            $filePatched = $true
        } else {
            if ($Verbose) {
                Write-Warning "    [WARN] Pattern not found: $patchName"
            }
        }
    }

    # Write back if modified
    if ($filePatched -and $content -ne $originalContent) {
        [System.IO.File]::WriteAllText($filePath, $content, [System.Text.Encoding]::UTF8)
        Write-Success "  [OK] Patched: $fileName"
        $patchResults.Success++
    } else {
        Write-Warning "  [WARN] No changes: $fileName"
        $patchResults.Failed++
    }
}

Write-Host ""
Write-Info "Patch Summary:"
Write-Success "  Success: $($patchResults.Success)"
Write-Warning "  Failed: $($patchResults.Failed)"
Write-Warning "  Skipped: $($patchResults.Skipped)"

# Repack xpui.spa
Write-Info "[7/8] Repacking xpui.spa..."
try {
    Remove-Item $XpuiSpaPath -Force
    [System.IO.Compression.ZipFile]::CreateFromDirectory($TempDir, $XpuiSpaPath, 'Optimal', $false)

    $newSize = [math]::Round((Get-Item $XpuiSpaPath).Length / 1MB, 2)
    Write-Success "[OK] xpui.spa repacked ($newSize MB)"
} catch {
    Write-Error "ERROR: Failed to repack xpui.spa: $_"
    Write-Warning "Attempting to restore from backup..."

    if (-not $SkipBackup) {
        $latestBackup = Get-ChildItem $BackupDir -Filter "xpui.spa.backup.*" |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1

        if ($latestBackup) {
            Copy-Item $latestBackup.FullName $XpuiSpaPath -Force
            Write-Success "[OK] Restored from backup"
        }
    }
    exit 1
}

# Cleanup
Write-Info "[8/8] Cleaning up..."
if (Test-Path $TempDir) {
    Remove-Item $TempDir -Recurse -Force
}
Write-Success "[OK] Temporary files removed"

# Final message
Write-Host ""
Write-ColorOutput @"
╔═══════════════════════════════════════════════════════════╗
║                   Patching Complete!                     ║
╚═══════════════════════════════════════════════════════════╝
"@ "Green"

Write-Host ""
Write-Info "Next steps:"
Write-Host "  1. Launch Spotify"
Write-Host "  2. Play some songs to verify ads are blocked"
Write-Host "  3. Check that premium buttons are hidden"
Write-Host ""
Write-Warning "Note: You'll need to re-run this patcher after Spotify updates."
Write-Host ""

# Ask to launch Spotify
$launch = Read-Host "Launch Spotify now? (Y/N)"
if ($launch -eq "Y" -or $launch -eq "y") {
    Start-Process "$SpotifyPath\Spotify.exe"
    Write-Success "[OK] Spotify launched"
}
