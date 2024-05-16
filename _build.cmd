@cls
@set PATH=%PATH%;C:\MinGW\bin
gcc.exe -g ld.c -o didYouMean.exe -O3
@echo exit code %errorlevel%
didYouMean.exe "NAMES" "FNAME" "JACOB" 2 "result12898.dbf"
