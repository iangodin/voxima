# PowerShell equivalent of render.sh.
# Renders the "movie" scene. Extra arguments go to sonimation, so a single scene (or several, in order) can be
# rendered instead, and the ones left out of the movie can be reached:
#     .\render.ps1 --scene missing-glyphs
#     .\render.ps1 --scene morph-shapes --scene karaoke
#     build\Release\sonimation.exe --list-scenes

$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Force -Path "render" | Out-Null
foreach ($pattern in "*.wav", "*.flac", "*.png", "*.svg", "*.mp4", "*.mkv", "*.webm") {
    Remove-Item (Join-Path "render" $pattern) -ErrorAction SilentlyContinue
}

make
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& .\build\Release\sonimation.exe --width 1280 --height 720 `
    --script-dir demos/eases `
    --font-dir assets/fonts `
    --render-dir render `
    --save-svg `
    --container mkv `
    --vcodec h264 `
	--theme "Academic Classic" `
    --acodec opus `
    --lang en `
    @args
exit $LASTEXITCODE
