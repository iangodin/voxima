# render.ps1
# PowerShell equivalent of render.sh

$ErrorActionPreference = "Stop"

# Create render directory if it doesn't exist
if (-not (Test-Path "render")) {
    New-Item -ItemType Directory -Path "render" | Out-Null
}

# Clean up existing render files
Write-Host "Cleaning up render directory..." -ForegroundColor Cyan
$filesToCleanup = @(
    "render\*.wav", "render\*.flac", "render\*.png", "render\*.svg", 
    "render\*.mp4", "render\*.mkv", "render\*.webm",
    "render\frame*.svg", "render\frame*.png"
)
foreach ($pattern in $filesToCleanup) {
    Remove-Item $pattern -ErrorAction SilentlyContinue
}

# Build the project
Write-Host "Building project..." -ForegroundColor Cyan
cmake --build build --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit $LASTEXITCODE
}

$exePath = ".\build\Release\sonimation.exe"

$params = @(
    "--width", "1920",
    "--height", "1080",
    "--script-dir", "demos/eases",
    "--font-dir", "assets/fonts",
    "--render-dir", "render",
    "--theme", "Playful Discovery",
    "--container", "mkv",
    "--vcodec", "h264",
    "--acodec", "opus",
    "--lang", "en",
    "--lang", "fr"
)

# Append any additional arguments passed to the script
$params += $args

if (Test-Path $exePath) {
    Write-Host "Running sonimation..." -ForegroundColor Cyan
    & $exePath @params
} else {
    Write-Error "Executable not found at $exePath. Please ensure the build succeeded."
    exit 1
}
