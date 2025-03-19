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

		void SetRotation(const DirectX::XMFLOAT3& rotation) noexcept;
		void SetRotation(float x, float y, float z) noexcept;
		void SetPosition(const DirectX::XMFLOAT3& position) noexcept;
		void SetPosition(float x, float y, float z) noexcept;
		void SetScale(const DirectX::XMFLOAT3& scale) noexcept;
		void SetScale(float x, float y, float z) noexcept;

		void Rotate(const DirectX::XMFLOAT3& eulerAngles) noexcept;		// 根据欧拉角旋转
		void RotateAxis(const DirectX::XMFLOAT3& axis, float radian) noexcept;
		void RotateAround(const DirectX::XMFLOAT3& point, const DirectX::XMFLOAT3& axis, float radian) noexcept;
		void Translate(const DirectX::XMFLOAT3& direction, float magnitude) noexcept;
		void LookAt(const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up) noexcept;
		void LookTo(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& up) noexcept;

		DirectX::XMFLOAT3 GetPosition() const noexcept;
		DirectX::XMVECTOR GetPositionXM() const noexcept;
		
		DirectX::XMFLOAT3 GetScale() const noexcept;
		DirectX::XMVECTOR GetScaleXM() const noexcept;
		
		DirectX::XMFLOAT3 GetRotation() const;			// 获取旋转欧拉角
		DirectX::XMVECTOR GetRotationXM() const;
		DirectX::XMFLOAT4 GetRotationQuat() const noexcept;		// 获取旋转四元数
		DirectX::XMVECTOR GetRotationQuatXM() const noexcept;

		DirectX::XMFLOAT3 GetRightAxis() const noexcept;		// 获取右轴
		DirectX::XMVECTOR GetRightAxisXM() const noexcept;
		DirectX::XMFLOAT3 GetUpAxis() const noexcept;			// 获取上轴
		DirectX::XMVECTOR GetUpAxisXM() const noexcept;
		DirectX::XMFLOAT3 GetForwardAxis() const noexcept;		// 获取前轴
		DirectX::XMVECTOR GetForwardAxisXM() const noexcept;
		
		DirectX::XMMATRIX GetScaleMatrix() const noexcept;
		DirectX::XMMATRIX GetRotateMatrix() const noexcept;
		DirectX::XMMATRIX GetTranslationMatrix() const noexcept;
		DirectX::XMMATRIX GetLocalToWorldMatrix() const noexcept;
		DirectX::XMMATRIX GetWorldToLocalMatrix() const noexcept;


		static DirectX::XMFLOAT3 GetEulerAnglesFromRotationMatrix(const DirectX::XMFLOAT4X4& rotationMatrix);

	private:
		DirectX::XMFLOAT3 m_Scale = { 1,1,1 };
		DirectX::XMFLOAT4 m_Rotation = { 0,0,0,1 };
		DirectX::XMFLOAT3 m_Position = {};
	};
}


#endif //!__TRANSFORM__H__