@echo off
setlocal enabledelayedexpansion
echo ========================================
echo   NAU DX - Tape Version Builder (Direct)
echo ========================================

set PROJECT_DIR=C:\msxgl\projects\nau_dx
set OUT_DIR=%PROJECT_DIR%\out
set TAPE_DIR=%PROJECT_DIR%\tape

echo.
echo [1/4] Building Game Binary...
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
echo [2/4] Converting Game to Binary...
"C:\MSXgl\tools\MSXtk\bin\MSXhex" %OUT_DIR%\nau_dx.ihx -e bin -s 0x4000 -o %OUT_DIR%\nau_dx_game.bin >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Game conversion failed!
    pause
    exit /b 1
)
echo     Game binary created.

echo.
echo [3/4] Patching game binary for tape...
C:\Users\esquerra\AppData\Local\Programs\Python\Python312\python.exe -c "d=bytearray(open('%OUT_DIR:\=\\%\\nau_dx_game.bin','rb').read());d[0x3E:0x41]=[0,0,0];open('%OUT_DIR:\=\\%\\nau_dx_game_tape.bin','wb').write(d);print('Patched CALL ENASLT -> NOPs')"
if %errorlevel% neq 0 (
    echo ERROR: Patching failed!
    pause
    exit /b 1
)
echo     Game patched for tape (ENASLT skipped).

echo.
echo [4/4] Creating CAS (Direct Load to 0x4000)...
py "%TAPE_DIR%\bin2cas.py" "%OUT_DIR%\nau_dx_game_tape.bin" "%TAPE_DIR%\nau_dx_tape.cas" "NAU_DX" 0x4000 0x4014
if !errorlevel! neq 0 (
    python "%TAPE_DIR%\bin2cas.py" "%OUT_DIR%\nau_dx_game_tape.bin" "%TAPE_DIR%\nau_dx_tape.cas" "NAU_DX" 0x4000 0x4014
    if !errorlevel! neq 0 (
        echo ERROR: CAS conversion failed!
        pause
        exit /b 1
    )
)
echo     CAS file created: %TAPE_DIR%\nau_dx_tape.cas

echo.
echo [5/4] Creating WAV (for real hardware)...
C:\Users\esquerra\AppData\Local\Programs\Python\Python312\python.exe "%TAPE_DIR%\cas2wav.py" "%OUT_DIR%\nau_dx_game_tape.bin" "%TAPE_DIR%\nau_dx_tape.wav" "NAU_DX" 0x4000 0x4014
if %errorlevel% neq 0 (
    echo WARNING: WAV conversion failed!
)
echo     WAV file created: %TAPE_DIR%\nau_dx_tape.wav

echo.
echo ========================================
echo   Tape build complete!
echo   Outputs:
echo     %TAPE_DIR%\nau_dx_tape.cas (openMSX)
echo     %TAPE_DIR%\nau_dx_tape.wav (real HW)
echo   Load: 0x4000, Exec: 0x4014
echo   Load with: BLOAD"CAS:",R
echo ========================================
pause
