targetName = "Imgui"
target(targetName)
    set_group(targetName)
    set_kind("static")
    set_targetdir(path.join(binDir, targetName))

    add_headerfiles("**.h")
    add_files("**.cpp")
    add_includedirs("./", {public = true})

target_end()