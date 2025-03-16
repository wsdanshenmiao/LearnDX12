groupName = "InitDX12 - Stencil"
commonName = groupName .. "Common"
if is_mode("debug") then 
    binDir = path.join(os.projectdir(), "Bin/Debug/" .. groupName)
else 
    binDir = path.join(os.projectdir(), "Bin/Release/" .. groupName)
end 
includes("Common")
includes("InitDX12")
includes("DrawBox")
includes("Shapes")
includes("LandAndWave")
includes("Light")
includes("Texturing")
includes("LandAndWaveWithTexture")
includes("Blend")
includes("Stencil")