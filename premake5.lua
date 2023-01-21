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
workspace "T3DFW_DVR_Workspace"

	-- https://premake.github.io/docs/characterset/
	-- characterset ("MBCS")

	--
	-- setup your solution's configuration here ...
	--
	configurations { "Debug", "Release" }
	platforms { "Win64", "macOS" }

	filter { "platforms:Win64" }
		system "Windows"
		architecture "x86_64"		

	filter { "platforms:macOS" }
		system "macosx"
		architecture "x86_64"		

	filter { "system:Windows" }
		systemversion "latest" -- To use the latest version of the SDK available
		--	systemversion "10.0.10240.0" -- To specify the version of the SDK you want
	
		-- https://stackoverflow.com/questions/40667910/optimizing-a-single-file-in-a-premake-generated-vs-solution
	filter { "configurations:Debug" }
		-- optimize "Debug"
		symbols "On"
		defines { "DEBUG", "TRACE" }
		buildoptions{ "-fdiagnostics-color=always" }
	filter { "configurations:Release" }
		optimize "Speed"
		defines { "NDEBUG" }

	filter {}

	cdialect "C99"
	cppdialect "C++20"

	
	include "t3dfw" -- runs "t3dfw/premake5.lua"

	
	-- main project	
	project "T3DFW_DVR_Project"

		openmp "On" -- ALTERNATIVELY per filter: buildoptions {"-fopenmp"}

		local PRJ_LOCATION = "%{prj.location}/"
		-- local T3DFW_BASE_DIR = "t3dfw/"
		local T3DFW_BASE_DIR = PRJ_LOCATION .. "t3dfw/"
		
		local EXTERNAL_DIR = PRJ_LOCATION .. "external/"
		
		local T3DFW_EXTERNAL_DIR = T3DFW_BASE_DIR .. "external/"
		local T3DFW_SRC_DIR = T3DFW_BASE_DIR .. "src/"

		local GFX_API_DIR = T3DFW_SRC_DIR .. "gfxAPI/"
	
		local NATIVEFILEDIALOG_DIR = T3DFW_EXTERNAL_DIR .. "nativefiledialog/"
		local TINYPROCESS_DIR = EXTERNAL_DIR .. "tiny-process-library/"
		local UTFCPP_DIR = EXTERNAL_DIR .. "utfcpp/"
		local SIMDB_DIR = EXTERNAL_DIR .. "simdb/"
	
		local IMGUI_DIR = T3DFW_EXTERNAL_DIR .. "imgui/"

		local GLFW_BASE_DIR = incorporateGlfw(GFX_API_DIR)
		local GLAD_BASE_DIR = GFX_API_DIR .. "glad/"
		
		
		
		filter {"platforms:Win*", "vs*"}
			defines { "_USE_MATH_DEFINES", "_WIN32", "_WIN64" }

		filter { "platforms:Win*", "action:gmake*", "toolset:gcc" }
			-- NEED TO LINK STATICALLY AGAINST libgomp.a AS WELL: https://stackoverflow.com/questions/30394848/c-openmp-undefined-reference-to-gomp-loop-dynamic-start
			links { 
				"kernel32", 
				"user32", 
				"comdlg32", 
				"advapi32", 
				"shell32", 
				"uuid", 
				"glfw3", -- BEFORE gdi32 AND opengl32
				"gdi32", 
				"opengl32", 
				"Dwmapi", 
				"ole32", 
				"oleaut32", 
				"gomp",
				
				--"bufferoverflowu", -- https://stackoverflow.com/questions/21627607/gcc-linker-error-undefined-reference-to-security-cookie
			}
			-- VS also links these two libs, but they seem to not be necessary... "odbc32.lib" "odbccp32.lib" 
			-- buildoptions {"-fopenmp"} -- NOT NEEDED IF DEFINED AT PROJECT SCOPE AS "openmp "On""
			-- linkoptions {"lgomp"}
			defines { "UNIX", "_USE_MATH_DEFINES" }

			
		filter {}

		filter { "configurations:Debug", "platforms:Win*", "action:gmake*", "toolset:gcc" }		
			buildoptions { "-g" }
			-- https://stackoverflow.com/questions/54969270/how-to-use-gcc-for-compilation-and-debug-on-vscode
			-- buildoptions { "-gdwarf-4 -g3" }
		filter {}

		includedirs { 
			_SCRIPT_DIR,
			PRJ_LOCATION,

			T3DFW_EXTERNAL_DIR, 
			T3DFW_SRC_DIR, 
			
			GLFW_BASE_DIR .. "include/",
			GLAD_BASE_DIR .. "include/", 
			GFX_API_DIR,
			
			NATIVEFILEDIALOG_DIR .. "include/",
			TINYPROCESS_DIR,
			UTFCPP_DIR,
			SIMDB_DIR,
			
			IMGUI_DIR,
		}	
		
		
		shaderincludedirs { "src/shaders" }  
		
		--
		-- setup your project's configuration here ...
		--
		kind "ConsoleApp"
		language "C++"

		-- add files to project
		files { 
			"**.h", 
			"**.hpp", 
			"**.cpp", 
			"**.c", 
			"**.inl", 
			"premake5.lua", 
			"**.bat", 
			"**.glsl",
			"**.frag",
			"**.geom",
			"**.vert",
		}
		removefiles { "t3dfw/**" }
		-- excludes { "t3dfw/**" }
		--removefiles { "externals/tiny-process-library/tests/**" }
		excludes { TINYPROCESS_DIR .. "tests/**" }
		removefiles{ TINYPROCESS_DIR .. "tests/**" }
		
		filter { "platforms:Win*" }
			removefiles { TINYPROCESS_DIR .. "process_unix.cpp", TINYPROCESS_DIR .. "examples.cpp" }
			defines { "_WIN32", "WIN32", "_WIN64", "_AMD64_", "_WINDOWS" }
		filter {}
			removefiles { UTFCPP_DIR .. "tests/**", UTFCPP_DIR .. "samples/**" }
			removefiles { SIMDB_DIR .. "ConcurrentMap.cpp" }
		
		-- https://premake.github.io/docs/defines/
		-- "_MBCS", 
		

		links{ "T3DFW_LIB_Project" }

		-- 
		-- disable filters, so this is valid for all projects
		-- 
		filter {} 


	-- externalproject "TinyProcessProject"
	-- 	location "external/tiny-process-library/tiny-process-library.vcxproj"
	-- 	uuid "9CDC89A2-DEF3-4CAC-83DF-BED879282BC5"
	-- 	kind "StaticLib"
	-- 	language "C++"
	-- 	buildcommands { "echo mkdir", "echo external/tiny-process-library/" }
	-- 	buildinputs { "external/tiny-process-library//CMakeLists.txt" }
	-- 	buildoutputs { "" }
		
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
	end,		
}
 