-- main project	
project "T3DFW_CSG_Project"

	filter { "platforms:Win64" }
		architecture "x86_64"

	filter { "platforms:macOS" }
		architecture "x86_64"		

	filter { "system:Windows" }
		systemversion "latest" -- To use the latest version of the SDK available
		-- https://stackoverflow.com/questions/40667910/optimizing-a-single-file-in-a-premake-generated-vs-solution
	filter { "configurations:Debug*" }
		symbols "On"
		targetsuffix "_d"

	filter { "configurations:Release*" }
		optimize "Speed"

	filter {}

	local allDefinesProject = { "_USE_MATH_DEFINES", "CSG_APP" }
	--local allDefinesProject = { "_USE_MATH_DEFINES", "VERBOSE_GFX_DEBUG" }
	
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
		"premake5-CSG.lua", 
		"**.bat", 
		"**.glsl",
		"**.frag",
		"**.geom",
		"**.vert",
		"data/**.labels.json"
	}
	removefiles { "t3dfw/**" }
	-- excludes { "t3dfw/**" }
	removefiles { "**applicationDVR*" }
				

	filter {}
		removefiles { path.join( UTFCPP_DIR, "**/ftest/**" ), path.join( UTFCPP_DIR, "tests/**" ), path.join( UTFCPP_DIR, "samples/**" ) }
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
		  '%{GLSL_LANG_VALIDATOR_BIN_DIR}glslangValidator -I"' ..SHADER_DIR .. '" -E -DVR_MODE=LEVOY_ISO_SURFACE %{file.relpath} > %{file.relpath}.preprocessed',
	   }

	   -- https://premake.github.io/docs/Custom-Build-Commands/
	   -- One or more additional dependencies for this build command (optional)
	   buildinputs { "files:%{file.directory}/*.h.glsl" } -- works

	   -- One or more outputs resulting from the build (required)
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
