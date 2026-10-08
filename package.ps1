param(
    [string]$Version = "1.0.1"
)

$ErrorActionPreference = "Stop"
$Root = $PSScriptRoot
$Bin = Join-Path $Root "Binaries\Release"
$Stage = Join-Path $Root "dist\EveOverlay"
$Zip = Join-Path $Root "dist\EveOverlay-$Version-win64.zip"

if ((Test-Path (Join-Path $Bin "EveOverlay.exe")) -eq $false)
{
    throw "Binaries\Release\EveOverlay.exe not found. Run build.bat first."
}

if (Test-Path (Join-Path $Root "dist"))
{
    Remove-Item (Join-Path $Root "dist") -Recurse -Force
}

New-Item -ItemType Directory -Force (Join-Path $Stage "universeData"), (Join-Path $Stage "sounds") | Out-Null
Copy-Item (Join-Path $Bin "EveOverlay.exe") $Stage
Copy-Item (Join-Path $Root "universeData\systems.csv") (Join-Path $Stage "universeData")
Copy-Item (Join-Path $Root "assets\sounds\defaultWarning.wav") (Join-Path $Stage "sounds")
Copy-Item (Join-Path $Root "LICENSE") $Stage

@"
Eve Overlay $Version

1. Extract this whole folder somewhere you can write to (for example your Documents or Desktop).
   Do not run the program from inside the zip, and avoid C:\Program Files.
2. Double-click EveOverlay.exe.
3. Look for the Eve Overlay icon in the system tray (bottom right, near the clock).

Windows may show a blue "Windows protected your PC" box the first time.
Click "More info", then "Run anyway". This happens because the program is not code-signed.

Full guide: https://github.com/interronator/EveOverlay#readme
"@ | Set-Content (Join-Path $Stage "READ ME FIRST.txt") -Encoding utf8

Compress-Archive -Path $Stage -DestinationPath $Zip
Write-Host "Created $Zip"
