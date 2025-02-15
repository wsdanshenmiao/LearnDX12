#pragma once
#ifndef __RENDERITEM__H__
#define __RENDERITEM__H__

#include "Pubh.h"
#include "Mesh.h"
#include "Transform.h"
#include "Material.h"

namespace DSM {

	/// <summary>
	/// 单个渲染对象，网格从总网格中获取
	/// </summary>
	struct RenderItem
	{
		Transform m_Transform = {};
		std::string m_Name;							// 根据名字在总网格资源中找到对应网格
		const Geometry::MeshData* m_Mesh = nullptr;		// 所使用的网格数据
		const Material* m_Material = {};	// 使用的材质
		UINT m_RenderCBIndex = -1;					// 对应常量缓冲区索引
		int m_NumFramesDirty = 0;					// 每个帧资源内的数据是否改变
		D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	};

}

#endif // !__RENDERITEM__H__
