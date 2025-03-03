if is_mode("debug") then 
    binDir = path.join(os.projectdir(), "Bin/Debug/ResourceAllocator -")
else 
    binDir = path.join(os.projectdir(), "Bin/Release/ResourceAllocator -")
end 
includes("Common")
includes("ResourceAllocator")