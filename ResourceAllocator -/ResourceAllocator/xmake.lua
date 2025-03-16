targetName = "ResourceAllocator"
target(targetName)
    set_kind("binary")
    set_group(groupName)
    set_targetdir(path.join(binDir, targetName))

    add_deps(commonName)
    
    add_rules("Imguiini")
    add_rules("ShaderCopy")
    add_rules("ModelCopy")
    add_rules("TextureCopy")

    add_dxsdk_options()

    add_files("**.cpp")
    add_headerfiles("**.h")
    add_headerfiles("Shaders/**.hlsli", "Shaders/**.hlsl")

target_end()