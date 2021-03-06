workspace "eden"
    architecture "x64"
    startproject "eden"
    
    configurations
    {
        "Debug",
        "Release"
    }

	targetdir ("bin")
    objdir ("bin")
    debugdir ("bin")

	include "libs/ch_stl"
	include "libs/lua"


project "eden"
    language "C++"
	dependson { "ch_stl", "lua" }
	kind "WindowedApp"

	defines
	{
		"_CRT_SECURE_NO_WARNINGS"
	}

    files
    {
        "src/*.h",
        "src/*.cpp",
    }

    includedirs
    {
        "src/**",
        "libs/",
    }

    links
    {
        "opengl32",
        "user32",
        "kernel32",
		"shlwapi",
		"bin/ch_stl",
		"bin/lua"
    }

    filter "configurations:Debug"
		defines 
		{
			"BUILD_DEBUG#1",
			"BUILD_RELEASE#0",
			"CH_BUILD_DEBUG#1"
		}
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		defines 
		{
			"BUILD_RELEASE#1",
			"BUILD_DEBUG#0",
			"NDEBUG"
		}
		runtime "Release"
        optimize "On"
        
    filter "system:windows"
        cppdialect "C++17"
		systemversion "latest"
		architecture "x64"

		defines
		{
			"PLATFORM_WINDOWS#1",
        }

		files 
		{
			"src/win32/**.h",
			"src/win32/**.cpp",
			"src/win32/**.rc"
		}