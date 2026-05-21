@echo off
setlocal enabledelayedexpansion
echo ========================================
echo   NAU DX - Tape Version Builder
echo ========================================

set PROJECT_DIR=C:\msxgl\projects\nau_dx
set OUT_DIR=%PROJECT_DIR%\out
set TAPE_DIR=%PROJECT_DIR%\tape

echo.
echo [1/5] Building Game Binary...
cd /d %PROJECT_DIR%
copy /y project_config_game.js project_config.js >nul
call build.bat >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Game build failed!
    pause
    exit /b 1
)
echo     Game built successfully.

echo.
echo [2/5] Converting Game to Binary...
"C:\MSXgl\tools\MSXtk\bin\MSXhex" %OUT_DIR%\nau_dx.ihx -e bin -s 0x4000 -o %OUT_DIR%\nau_dx_game.bin >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Game conversion failed!
    pause
    exit /b 1
)
echo     Game binary created.

echo.
echo [3/5] Compressing Game...
powershell -ExecutionPolicy Bypass -File %TAPE_DIR%\compress_rle.ps1 -InputFile "%OUT_DIR%\nau_dx_game.bin" -OutputFile "%TAPE_DIR%\game_compressed.bin"
if %errorlevel% neq 0 (
    echo ERROR: Game compression failed!
    pause
    exit /b 1
)
echo     Game compressed.

echo.
echo [4/5] Compiling Loader...
cd /d %TAPE_DIR%
del /q loader.rel loader.ihx loader.bin loader.asm loader.lst loader.sym loader.lk >nul 2>&1
"C:\MSXgl\tools\sdcc\bin\sdcc" -c -mz80 --opt-code-speed loader.c -o loader.rel >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Loader compilation failed!
    pause
    exit /b 1
)
"C:\MSXgl\tools\sdcc\bin\sdcc" -mz80 --no-std-crt0 --code-loc 0x8000 --data-loc 0xD000 loader.rel -o loader.ihx >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Loader linking failed!
    pause
    exit /b 1
)
"C:\MSXgl\tools\MSXtk\bin\MSXhex" loader.ihx -e bin -s 0x8000 -o loader.bin >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Loader conversion failed!
    pause
    exit /b 1
)
echo     Loader compiled.

echo.
echo [5/5] Creating Tape Binary...
:: Create magic marker
echo -ne '\xDE\xAD\xBE\xEF' > "%TAPE_DIR%\magic.bin"
:: Concatenate Loader + Magic + Compressed Data
copy /b loader.bin + magic.bin + game_compressed.bin %OUT_DIR%\nau_dx_tape_loader.bin >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Binary concatenation failed!
    pause
    exit /b 1
)
echo     Tape binary created: %OUT_DIR%\nau_dx_tape_loader.bin

echo.
echo [6/6] Converting to CAS...
py "%TAPE_DIR%\bin2cas.py" "%OUT_DIR%\nau_dx_tape_loader.bin" "%TAPE_DIR%\nau_dx_tape.cas" "NAU_DX" 0xC000 0xC000
if !errorlevel! neq 0 (
    python "%TAPE_DIR%\bin2cas.py" "%OUT_DIR%\nau_dx_tape_loader.bin" "%TAPE_DIR%\nau_dx_tape.cas" "NAU_DX" 0xC000 0xC000
    if !errorlevel! neq 0 (
        echo ERROR: CAS conversion failed!
        pause
        exit /b 1
    )
)
echo     CAS file created: %TAPE_DIR%\nau_dx_tape.cas

echo.
echo ========================================
echo   Tape build complete!
echo   Output: %TAPE_DIR%\nau_dx_tape.cas
echo   Load with: BLOAD"CAS:",R
echo ========================================
pause
