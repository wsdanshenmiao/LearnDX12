#ifndef __MESH__H__
#define __MESH__H__

#include "Pubh.h"

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

			MeshData() = default;

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
			std::unordered_map<std::string, SubmeshData> m_DrawArgs;
		};
	}

}
#endif // !__MESH__H__
