set ARGS=-x 1600 -y 1200

@IF "%~1" == "release" GOTO :runRelease
@IF "%~1" == "debug" GOTO :runDebug

:runRelease
bin\Win64\ReleaseMingw\T3DFW_DVR_Project.exe %ARGS%
GOTO exitRun

:runDebug
bin\Win64\DebugMingw\T3DFW_DVR_Project.exe %ARGS%

:exitRun
@exit /b %errorlevel%