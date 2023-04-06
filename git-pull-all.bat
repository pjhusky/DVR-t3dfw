@echo.
@echo ######### START REPO UPDATE ###########

@REM git checkout main
git pull


@echo.
@echo ######### STEP 1 / 5 ############
git submodule foreach --recursive git submodule update --init --remote --merge

@echo.
@echo ######### STEP 2 / 5 ############
@echo checked-out submodules may be in detached HEAD state now - fixing this now!
git submodule foreach --recursive git branch temp-merge

@echo.
@echo ######### STEP 3 / 5 ############
@REM git submodule foreach --recursive git stash
@REM git submodule foreach --recursive git checkout (master 2>/dev/null || git checkout main)
@REM git submodule foreach --recursive "git checkout master ; git checkout main ; echo done"
@REM 2>/dev/null redirects error msg to null device, since we are fine with master not-being-found errors
git submodule foreach --recursive "git checkout master 2>/dev/null || git checkout main ; echo done"
@REM git submodule foreach --recursive git stash pop

@echo.
@echo ######### STEP 4 / 5 ############
@REM # merge commits made locally in detached head state that are now in temp-merge branch back into main
git submodule foreach --recursive git merge temp-merge

@echo.
@echo ######### STEP 5 / 5 ############
@REM # delete the temp branch after the merge back to main as it is no longer needed
git submodule foreach --recursive git branch --delete temp-merge

git submodule foreach --recursive "git pull origin master 2>/dev/null || git pull origin main ; echo done"

@echo.
@echo ########## SYNC SUBMODULE URLs ############
git submodule sync --recursive


@echo.
@echo #########  REPO UP-TO-DATE  ###########
@echo.

