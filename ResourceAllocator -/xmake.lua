groupName = "ResourceAllocator -"
commonName = groupName .. "Common"
if is_mode("debug") then 
    binDir = path.join(os.projectdir(), "Bin/Debug/" .. groupName)
else 
    binDir = path.join(os.projectdir(), "Bin/Release/" .. groupName)
end 
includes("Common")
includes("ResourceAllocator")
includes("DescriptorHeap")
includes("ShadowMap")
includes("TreeBillboards")