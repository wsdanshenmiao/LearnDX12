target(commonName)
    -- 设置目标类型
    set_kind("static")
    set_group(groupName)
    set_targetdir(path.join(binDir, targetName))
    set_filename("Common.lib") -- 指定输出的文件的名字

    add_deps("Imgui")
    add_headerfiles("**.h")
    add_files("**.cpp")
    -- 设置头文件的搜索目录
    add_includedirs("./", {public = true})

target_end()