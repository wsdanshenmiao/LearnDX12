#include "Transform.h"

using namespace DirectX;

namespace DSM {
	Transform::Transform(const DirectX::XMFLOAT3& scale,
		const DirectX::XMFLOAT3& rotation,
		const DirectX::XMFLOAT3& position) noexcept
		:m_Scale(scale), m_Position(position)
	{
		SetRotate(rotation);
	}

	void Transform::SetRotate(const DirectX::XMFLOAT3& rotation) noexcept
	{
		auto quat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
		XMStoreFloat4(&m_Rotation, quat);
	}

	void Transform::SetPosition(const DirectX::XMFLOAT3& position) noexcept
	{
		m_Position = position;
	}

	void Transform::SetScale(const DirectX::XMFLOAT3& scale) noexcept
	{
		m_Scale = scale;
	}

	DirectX::XMMATRIX Transform::GetScaleMatrix() const noexcept
	{
		return XMMatrixScalingFromVector(XMLoadFloat3(&m_Scale));
	}

	DirectX::XMMATRIX Transform::GetRotateMatrix() const noexcept
	{
		return XMMatrixRotationQuaternion(XMLoadFloat4(&m_Rotation));
	}

	DirectX::XMVECTOR Transform::GetTranslation() const noexcept
	{
		return XMLoadFloat3(&m_Position);
	}

	DirectX::XMMATRIX Transform::GetWorldMatrix() const noexcept
	{
		DirectX::XMMATRIX world = XMMatrixAffineTransformation(
			XMLoadFloat3(&m_Scale),
			g_XMZero,
			XMLoadFloat4(&m_Rotation),
			XMLoadFloat3(&m_Position));
		return world;
	}


}