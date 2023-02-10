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
	--configurations { "Debug_" .. _ACTION, "Release_" .. _ACTION }
	--configurations { "Debug" .. (if _ACTION == "vs*" then "" else "_" .. _ACTION end), "Release" .. (if _ACTION == "vs*" then "" else "_" .. _ACTION end) }
	if string.match( _ACTION, 'vs*') then
		configurations { "Debug", "Release" }
	elseif _ACTION == "gmake2" and os.host() == "windows" then
		configurations { "Debug_MinGW", "Release_MinGW" }
	else
		configurations { "Debug_" .. _ACTION, "Release_" .. _ACTION }
	end
	
	--platforms { "Win64", "macOS" }
	if os.host() == "windows" then
		platforms { "Win64" }
	elseif os.host() == "macosx" then
		platforms { "macOS" }
	else
		platforms { "linux" }
	end

	filter { "platforms:Win64" }
		-- system "Windows"
		architecture "x86_64"

	filter { "platforms:macOS" }
		-- system "macosx"
		architecture "x86_64"		

	--filter { "system:macosx" }
	--	removeplatforms { "Win64" }

	filter { "system:Windows" }
		systemversion "latest" -- To use the latest version of the SDK available
		--	systemversion "10.0.10240.0" -- To specify the version of the SDK you want
		-- removeplatforms { "macOS" }
		-- https://stackoverflow.com/questions/40667910/optimizing-a-single-file-in-a-premake-generated-vs-solution
	filter { "configurations:Debug*" }
		-- optimize "Debug"
		symbols "On"
		--defines { "DEBUG", "TRACE" }

		--buildoptions{ "-fdiagnostics-color=always" }
		targetsuffix "_d"

	filter { "configurations:Release*" }
		optimize "Speed"
		--defines { "NDEBUG" }

	filter {}

	cdialect "C99"
	cppdialect "C++20"

	
	include "t3dfw" -- runs "t3dfw/premake5.lua"
	
	-- main project	
	project "T3DFW_DVR_Project"

		--filter { "system:macosx" }
		--	removeplatforms { "Win64" }
		--filter { "system:Windows" }
		--	removeplatforms { "macOS" }

		local allDefinesProject = { "_USE_MATH_DEFINES" }
		if _ACTION == "gmake2" then
			table.insert( allDefinesProject, "UNIX" )
		elseif string.match( _ACTION, 'vs*') then
			table.insert( allDefinesProject, "_WIN32" )
			table.insert( allDefinesProject, "WIN32" )
			table.insert( allDefinesProject, "_WIN64" )
			table.insert( allDefinesProject, "_AMD64_" )
			table.insert( allDefinesProject, "_WINDOWS" )
			table.insert( allDefinesProject, "_CRT_SECURE_NO_WARNINGS" )
		end


		openmp "On" -- ALTERNATIVELY per filter: buildoptions {"-fopenmp"}

		local PRJ_LOCATION = path.normalize( "%{prj.location}" )
		local T3DFW_BASE_DIR = path.normalize( path.join( PRJ_LOCATION, "t3dfw" ) )
		
		local EXTERNAL_DIR = path.normalize( path.join( PRJ_LOCATION, "external" ) )
		
		local T3DFW_EXTERNAL_DIR = path.normalize( path.join( T3DFW_BASE_DIR, "external" ) )
		local T3DFW_SRC_DIR = path.normalize( path.join( T3DFW_BASE_DIR, "src" ) )

		local GFX_API_DIR = path.normalize( path.join( T3DFW_SRC_DIR, "gfxAPI" ) )
	
		local NATIVEFILEDIALOG_DIR = path.normalize( path.join( T3DFW_EXTERNAL_DIR, "nativefiledialog" ) )
		local TINYPROCESS_DIR = path.normalize( path.join( T3DFW_EXTERNAL_DIR, "tiny-process-library" ) )
		local UTFCPP_DIR = path.normalize( path.join( EXTERNAL_DIR, "utfcpp" ) )
		local SIMDB_DIR = path.normalize( path.join( EXTERNAL_DIR, "simdb" ) )

		-- TODO: move this to T3DFW framework
		local GLSL_LANG_VALIDATOR_DIR = path.normalize( path.join( EXTERNAL_DIR, "glslang-master-windows-x64-Release" ) )
		local GLSL_LANG_VALIDATOR_BIN_DIR = path.normalize( path.join( GLSL_LANG_VALIDATOR_DIR, "bin" ) )
	
		local IMGUI_DIR = path.normalize( path.join( T3DFW_EXTERNAL_DIR, "imgui" ) )

		local GLFW_BASE_DIR = incorporateGlfw(GFX_API_DIR)
		local GLAD_BASE_DIR = path.normalize( path.join( GFX_API_DIR, "glad" ) )
		
		local SHADER_DIR = path.normalize( path.join( PRJ_LOCATION, "shaders" ) )

		filter {"platforms:Win*", "vs*"}
			--defines { "_USE_MATH_DEFINES", "_WIN32", "_WIN64", "_CRT_SECURE_NO_WARNINGS" }

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
			
			--defines { "UNIX", "_USE_MATH_DEFINES" }

			--undefines { "__STRICT_ANSI__" } -- for simdb wvsprintfA() - https://stackoverflow.com/questions/3445312/swprintf-and-vswprintf-not-declared
			--cppdialect "gnu++20" -- https://github.com/assimp/assimp/issues/4190, https://premake.github.io/docs/cppdialect/

			
		filter {}

		filter { "configurations:Debug*", "platforms:Win*", "action:gmake*", "toolset:gcc" }		
			buildoptions { "-g" }
			-- https://stackoverflow.com/questions/54969270/how-to-use-gcc-for-compilation-and-debug-on-vscode
			-- buildoptions { "-gdwarf-4 -g3" }
		
		filter { "platforms:Win*", "action:gmake*", "toolset:gcc" }	
			--buildoptions { "-fpermissive" }

		filter {}

		

		includedirs { 
			_SCRIPT_DIR,
			PRJ_LOCATION,

			T3DFW_EXTERNAL_DIR, 
			T3DFW_SRC_DIR, 
			
			path.normalize( path.join( GLFW_BASE_DIR, "include" ) ),
			path.normalize( path.join( GLAD_BASE_DIR, "include" ) ), 
			GFX_API_DIR,
			
			path.normalize( path.join( NATIVEFILEDIALOG_DIR, "include" ) ),
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
					

		filter {}
			removefiles { path.join( UTFCPP_DIR, "tests/**" ), path.join( UTFCPP_DIR, "samples/**" ) }
			removefiles { path.join( SIMDB_DIR, "ConcurrentMap.cpp" ) }
		
		-- https://premake.github.io/docs/defines/
		-- "_MBCS", 
		
		-- https://premake.github.io/docs/buildcommands/
		--filter 'files:**.glsl'
		filter "files:**.vert.glsl or files:**.geom.glsl or files:**.frag.glsl"
		   -- A message to display while this build step is running (optional)
		   buildmessage 'Compiling Shader %{file.relpath}'

		   -- One or more commands to run (required)
		   buildcommands {
			  -- 'echo blah2.2',
			  -- 'echo ' .. SHADER_DIR,
			  -- 'echo blah3',
			  '%{GLSL_LANG_VALIDATOR_BIN_DIR}glslangValidator -I"' ..SHADER_DIR .. '" -E %{file.relpath} > %{file.relpath}.preprocessed',
			  --'%{GLSL_LANG_VALIDATOR_BIN_DIR}glslangValidator -I"' ..SHADER_DIR .. '" --target-env opengl --vn %{file.relpath}.c %{file.relpath}',
		   }

		   -- One or more outputs resulting from the build (required)
		   -- buildoutputs { '%{cfg.objdir}/%{file.basename}.preprocessed' }
		   buildoutputs { '%{file.relpath}.preprocessed' }
		   

		--
		-- apply defines
		-- 
		filter { "configurations:Debug*" }
			defines { allDefinesProject, "DEBUG", "TRACE" }
		filter { "configurations:Release*" }
			defines { allDefinesProject, "NDEBUG" }

		-- 
		-- disable filters, so this is valid for all projects
		-- 
		filter {} 

		links{ "T3DFW_LIB_Project" }

		
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
 