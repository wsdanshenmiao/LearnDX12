targetName = "Common"
target(targetName)
    -- 设置目标类型
    set_kind("static")
    set_group("InitDX12 - Stencil")
    set_targetdir(path.join(binDir, targetName))

    add_deps("Imgui")
    add_headerfiles("**.h")
    add_files("**.cpp")
    -- 设置头文件的搜索目录
    add_includedirs("./", {public = true})

target_end()