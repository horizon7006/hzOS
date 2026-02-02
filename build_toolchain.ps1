
$ErrorActionPreference = "Stop"

$VD = "c:\Users\Horizon\repos\hzOS"
$MLIBC_SRC = "$VD\submodules\mlibc"
$SYSDEPS_SRC = "$VD\src\lib\sysdeps\hzos"
$SYSDEPS_DEST = "$MLIBC_SRC\sysdeps\hzos"
$BUILD_DIR = "$VD\build\mlibc"
$CROSS_FILE = "$VD\mlibc-crossfile.ini"
$INSTALL_DIR = "$VD\build\sysroot"

# Ensure sysdeps directory exists in mlibc
if (-not (Test-Path $SYSDEPS_DEST)) {
    New-Item -ItemType Directory -Force -Path $SYSDEPS_DEST
}

# Copy sysdeps files
Copy-Item -Path "$SYSDEPS_SRC\*" -Destination $SYSDEPS_DEST -Recurse -Force
Write-Host "Copied sysdeps to $SYSDEPS_DEST"

# Setup Meson
if (-not (Test-Path $BUILD_DIR)) {
    New-Item -ItemType Directory -Force -Path $BUILD_DIR
    Set-Location $MLIBC_SRC
    meson setup $BUILD_DIR --cross-file $CROSS_FILE --prefix=$INSTALL_DIR -Dmaven_repos='[]' -Dheaders_only=false -Ddisable_linux_option=true
}

# Build and Install
Set-Location $BUILD_DIR
ninja
ninja install

Write-Host "mlibc built and installed to $INSTALL_DIR"
