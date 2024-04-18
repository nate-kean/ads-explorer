::::::::::::::::::::::::::::
::START
::::::::::::::::::::::::::::
@echo on
pushd ..\x64\Debug\
taskkill /f /im explorer.exe
regsvr32 /u ADSExplorer.dll || goto :error
explorer
goto :eof

:error
explorer
echo.
echo An error has occurred.
echo Press any key to exit...
pause > NUL

:eof
