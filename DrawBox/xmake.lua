targetName = "DrawBox"
target(targetName)
    set_kind("binary")
    set_targetdir(path.join(binDir, targetName))

    add_deps("Common")
    add_dxsdk_options()
    add_rules("Imguiini")
    add_rules("ShaderCopy")
    -- add_headerfiles("Shaders/**.hlsl|Shaders/**.hlsli")
    -- add_files("Shaders/**.hlsl|Shaders/**.hlsli")
    add_headerfiles("**.h")
    add_files("**.cpp")

target_end()