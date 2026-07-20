$ErrorActionPreference = "Stop"

$ProjectDir = Get-Location
$EmsdkDir = Join-Path $ProjectDir "emsdk"

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host " Setting up WebAssembly Environment (emsdk) " -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

if (-not (Test-Path $EmsdkDir)) {
    Write-Host "[1/4] Cloning emsdk repository..." -ForegroundColor Yellow
    git clone https://github.com/emscripten-core/emsdk.git
} else {
    Write-Host "[1/4] emsdk already exists. Updating..." -ForegroundColor Yellow
    cd $EmsdkDir
    git pull
    cd $ProjectDir
}

cd $EmsdkDir
Write-Host "[2/4] Installing latest Emscripten (this may take a few minutes)..." -ForegroundColor Yellow
.\emsdk.bat install latest

Write-Host "[3/4] Activating latest Emscripten..." -ForegroundColor Yellow
.\emsdk.bat activate latest

# Set environment variables for the current session
$env:PATH = "$EmsdkDir;$EmsdkDir\upstream\emscripten;" + $env:PATH

cd $ProjectDir
Write-Host "[4/4] Compiling C++ to WebAssembly..." -ForegroundColor Yellow

emcc snake_wasm.cpp -o snake.js -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" -s EXPORTED_FUNCTIONS="['_malloc','_free','_init_game','_tick','_is_game_over','_get_score','_get_snake_length','_get_snake_body','_get_food_x','_get_food_y']" -O3

Write-Host "==========================================" -ForegroundColor Green
Write-Host " Build Successful! Starting Local Web Server" -ForegroundColor Green
Write-Host " Open http://localhost:8000 in your browser " -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green

python -m http.server 8000
