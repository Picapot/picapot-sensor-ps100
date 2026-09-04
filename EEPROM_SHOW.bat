@echo off
cd %CD%

avrdude  -c avrispmkii -p atmega88p -B 10 -F -U eeprom:r:output1.txt:d 

set /p s=<output1.txt
echo %s%

pause




