Param(
    [string]$PythonVersion = "3.10",
    [string]$VenvPath = "$PSScriptRoot\..\..\.venv-nsfw",
    [string]$ModelDir = "$PSScriptRoot\..\models",
    [string]$ModelName = "nsfw_mobilenet2.224x224.h5"
)

$ErrorActionPreference = "Stop"

function Ensure-Dir($path) {
    if (-not (Test-Path $path)) {
        New-Item -ItemType Directory -Path $path | Out-Null
    }
}

Write-Host "[NSFW] Creating venv at $VenvPath"
Ensure-Dir $VenvPath

Write-Host "[NSFW] Locating Python $PythonVersion"
$py = & py -$PythonVersion -c "import sys; print(sys.executable)" 2>$null
if (-not $py) {
    Write-Host "[NSFW] Python $PythonVersion not found via 'py'. Trying PATH..."
    $py = & python -c "import sys; print(sys.executable)" 2>$null
}
if (-not $py) { throw "Python $PythonVersion not found. Install it or pass -PythonVersion." }

Write-Host "[NSFW] Using Python: $py"

& $py -m venv $VenvPath

$venvPython = Join-Path $VenvPath "Scripts\python.exe"
if (-not (Test-Path $venvPython)) { throw "Venv python not found: $venvPython" }

Write-Host "[NSFW] Upgrading pip"
& $venvPython -m pip install --upgrade pip

Write-Host "[NSFW] Installing requirements (Windows)"
$req = Join-Path $PSScriptRoot "..\python\requirements.win.txt"
& $venvPython -m pip install -r $req

Ensure-Dir $ModelDir
$modelPath = Join-Path $ModelDir $ModelName
if (-not (Test-Path $modelPath)) {
    Write-Host "[NSFW] Downloading model to $modelPath"
    $url = "https://github.com/GantMan/nsfw_model/releases/download/1.1.0/$ModelName"
    Invoke-WebRequest -Uri $url -OutFile $modelPath
}

Write-Host "[NSFW] Setting user environment variables"
setx KYLIN_NSFW_PYTHON $venvPython | Out-Null
setx KYLIN_NSFW_MODEL $modelPath | Out-Null
setx KYLIN_NSFW_BACKEND "python" | Out-Null

Write-Host "[NSFW] Done. Restart the app to take effect."


