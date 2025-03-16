targetName = "LandAndWave"
target(targetName)
    set_kind("binary")
    set_group(groupName)
    set_targetdir(path.join(binDir, targetName))

    add_deps(commonName)
    add_rules("Imguiini")
    add_dxsdk_options()
    add_rules("ShaderCopy")

    add_headerfiles("**.h")
    add_files("**.cpp")

target_end()