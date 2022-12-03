@echo.
@echo ######### STEP 1 / 6 ############
git submodule foreach --recursive git pull origin main

@echo.
@echo ######### STEP 2 / 6 ############
git pull

@echo.
@echo ######### STEP 3 / 6 ############
@echo checked-out submodules may be in detached HEAD state now - fixing this now!

@REM # create new branch (saving commits made in detached head state)
git submodule foreach --recursive git branch temp-merge

@echo.
@echo ######### STEP 4 / 6 ############
@REM # checkout main (no commits are lost! -- otherwise commits made in detached head state can be seen with 'git reflog'
git submodule foreach --recursive git checkout main

@echo.
@echo ######### STEP 5 / 6 ############
@REM # merge commits made locally in detached head state that are now in temp-merge branch back into main
git submodule foreach --recursive git merge temp-merge

@echo.
@echo ######### STEP 6 / 6 ############
@REM # delete the temp branch after the merge back to main as it is no longer needed
git submodule foreach --recursive git branch --delete temp-merge
