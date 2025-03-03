#pragma once
#ifndef __LIGHT__H__
#define __LIGHT__H__

/*
	匹配光源常量缓冲区的结构体
*/

#include "Pubh.h"

namespace DSM {
	struct ILight
	{
		DirectX::XMFLOAT3 m_Color;
	};

	struct DirectionalLight : public ILight
	{
		float m_Pad0;
		DirectX::XMFLOAT3 m_Dir;
		float m_Pad1;
	};

	struct PointLight : ILight
	{
		float m_StartAtten;
		DirectX::XMFLOAT3 m_Pos;
		float m_EndAtten;
	};

	struct SpotLight : ILight
	{
		float m_StartAtten;
		DirectX::XMFLOAT3 m_Pos;
		float m_EndAtten;
		DirectX::XMFLOAT3 m_Dir;
		float m_SpotPower;
	};

}

#endif // !__LIGHT__H__
