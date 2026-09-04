@echo off
SETLOCAL EnableDelayedExpansion
cd %CD%
echo PICAPOT SENSOR CALIBRATION (ATMEGA88) 

::-b baudrate
::-B bitclock
::-F disable verify signature
::-l logfile
::-s always recover fuse errors
::-q disable progress bar
::-e erase chip
::-u make fuse change persistent
::-V Disable automatic verify
::-v enable verbose output

IF EXIST log.txt del log.txt
IF EXIST log2.txt del log2.txt
IF EXIST log3.txt del log3.txt
IF EXIST log4.txt del log4.txt
IF EXIST log5.txt del log5.txt
IF EXIST log6.txt del log6.txt
IF EXIST log7.txt del log7.txt
IF EXIST output1.txt del output1.txt
IF EXIST output2.txt del output2.txt


avrdude -c avrispmkii -p atmega88p -B 100 -e -q -u -s -U lfuse:w:0xE2:m -U hfuse:w:0xDD:m -U efuse:w:0xFF:m -l log.txt
for /f "Tokens=* Delims=" %%x in (log.txt) do set res0=!res0!%%x
echo %res0% | findstr /c:"Could not find avrispmkii device" > nul
if %errorlevel% == 0 (
	echo.
	call :printBox Red "=== ERROR: Could not find avrispmkii device ==="
	pause
	exit /b 1
)


avrdude -c avrispmkii -p atmega88p -B 4 -e -q -u -s -V -U flash:w:picapot-sensor-ps100-calib.ino.hex:i -l log2.txt
for /f "Tokens=* Delims=" %%x in (log2.txt) do set res1=!res1!%%x
echo %res1% | findstr /c:"bytes of flash written" > nul
if %errorlevel% NEQ 0 (
	echo.
	call :printBox Red "=== ERROR: Writing sampling program to flash failed ==="
	pause
	exit /b 1
)


TIMEOUT 5 /nobreak
avrdude -c avrispmkii -p atmega88p -B 10 -q -U eeprom:r:output1.txt:d -l log3.txt
set /p s=<output1.txt
for /f "tokens=1 delims=," %%G IN ("%s%") DO (set w=%%G)
echo.
echo CALCULATED CALIBRATION: [%w%]

if %w% LSS 1 (
	echo.
	call :printBox Red "=== ERROR: Calibration out of bounds due to timeout ==="
	pause
	exit /b 1
) 


if %w% GTR 254 (
	echo.
	call :printBox Red "=== ERROR: Calibration out of bounds ==="
	pause
	exit /b 1
) 

avrdude -c avrispmkii -p atmega88p -B 4 -u -q -s -V -U hfuse:w:0xD5:m -l log4.txt
for /f "Tokens=* Delims=" %%x in (log4.txt) do set res4=!res4!%%x
echo %res4% | findstr /c:"Could not find avrispmkii device" > nul
if %errorlevel% == 0 (
	echo.
	call :printBox Red "=== ERROR: Could not find avrispmkii device ==="
	pause
	exit /b 1
)


avrdude -c avrispmkii -p atmega88p -B 4 -u -q -s -V -U flash:w:picapot-sensor-ps100.ino.hex:i -l log5.txt
for /f "Tokens=* Delims=" %%x in (log5.txt) do set res2=!res2!%%x
echo %res2% | findstr /c:"bytes of flash written" > nul
if %errorlevel% NEQ 0 (
	echo.
	call :printBox Red "=== ERROR: Writing sensor program to flash failed ==="
	pause
	exit /b 1
) 


avrdude -c avrispmkii -p atmega88p -B 10 -q -U eeprom:r:output2.txt:d -l log6.txt
set /p s=<output2.txt
for /f "tokens=1 delims=," %%G IN ("%s%") DO (set ww=%%G)
echo CONFIRMED CALIBRATION:  [%ww%]
if %w% NEQ %ww% (
	echo.
	call :printBox Red "=== ERROR: EEPROM calibration confirmation failed ==="
	pause
	exit /b 1
)




echo.
call :printBox Green "=== CALIBRATION AND UPLOAD COMPLETED SUCCESSFULLY ==="
pause
exit /b 0


:printBox
set "BOXCOLOR=%~1"
set "BOXMESSAGE=%~2"
powershell.exe -NoProfile -Command "$message=$env:BOXMESSAGE; $border='*' * ($message.Length + 4); Write-Host $border -ForegroundColor $env:BOXCOLOR; Write-Host ('* ' + $message + ' *') -ForegroundColor $env:BOXCOLOR; Write-Host $border -ForegroundColor $env:BOXCOLOR"
exit /b 0


