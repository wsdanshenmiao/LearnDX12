targetName = "Shapes"
target(targetName)
    set_kind("binary")
    set_targetdir(path.join(binDir, targetName))
    
    add_deps("Common")
    add_rules("Imguiini")
    add_rules("ShaderCopy")
    add_dxsdk_options()
    add_headerfiles("**.h")
    add_files("**.cpp")

target_end()