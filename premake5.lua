workspace "YamenC3Tools"
    architecture "x64"
    configurations { "Debug", "Release" }
    startproject "YamenC3Tools"
    
    outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    
    IncludeDir = {}
    IncludeDir["ImGui"] = "third_party/imgui"
    IncludeDir["json"] = "third_party/json/single_include"
    IncludeDir["glm"] = "third_party/glm"
    IncludeDir["stb"] = "third_party/stb"
    IncludeDir["tinyobj"] = "third_party/tinyobjloader"

project "YamenC3Tools"
    location "YamenC3Tools"
    kind "WindowedApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
    
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
        "third_party/imgui/imgui.cpp",
        "third_party/imgui/imgui_demo.cpp",
        "third_party/imgui/imgui_draw.cpp",
        "third_party/imgui/imgui_tables.cpp",
        "third_party/imgui/imgui_widgets.cpp",
        "third_party/imgui/backends/imgui_impl_win32.cpp",
        "third_party/imgui/backends/imgui_impl_dx11.cpp",
    }
    
    includedirs {
		"%{prj.name}/src/**.h",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.ImGui}/backends",
        "%{IncludeDir.json}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb}",
        "%{IncludeDir.tinyobj}"
    }
    
    links {
        "d3d11.lib",
        "dxgi.lib",
        "d3dcompiler.lib",
        "dxguid.lib",
        "Comdlg32.lib"
    }
    
    defines {
        "_CRT_SECURE_NO_WARNINGS",
        "NOMINMAX",
        "WIN32_LEAN_AND_MEAN"
    }
    
    filter "system:windows"
        systemversion "latest"
        defines { "PLATFORM_WINDOWS" }
    
    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "on"
        optimize "off"
    
    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        optimize "speed"
        inlining "auto"