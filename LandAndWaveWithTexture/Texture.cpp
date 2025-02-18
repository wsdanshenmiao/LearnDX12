#include "Texture.h"
#include "D3DUtil.h"
#include "DDSTextureLoader12.h"

using namespace DirectX;

namespace DSM {
    Texture::Texture(const std::string& name) noexcept
        :m_Name(name){
    }

    Texture::Texture(
        const std::string& name,
        const std::wstring& fileName,   
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList)
        :m_Name(name){
        LoadTextureFromFile(*this, fileName,device,cmdList);
    }

    const std::string& Texture::GetName() const noexcept
    {
        return m_Name;
    }

    ID3D12Resource* Texture::GetTexture()
    {
        return m_Texture.Get();
    }

    const ID3D12Resource* Texture::GetTexture() const
    {
        return m_Texture.Get();
    }

    void Texture::SetName(const std::string& name)
    {
        m_Name = name;
    }

    void Texture::DisposeUploader() noexcept
    {
        m_UploadHeap = nullptr;
    }

    bool Texture::LoadTextureFromFile(
        Texture& texture,
        const std::wstring& fileName,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* cmdList)
    {
        texture.m_Texture = nullptr;
        texture.m_UploadHeap = nullptr;
        
        std::unique_ptr<uint8_t[]> ddsData;
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        ThrowIfFailed(LoadDDSTextureFromFile(
            device,
            fileName.c_str(),
            texture.m_Texture.GetAddressOf(),
            ddsData, subresources));

        // 获取上传堆的大小
        UINT64 uploadBufferSize;
        auto texDesc = texture.m_Texture->GetDesc();
        texture.m_Texture->GetDevice(IID_PPV_ARGS(&device));
        device->GetCopyableFootprints(&texDesc, 0, subresources.size(), 0, nullptr, nullptr, nullptr, &uploadBufferSize);


        // 创建GPU上传堆
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        auto uploadResDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(texture.m_UploadHeap.GetAddressOf())));

        UpdateSubresources(
            cmdList, texture.m_Texture.Get(), texture.m_UploadHeap.Get(),
            0, 0, static_cast<UINT>(subresources.size()), subresources.data());

        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition = {
            texture.m_Texture.Get(),
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        };
        cmdList->ResourceBarrier(1, &barrier);

        return true;
    }
}
