premake5 gmake2

@IF "%~1" == "" GOTO buildRelease
@IF "%~1" == "release_win64" GOTO buildRelease

@IF "%~1" == "debug_win64" GOTO buildDebug

@REM use commandline args passed to this batch script
mingw32-make -f Makefile config=%~1 CC=gcc
@GOTO end

:buildDebug
mingw32-make -f Makefile config=debug_win64 CC=gcc
@GOTO end

:buildRelease
mingw32-make -f Makefile config=release_win64 CC=gcc
@GOTO end

:end
