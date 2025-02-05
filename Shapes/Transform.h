#pragma once
#ifndef __TRANSFORM__H__
#define __TRANSFORM__H__

#include <DirectXMath.h>

namespace DSM {
	class Transform
	{
	public:
		Transform() = default;
		Transform(const DirectX::XMFLOAT3& scale,
			const DirectX::XMFLOAT3& rotate,
			const DirectX::XMFLOAT3& translate) noexcept;

		void SetRotate(const DirectX::XMFLOAT3& rotation) noexcept;
		void SetPosition(const DirectX::XMFLOAT3& position) noexcept;
		void SetScale(const DirectX::XMFLOAT3& scale) noexcept;

		DirectX::XMMATRIX GetScaleMatrix() const noexcept;
		DirectX::XMMATRIX GetRotateMatrix() const noexcept;
		DirectX::XMVECTOR GetTranslation() const noexcept;
		DirectX::XMMATRIX GetWorldMatrix() const noexcept;

	private:
		DirectX::XMFLOAT3 m_Scale = { 1,1,1 };
		DirectX::XMFLOAT4 m_Rotation = { 0,0,0,1 };
		DirectX::XMFLOAT3 m_Position = {};
	};
}


#endif //!__TRANSFORM__H__