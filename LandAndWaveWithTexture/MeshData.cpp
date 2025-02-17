#include "MeshData.h"

namespace DSM::Geometry{
    D3D12_VERTEX_BUFFER_VIEW MeshData::GetVertexBufferView() const
    {
        D3D12_VERTEX_BUFFER_VIEW vbView{};
        vbView.BufferLocation = m_VertexBufferGPU->GetGPUVirtualAddress();
        vbView.SizeInBytes = m_VertexBufferByteSize;
        vbView.StrideInBytes = m_VertexByteStride;
        return vbView;
    }

    D3D12_INDEX_BUFFER_VIEW MeshData::GetIndexBufferView() const
    {
        D3D12_INDEX_BUFFER_VIEW ibView{};
        ibView.BufferLocation = m_IndexBufferGPU->GetGPUVirtualAddress();
        ibView.Format = m_IndexFormat;
        ibView.SizeInBytes = m_IndexBufferByteSize;
        return ibView;
    }

    void MeshData::DisposeUploaders()
    {
        m_VertexBufferUploader = nullptr;
        m_IndexBufferUploader = nullptr;
    }

}
