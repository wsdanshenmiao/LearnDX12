targetName = "LandAndWaveWithTexture"
target(targetName)
    set_kind("binary")
    set_targetdir(path.join(binDir, targetName))
    set_group("InitDX12 - Stencil")
    add_deps("Common")
    
    add_rules("Imguiini")
    add_rules("ShaderCopy")
    add_rules("TextureCopy")
    add_rules("ModelCopy")
    add_dxsdk_options()

    add_headerfiles("**.h")
    add_files("**.cpp")
    
    add_headerfiles("Shaders/**.hlsli", "Shaders/**.hlsl")

target_end()