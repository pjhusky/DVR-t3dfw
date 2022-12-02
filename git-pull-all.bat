git submodule foreach --recursive git pull origin main
git pull

@echo checked-out submodules may be in detached HEAD state now - fix this!

@REM # create new branch (saving commits made in detached head state)
git submodule foreach --recursive git branch temp-merge

@REM # checkout main (no commits are lost! -- otherwise commits made in detached head state can be seen with 'git reflog'
git submodule foreach --recursive git checkout main

@REM # merge commits made locally in detached head state that are now in temp-merge branch back into main
git submodule foreach --recursive git merge temp-merge

@REM # delete the temp branch after the merge back to main as it is no longer needed
git submodule foreach --recursive git branch --delete temp-merge
