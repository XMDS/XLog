-- workspace: 工作环境
workspace "XLog" -- Visual Studio中的解决方案
    location "../build" -- 解决方案文件夹
    objdir "../build/obj" -- obj目录

    configurations { -- 解决方案配置项, 默认的配置
        "Debug", 
        "Release" 
    }
    
    platforms { -- 设置平台
        "ARM",
        "ARM64"
    }

    filter "configurations:Debug" -- debug模式的设置
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release" -- release模式的设置
        defines { "NDEBUG" }
        symbols "Off"
        optimize "Full"
        
project "XLog" -- 项目名称
    system "android"
    language "C++"
    cdialect "C11"
    cppdialect "C++latest"
    characterset "MBCS"
    staticruntime "On"
    toolset "clang"
    kind "SharedLib" --编译为动态共享库
    
    files { -- 项目文件
        "../src/**",
        "../../ARMHook/CHook.cpp",
        "../ini/**",
        "../../ARMHook/CHook.h",
        "../../ARMHook/Assembly/**"
    }

    includedirs { -- 头文件目录（实际是包含目录）
        "../../ARMHook",
        "../ini"
    }

    filter "platforms:ARM" -- arm模式的配置
        architecture "ARM"
        targetdir "../build/ARM" -- 生成的动态库文件的路径
        libdirs "../../ARMHook/Library" -- 外部库文件的路径
        links { "CydiaSubstrateInlineHook", "xDL" } -- 链接外部库文件
    
    filter "platforms:ARM64" -- arm64模式的配置
        targetname "XLog64"
        architecture "ARM64"
        targetdir "../build/ARM64" -- 生成的动态库文件的路径
        libdirs "../../ARMHook/Library" -- 外部库文件的路径
        links { "And64InlineHook", "xDL64" } -- 链接外部库文件