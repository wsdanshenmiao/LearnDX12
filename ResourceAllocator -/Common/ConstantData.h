#pragma once
#ifndef __CONSTANTDATA__H__
#define __CONSTANTDATA__H__

#include <DirectXMath.h>
#include "MathHelper.h"
#include "UploadBuffer.h"

namespace DSM {
	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 m_World = MathHelper::Identity();
		DirectX::XMFLOAT4X4 m_WorldInvTranspose = MathHelper::Identity();
	};

	struct PassConstants
	{
		DirectX::XMFLOAT4X4 m_View = MathHelper::Identity();
		DirectX::XMFLOAT4X4 m_InvView = MathHelper::Identity();
		DirectX::XMFLOAT4X4 m_Proj = MathHelper::Identity();
		DirectX::XMFLOAT4X4 m_InvProj = MathHelper::Identity();
		DirectX::XMFLOAT3 m_EyePosW = { 0.0f, 0.0f, 0.0f };
		float m_FogStart = 20;
		DirectX::XMFLOAT3 m_FogColor = { 1.0f, 1.0f, 1.0f };
		float m_FogRange = 100;
		DirectX::XMFLOAT2 m_RenderTargetSize = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 m_InvRenderTargetSize = { 0.0f, 0.0f };
		float m_NearZ = 0.0f;
		float m_FarZ = 0.0f;
		float m_TotalTime = 0.0f;
		float m_DeltaTime = 0.0f;
	};

	struct MaterialConstants
	{
		DirectX::XMFLOAT3 m_Diffuse = { 0,0,0 };
		float m_Alpha = 1;
		DirectX::XMFLOAT3 m_Specular = { 0,0,0 };
		float m_Gloss = 0.2;
		DirectX::XMFLOAT3 m_Ambient = { 0,0,0 };
		float m_Pad1;
	};
}

#endif // !__CONSTANTDATA__H__
