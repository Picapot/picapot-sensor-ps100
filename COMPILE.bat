@echo off
setlocal



rem Prende il primo file .ino nella cartella (o imposta manualmente SKETCH=...)
set "SKETCH="
for %%F in (*.ino) do if not defined SKETCH set "SKETCH=%%F"

if not defined SKETCH (
  call :printBox Red "Nessun file .ino trovato nella cartella."
  pause
  exit /b 1
)

rem Percorso dell'arduino-cli incluso nell'IDE 2.x (modifica se necessario)
set "CLI=C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"

if not exist "%CLI%" (
  call :printBox Red "Arduino CLI non trovato: %CLI%"
  pause
  exit /b 1
)

rem Cartella dello sketch
for %%I in ("%SKETCH%") do set "SKETCHDIR=%%~dpI"
REM Rimuove l'ultimo backslash
if "%SKETCHDIR:~-1%"=="\" set "SKETCHDIR=%SKETCHDIR:~0,-1%"

rem Legge tutta la riga con //board= e prende solo quello dopo l'uguale
for /f "usebackq tokens=1* delims==" %%A in (`findstr /B /C:"//board=" "%SKETCH%"`) do set "BOARD=%%B"

if not defined BOARD (
  call :printBox Red "Riga //board= non trovata in %SKETCH%."
  pause
  exit /b 1
)

echo FQBN: %BOARD%
echo OutDir: %SKETCHDIR%

"%CLI%" compile --fqbn "%BOARD%" --output-dir "%SKETCHDIR%" "%SKETCHDIR%"
set "EXITCODE=%ERRORLEVEL%"

echo.
if not "%EXITCODE%"=="0" (
  call :printBox Red "=== COMPILATION FAILED - exit code %EXITCODE% ==="
) else (
  call :printBox Green "=== COMPILATION COMPLETED SUCCESSFULLY ==="
)

pause
exit /b %EXITCODE%

:printBox
set "BOXCOLOR=%~1"
set "BOXMESSAGE=%~2"
powershell.exe -NoProfile -Command "$message=$env:BOXMESSAGE; $border='*' * ($message.Length + 4); Write-Host $border -ForegroundColor $env:BOXCOLOR; Write-Host ('* ' + $message + ' *') -ForegroundColor $env:BOXCOLOR; Write-Host $border -ForegroundColor $env:BOXCOLOR"
exit /b 0
