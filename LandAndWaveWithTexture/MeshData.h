#ifndef __MESH__H__
#define __MESH__H__

#include "Pubh.h"
#include "Geometry.h"
#include "D3DUtil.h"

namespace DSM {

	namespace Geometry {
		struct SubmeshData
		{
			UINT m_IndexCount = 0;
			UINT m_StarIndexLocation = 0;
			INT m_BaseVertexLocation = 0;

			// 单个物体的包围盒
			DirectX::BoundingBox m_Bound;
		};

		struct MeshData
		{
			template <class T>
			using ComPtr = Microsoft::WRL::ComPtr<T>;

			template<typename VertexData, typename VertFunc>
			void CreateMeshData(
				const GeometryMesh& mesh,
				ID3D12Device* device,
				ID3D12GraphicsCommandList* cmdList,
				VertFunc vertFunc);

			D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;
			D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;
			// 当上传数据到GPU后即可释放上传缓冲区的内存
			void DisposeUploaders();

			// 几何体集合的名字，便于索引
			std::string m_Name;

			// 系统内存中的副本，顶点和索引可为范式类型，用Blob表示，可在特定的时候转换
			ComPtr<ID3DBlob> m_VertexBufferCPU = nullptr;
			ComPtr<ID3DBlob> m_IndexBufferCPU = nullptr;

			ComPtr<ID3D12Resource> m_VertexBufferGPU = nullptr;
			ComPtr<ID3D12Resource> m_IndexBufferGPU = nullptr;
			ComPtr<ID3D12Resource> m_VertexBufferUploader = nullptr;
			ComPtr<ID3D12Resource> m_IndexBufferUploader = nullptr;

			// 缓冲区相关数据
			UINT m_VertexByteStride = 0;
			UINT m_VertexBufferByteSize = 0;
			DXGI_FORMAT m_IndexFormat = DXGI_FORMAT_R16_UINT;
			UINT m_IndexSize = 0;
			UINT m_IndexBufferByteSize = 0;

			// 一个MeshData可以统一储存多个几何体，使用下列容器可单独绘制每个几何体
			std::map<std::string, SubmeshData> m_DrawArgs;
		};

		template<typename VertexData, typename VertFunc>
	    inline void MeshData::CreateMeshData(
    		const GeometryMesh& mesh,
    		ID3D12Device* device,
			ID3D12GraphicsCommandList* cmdList,
			VertFunc vertFunc)
		{
			// 将顶点数据转换为需要的格式
			std::vector<VertexData> verticesData;
			verticesData.reserve(mesh.m_Vertices.size());
			for (const auto& vert : mesh.m_Vertices) {
				verticesData.push_back(vertFunc(vert));
			}

			auto vbByteSize = verticesData.size() * sizeof(VertexData);
			auto ibByteSize = mesh.m_Indices32.size() * sizeof(std::uint32_t);
			
			ThrowIfFailed(D3DCreateBlob(vbByteSize, m_VertexBufferCPU.GetAddressOf()));
			memcpy(m_VertexBufferCPU->GetBufferPointer(), verticesData.data(), vbByteSize);
			ThrowIfFailed(D3DCreateBlob(ibByteSize, m_IndexBufferCPU.GetAddressOf()));
			memcpy(m_IndexBufferCPU->GetBufferPointer(), mesh.m_Indices32.data(), ibByteSize);
			
			m_VertexByteStride = (UINT)sizeof(VertexData);
			m_VertexBufferByteSize = (UINT)vbByteSize;
			m_IndexFormat = DXGI_FORMAT_R32_UINT;
			m_IndexBufferByteSize = (UINT)ibByteSize;
			

			// 创建顶点和索引的资源
			D3D12_HEAP_PROPERTIES defaultHeapProps{};
			defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
			D3D12_HEAP_PROPERTIES uploadHeapProps{};
			uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

			D3D12_RESOURCE_DESC desc{};
			desc.Alignment = 0;
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;
			desc.Width = vbByteSize;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.SampleDesc = {1,0};
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.MipLevels = 1;

			ThrowIfFailed(device->CreateCommittedResource(
				&defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(m_VertexBufferGPU.GetAddressOf())));
			ThrowIfFailed(device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				nullptr,
				IID_PPV_ARGS(m_VertexBufferUploader.GetAddressOf())));

			desc.Width = ibByteSize;
			ThrowIfFailed(device->CreateCommittedResource(
				&defaultHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(m_IndexBufferGPU.GetAddressOf())));
			ThrowIfFailed(device->CreateCommittedResource(
				&uploadHeapProps,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_COPY_SOURCE,
				nullptr,
				IID_PPV_ARGS(m_IndexBufferUploader.GetAddressOf())));

			// 拷贝数据到上传堆中
			BYTE* mappedData = nullptr;
			ThrowIfFailed(m_VertexBufferUploader->Map(0,nullptr, reinterpret_cast<void**>(&mappedData)));
			memcpy(mappedData,verticesData.data(),vbByteSize);
			m_VertexBufferUploader->Unmap(0, nullptr);
			// 将上传堆中的数据拷贝到默认堆中
			cmdList->CopyBufferRegion(m_VertexBufferGPU.Get(), 0, m_VertexBufferUploader.Get(), 0, vbByteSize);

			mappedData = nullptr;
			ThrowIfFailed(m_IndexBufferUploader->Map(0,nullptr, reinterpret_cast<void**>(&mappedData)));
			memcpy(mappedData,mesh.m_Indices32.data(),ibByteSize);
			m_IndexBufferUploader->Unmap(0, nullptr);
			cmdList->CopyBufferRegion(m_IndexBufferGPU.Get(), 0, m_IndexBufferUploader.Get(), 0, ibByteSize);
	    }
		
	}

}
#endif // !__MESH__H__
