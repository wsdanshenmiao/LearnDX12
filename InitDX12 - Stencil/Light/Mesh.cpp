#include "Mesh.h"

D3D12_VERTEX_BUFFER_VIEW DSM::Geometry::MeshData::GetVertexBufferView() const
{
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	vbView.BufferLocation = m_VertexBufferGPU->GetGPUVirtualAddress();
	vbView.SizeInBytes = m_VertexBufferByteSize;
	vbView.StrideInBytes = m_VertexByteStride;
	return vbView;
}

D3D12_INDEX_BUFFER_VIEW DSM::Geometry::MeshData::GetIndexBufferView() const
{
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = m_IndexBufferGPU->GetGPUVirtualAddress();
	ibView.Format = m_IndexFormat;
	ibView.SizeInBytes = m_IndexBufferByteSize;
	return ibView;
}

void DSM::Geometry::MeshData::DisposeUploaders()
{
	m_VertexBufferUploader = nullptr;
	m_IndexBufferUploader = nullptr;
}
