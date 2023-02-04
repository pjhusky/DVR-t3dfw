premake5 gmake2

@REM md external\tiny-process-library
@REM cmake -DCMAKE_INSTALL_PREFIX=external\tiny-process-library\install\Win64_vs2022  -S excmaketernal\tiny-process-library -B external\tiny-process-library\build\Win64_vs2022 -G "Visual Studio 17 2022" -A x64
@REM cmake --build --preset external\tiny-process-library\build\Win64_vs2022


@IF "%~1" == "" GOTO buildRelease
@IF "%~1" == "release_mingw_win64" GOTO buildRelease
@IF "%~1" == "release" GOTO buildRelease
@IF "%~1" == "debug_mingw_win64" GOTO buildDebug
@IF "%~1" == "debug" GOTO buildDebug
@IF "%~1" == "clean" GOTO cleanRelease

@REM use commandline args passed to this batch script
mingw32-make -f Makefile config=%~1 CC=gcc %~2
@GOTO end

:buildDebug
mingw32-make -f Makefile config=debug_mingw_win64 CC=gcc %~2
@GOTO end

:buildRelease
mingw32-make -f Makefile config=release_mingw_win64 CC=gcc %~2
@GOTO end

:cleanRelease
mingw32-make -f Makefile config=release_mingw_win64 CC=gcc clean
@GOTO end

:end

@exit /b %errorlevel%
