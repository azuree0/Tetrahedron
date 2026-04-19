# Copy SFML 3 runtime DLLs next to run.exe after a Release build.
$sfmlBin = "C:\SFML\bin"
$targetDir = ".\build\Release"

if (-not (Test-Path $targetDir)) {
    Write-Host "Error: Release directory not found at $targetDir"
    Write-Host "Build the project first (cmake --build build --config Release)."
    exit 1
}

if (-not (Test-Path $sfmlBin)) {
    Write-Host "Error: SFML bin directory not found at $sfmlBin"
    Write-Host "Install SFML or edit this script to match your SFML path."
    exit 1
}

Write-Host "Copying SFML DLLs from $sfmlBin to $targetDir..."
Copy-Item "$sfmlBin\sfml-*.dll" -Destination $targetDir -Force

Write-Host "`nDLLs copied:"
Get-ChildItem "$targetDir\sfml-*.dll" | ForEach-Object { Write-Host "  - $($_.Name)" }

Write-Host "`nRun: .\build\Release\run.exe"
