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

	void Transform::SetRotate(float x, float y, float z) noexcept
	{
		SetRotate(XMFLOAT3{ x,y,z });
	}

	void Transform::SetPosition(const DirectX::XMFLOAT3& position) noexcept
	{
		m_Position = position;
	}

	void Transform::SetPosition(float x, float y, float z) noexcept
	{
		SetPosition(XMFLOAT3{ x,y,z });
	}

	void Transform::SetScale(const DirectX::XMFLOAT3& scale) noexcept
	{
		m_Scale = scale;
	}

	void Transform::SetScale(float x, float y, float z) noexcept
	{
		SetScale(XMFLOAT3{ x, y, z });
	}

	DirectX::XMFLOAT3 Transform::GetPosition() const noexcept
	{
		return m_Position;
	}

	DirectX::XMFLOAT3& Transform::GetPosition() noexcept
	{
		return m_Position;
	}

	DirectX::XMFLOAT3 Transform::GetScale() const noexcept
	{
		return m_Scale;
	}

	DirectX::XMFLOAT3& Transform::GetScale() noexcept
	{
		return m_Scale;
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