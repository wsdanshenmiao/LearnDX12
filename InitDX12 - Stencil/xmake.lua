if is_mode("debug") then 
    binDir = path.join(os.projectdir(), "Bin/Debug/InitDX12 - Stencil")
else 
    binDir = path.join(os.projectdir(), "Bin/Release/InitDX12 - Stencil")
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