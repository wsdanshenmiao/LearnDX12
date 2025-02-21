set_project("LearnDX12")

option("WIN7_SYSTEM_SUPPORT")
    set_default(false)
    set_description("Windows7 users need to select this option!")
option_end()

-- 需要定义 UNICODE 否则 TCHAR 无法转换为 wstring 
if is_os("windows") then 
    add_defines("UNICODE")
    add_defines("_UNICODE")
end

add_rules("mode.debug", "mode.release")

set_languages("c99", "cxx20")
set_toolchains("msvc")
set_encodings("utf-8")


if is_mode("debug") then 
    binDir = path.join(os.projectdir(), "Bin/Debug")
else 
    binDir = path.join(os.projectdir(), "Bin/Release")
end 


function add_dxsdk_options() 
    if has_config("WIN7_SYSTEM_SUPPORT") then
        add_defines("_WIN32_WINNT=0x601")
    end
    -- 添加系统库依赖
    add_syslinks("d3d12", "dxgi", "d3dcompiler", "dxguid")
end

-- 添加需要的依赖包,同时禁用系统包
add_requires("assimp", {system = false})
add_packages("assimp")

includes("rules.lua")

includes("Imgui")
includes("Common")
includes("InitDX12")
includes("DrawBox")
includes("Shapes")
includes("LandAndWave")
includes("Light")
includes("Texturing")
includes("LandAndWaveWithTexture")
includes("Blend")
