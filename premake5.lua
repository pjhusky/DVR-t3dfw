--
-- run: premake5 vs2022
-- then open the VS2022 solution file
--
-- additional info when using Qt
-- https://github.com/dcourtois/premake-qt
--
--
-- sample invocation for building from the command line:
-- then open VS command prompt from the start menu "x64 Native Tools Command Prompt for VS 2017"
-- msbuild OrientationMatchingWorkspace.sln -target:OrientationMatchingProject -property:Configuration=Release -property:Platform="Win64"
--

--
-- NOTE: on macOS gmake2 doesn't work (moc error)
--    https://github.com/dcourtois/premake-qt/issues/15
--    https://github.com/premake/premake-core/issues/1398
-- for macOS, run: premake5 gmake
-- then, run make config=Release_macos
--
-- NOTE: minGW makefiles:
--
-- > premake5 gmake2
-- > mingw32-make -f Makefile config=release_win64 CC=gcc
--
-- the CC=gcc define is needed to prevent this error: "'cc' is not recognized as an internal or external command"
--

--
-- run: 'premake5 clean' to delete all temporary/generated files
--


--
-- workspace is roughly a VS solution; contains projects
--
workspace "T3DFW_Workspace"

	-- https://premake.github.io/docs/characterset/
	-- characterset ("MBCS")

	--
	-- setup your solution's configuration here ...
	--
	if string.match( _ACTION, 'vs*') then
		configurations { "Debug", "Release" }
	elseif _ACTION == "gmake2" and os.host() == "windows" then
		configurations { "Debug_MinGW", "Release_MinGW" }
	else
		configurations { "Debug_" .. _ACTION, "Release_" .. _ACTION }
	end
	
	if os.host() == "windows" then
		platforms { "Win64" }
	elseif os.host() == "macosx" then
		platforms { "macOS" }
	else
		platforms { "linux" }
	end


	--cdialect "C99"
	cdialect "C17"
	cppdialect "C++20"

	
	-- include "t3dfw" -- runs "t3dfw/premake5.lua"
	include "t3dfw/premake5.lua" -- runs "t3dfw/premake5.lua"
	
	include( "premake5-DVR.lua" )
	include( "premake5-CSG.lua" )


		
-- NOTE: this way we could trigger cleanup code as well:
-- if _ACTION == 'clean' then
-- 	print("clean action DVR!")
-- end
				 
-- https://stackoverflow.com/questions/33307155/how-to-clean-projects-with-premake5
-- Clean Function --
newaction {
	trigger     = "clean",
	description = "clean build artifacts",
	onProject 	= function(prj)
		print("Clean action for project " .. prj.name)
		print("project " .. prj.name .. ", dir: " .. prj.location )
		print("cleaning T3DFW ...")
		os.rmdir( prj.location .. "/obj" )
		os.rmdir( prj.location .. "/bin" )
		os.remove( prj.location .. "/*.sln" )
		os.remove( prj.location .. "/*.vcxproj" )
		os.remove( prj.location .. "/*.vcxproj.*" )
		os.remove( prj.location .. "/src/shaders/*.preprocessed" )
	end,		
}
 