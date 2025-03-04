#include "Transform.h"

#include <complex>

using namespace DirectX;

namespace DSM {
	Transform::Transform(const DirectX::XMFLOAT3& scale,
		const DirectX::XMFLOAT3& rotation,
		const DirectX::XMFLOAT3& position) noexcept
		:m_Scale(scale), m_Position(position)
	{
		SetRotation(rotation);
	}

	void Transform::SetRotation(const DirectX::XMFLOAT3& rotation) noexcept
	{
		auto quat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&rotation));
		XMStoreFloat4(&m_Rotation, quat);
	}

	void Transform::SetRotation(float x, float y, float z) noexcept
	{
		SetRotation(XMFLOAT3{ x,y,z });
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

	void Transform::Rotate(const XMFLOAT3& eulerAngles) noexcept
	{
		auto newQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&eulerAngles));
		auto rotate = XMQuaternionMultiply(XMLoadFloat4(&m_Rotation), newQuat);
		XMStoreFloat4(&m_Rotation, rotate);
	}

	void Transform::RotateAxis(const DirectX::XMFLOAT3& axis, float radian) noexcept
	{
		auto newQuat = XMQuaternionRotationAxis(XMLoadFloat3(&axis), radian);
		auto rotate = XMQuaternionMultiply(XMLoadFloat4(&m_Rotation), newQuat);
		XMStoreFloat4(&m_Rotation, rotate);
	}

	// 指定以point为旋转中心绕轴旋转
	void Transform::RotateAround(const DirectX::XMFLOAT3& point, const DirectX::XMFLOAT3& axis, float radian) noexcept
	{
	    XMVECTOR quat = XMLoadFloat4(&m_Rotation);
	    XMVECTOR positionVec = XMLoadFloat3(&m_Position);
	    XMVECTOR centerVec = XMLoadFloat3(&point);

	    // 以point作为原点进行旋转
	    XMMATRIX RT = XMMatrixRotationQuaternion(quat) * XMMatrixTranslationFromVector(positionVec - centerVec);
	    RT *= XMMatrixRotationAxis(XMLoadFloat3(&axis), radian);
	    RT *= XMMatrixTranslationFromVector(centerVec);
	    XMStoreFloat4(&m_Rotation, XMQuaternionRotationMatrix(RT));
	    XMStoreFloat3(&m_Position, RT.r[3]);
	}
	// 沿着某一方向平移
	void Transform::Translate(const DirectX::XMFLOAT3& direction, float magnitude) noexcept
	{
	    XMVECTOR directionVec = XMVector3Normalize(XMLoadFloat3(&direction));
	    XMVECTOR newPosition = XMVectorMultiplyAdd(XMVectorReplicate(magnitude), directionVec, XMLoadFloat3(&m_Position));
	    XMStoreFloat3(&m_Position, newPosition);
	}

	// 观察某一点
	void Transform::LookAt(const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up = { 0.0f, 1.0f, 0.0f }) noexcept
	{
	    XMMATRIX View = XMMatrixLookAtLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&target), XMLoadFloat3(&up));
	    XMMATRIX InvView = XMMatrixInverse(nullptr, View);
	    XMStoreFloat4(&m_Rotation, XMQuaternionRotationMatrix(InvView));
	}
	// 沿着某一方向观察
	void Transform::LookTo(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& up = { 0.0f, 1.0f, 0.0f }) noexcept
	{
	    XMMATRIX View = XMMatrixLookToLH(XMLoadFloat3(&m_Position), XMLoadFloat3(&direction), XMLoadFloat3(&up));
	    XMMATRIX InvView = XMMatrixInverse(nullptr, View);
	    XMStoreFloat4(&m_Rotation, XMQuaternionRotationMatrix(InvView));
	}

	DirectX::XMFLOAT3 Transform::GetPosition() const noexcept
	{
		return m_Position;
	}

	DirectX::XMVECTOR Transform::GetPositionXM() const noexcept
	{
		return XMLoadFloat3(&m_Position);
	}

	DirectX::XMFLOAT3 Transform::GetScale() const noexcept
	{
		return m_Scale;
	}

	DirectX::XMVECTOR Transform::GetScaleXM() const noexcept
	{
		return XMLoadFloat3(&m_Scale);
	}

	DirectX::XMFLOAT3 Transform::GetRotation() const
	{
		// 四元数转欧拉角
		// 以Z-X-Y轴顺序旋转
		float sinX = 2 * (m_Rotation.w * m_Rotation.x - m_Rotation.y * m_Rotation.z);
		float sinY_cosX = 2 * (m_Rotation.w * m_Rotation.y + m_Rotation.x * m_Rotation.z);
		float cosY_cosX = 1 - 2 * (m_Rotation.x * m_Rotation.x + m_Rotation.y * m_Rotation.y);
		float sinZ_cosX = 2 * (m_Rotation.w * m_Rotation.z + m_Rotation.x * m_Rotation.y);
		float cosZ_cosX = 1 - 2 * (m_Rotation.x * m_Rotation.x + m_Rotation.z * m_Rotation.z);

		DirectX::XMFLOAT3 rotation;
		// 死锁特殊处理
		if (fabs(fabs(sinX) - 1.0f) < 1e-5f)
		{
			rotation.x = copysignf(DirectX::XM_PI / 2, sinX);
			rotation.y = 2.0f * atan2f(m_Rotation.y, m_Rotation.w);
			rotation.z = 0.0f;
		}
		else
		{
			rotation.x = asinf(sinX);
			rotation.y = atan2f(sinY_cosX, cosY_cosX);
			rotation.z = atan2f(sinZ_cosX, cosZ_cosX);
		}

		return rotation;
	}

	DirectX::XMVECTOR Transform::GetRotationXM() const
	{
		auto rotation = GetRotation();
		return XMLoadFloat3(&rotation);
	}

	DirectX::XMFLOAT4 Transform::GetRotationQuat() const noexcept
	{
		return m_Rotation;
	}

	DirectX::XMVECTOR Transform::GetRotationQuatXM() const noexcept
	{
		return XMLoadFloat4(&m_Rotation);
	}

	DirectX::XMFLOAT3 Transform::GetRightAxis() const noexcept
	{
		XMFLOAT3 right;
		XMStoreFloat3(&right, GetRotateMatrix().r[0]);
		return right;
	}

	DirectX::XMVECTOR Transform::GetRightAxisXM() const noexcept
	{
		auto right = GetRightAxis();
		return XMLoadFloat3(&right);
	}

	DirectX::XMFLOAT3 Transform::GetUpAxis() const noexcept
	{
		XMFLOAT3 up;
		XMStoreFloat3(&up, GetRotateMatrix().r[1]);
		return up;
	}

	DirectX::XMVECTOR Transform::GetUpAxisXM() const noexcept
	{
		auto up = GetUpAxis();
		return XMLoadFloat3(&up);
	}

	DirectX::XMFLOAT3 Transform::GetForwardAxis() const noexcept
	{
		XMFLOAT3 forward;
		XMStoreFloat3(&forward, GetRotateMatrix().r[2]);
		return forward;
	}

	DirectX::XMVECTOR Transform::GetForwardAxisXM() const noexcept
	{
		auto forward = GetForwardAxis();
		return XMLoadFloat3(&forward);
	}

	DirectX::XMMATRIX Transform::GetScaleMatrix() const noexcept
	{
		return XMMatrixScalingFromVector(XMLoadFloat3(&m_Scale));
	}

	DirectX::XMMATRIX Transform::GetRotateMatrix() const noexcept
	{
		return XMMatrixRotationQuaternion(XMLoadFloat4(&m_Rotation));
	}

	DirectX::XMMATRIX Transform::GetTranslationMatrix() const noexcept
	{
		return XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
	}

	DirectX::XMMATRIX Transform::GetLocalToWorldMatrix() const noexcept
	{
		DirectX::XMMATRIX world = XMMatrixAffineTransformation(
			XMLoadFloat3(&m_Scale),
			g_XMZero,
			XMLoadFloat4(&m_Rotation),
			XMLoadFloat3(&m_Position));
		return world;
	}

	DirectX::XMMATRIX Transform::GetWorldToLocalMatrix() const noexcept
	{
		return XMMatrixInverse(nullptr, GetLocalToWorldMatrix());
	}


	
	// 从旋转矩阵获取旋转欧拉角
	DirectX::XMFLOAT3 Transform::GetEulerAnglesFromRotationMatrix(const DirectX::XMFLOAT4X4& rotationMatrix)
	{
		DirectX::XMFLOAT3 rotation{};
		// 死锁特殊处理
		if (fabs(1.0f - fabs(rotationMatrix(2, 1))) < 1e-5f)
		{
			rotation.x = copysignf(DirectX::XM_PIDIV2, -rotationMatrix(2, 1));
			rotation.y = -atan2f(rotationMatrix(0, 2), rotationMatrix(0, 0));
			return rotation;
		}

		// 通过旋转矩阵反求欧拉角
		float c = sqrtf(1.0f - rotationMatrix(2, 1) * rotationMatrix(2, 1));
		// 防止r[2][1]出现大于1的情况
		if (isnan(c))
			c = 0.0f;
	    
		rotation.z = atan2f(rotationMatrix(0, 1), rotationMatrix(1, 1));
		rotation.x = atan2f(-rotationMatrix(2, 1), c);
		rotation.y = atan2f(rotationMatrix(2, 0), rotationMatrix(2, 2));
		return rotation;
	}
}
