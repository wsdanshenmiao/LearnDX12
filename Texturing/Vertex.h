#pragma once
#ifndef __VERTEX__H__
#define __VERTEX__H__

#include "Pubh.h"

namespace DSM {

	struct VertexPosLColor
	{
		static decltype(auto) GetInputLayout()
		{
			static const std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputLayout = {
				D3D12_INPUT_ELEMENT_DESC{ "POSITIONT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};
			return inputLayout;
		};

		DirectX::XMFLOAT3 m_Pos;
		DirectX::XMFLOAT4 m_Color;
	};

	struct VertexPosLNormalColor
	{
		static decltype(auto) GetInputLayout()
		{
			static const std::array<D3D12_INPUT_ELEMENT_DESC, 3> inputLayout = {
				D3D12_INPUT_ELEMENT_DESC{ "POSITIONT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};
			return inputLayout;
		};

		DirectX::XMFLOAT3 m_Pos;
		DirectX::XMFLOAT3 m_Normal;
		DirectX::XMFLOAT4 m_Color;
	};

	struct VertexPosLNormalTangentColor
	{
		static decltype(auto) GetInputLayout()
		{
			static const std::array<D3D12_INPUT_ELEMENT_DESC, 4> inputLayout = {
				D3D12_INPUT_ELEMENT_DESC{ "POSITIONT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};
			return inputLayout;
		}

		DirectX::XMFLOAT3 m_Pos;
		DirectX::XMFLOAT3 m_Normal;
		DirectX::XMFLOAT4 m_Tangent;
		DirectX::XMFLOAT4 m_Color;
	};

	struct VertexPosLNormal
	{
		static decltype(auto) GetInputLayout()
		{
			static const std::array<D3D12_INPUT_ELEMENT_DESC, 2> inputLayout = {
				D3D12_INPUT_ELEMENT_DESC{ "POSITIONT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};
			return inputLayout;
		};

		DirectX::XMFLOAT3 m_Pos;
		DirectX::XMFLOAT3 m_Normal;
	};

	struct VertexPosLNormalTex
	{
		static decltype(auto) GetInputLayout()
		{
			static const std::array<D3D12_INPUT_ELEMENT_DESC, 3> inputLayout = {
				D3D12_INPUT_ELEMENT_DESC{ "POSITIONT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24,
				D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};
			return inputLayout;
		};

		DirectX::XMFLOAT3 m_Pos;
		DirectX::XMFLOAT3 m_Normal;
		DirectX::XMFLOAT2 m_TexCoord;
	};

}

#endif // !__VERTEX__H__
