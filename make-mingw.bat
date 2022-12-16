premake5 gmake2

@IF "%~1" == "" GOTO buildRelease
@IF "%~1" == "release_win64" GOTO buildRelease
@IF "%~1" == "release" GOTO buildRelease
@IF "%~1" == "debug_win64" GOTO buildDebug
@IF "%~1" == "debug" GOTO buildDebug
@IF "%~1" == "clean" GOTO cleanRelease

@REM use commandline args passed to this batch script
mingw32-make -f Makefile config=%~1 CC=gcc %~2
@GOTO end

:buildDebug
mingw32-make -f Makefile config=debug_win64 CC=gcc %~2
@GOTO end

:buildRelease
mingw32-make -f Makefile config=release_win64 CC=gcc %~2
@GOTO end

:cleanRelease
mingw32-make -f Makefile config=release_win64 CC=gcc clean
@GOTO end

:end

@exit /b %errorlevel%
