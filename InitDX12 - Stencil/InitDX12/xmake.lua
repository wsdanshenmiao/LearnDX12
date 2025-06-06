targetName = "InitDX12"
target(targetName)
    -- 设置目标类型
    set_kind("binary")
    set_group(groupName)
    set_targetdir(path.join(binDir, targetName))

    add_deps(commonName)

    -- 添加当前目标的依赖目标，编译的时候，会去优先编译依赖的目标，然后再编译当前目标。
    add_dxsdk_options();
    add_rules("Imguiini")
    add_headerfiles("**.h")
    add_files("**.cpp")

target_end()