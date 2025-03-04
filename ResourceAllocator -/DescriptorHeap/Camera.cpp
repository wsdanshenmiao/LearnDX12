#include "Camera.h"

using namespace DirectX;

namespace DSM {
    const Transform& Camera::GetTransform() const noexcept
    {
        return m_Transform ;
    }

    DirectX::XMMATRIX Camera::GetViewMatrixXM() const noexcept
    {
        // 获取世界矩阵的逆矩阵即可
        return m_Transform.GetWorldToLocalMatrix();
    }

    DirectX::XMMATRIX Camera::GetProjMatrixXM(bool reversedZ) const noexcept
    {
        if (reversedZ) {
            return XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_FarZ, m_NearZ);
        }
        else {
            return XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearZ, m_FarZ);
        }
    }

    DirectX::XMMATRIX Camera::GetViewProjMatrixXM(bool reversedZ) const noexcept
    {
        return GetViewMatrixXM() * GetProjMatrixXM(reversedZ);
    }

    D3D12_VIEWPORT Camera::GetViewPort() const noexcept
    {
        return m_ViewPort;
    }

    float Camera::GetNearZ() const noexcept
    {
        return m_NearZ;
    }

    float Camera::GetFarZ() const noexcept
    {
        return m_FarZ;
    }

    float Camera::GetFovY() const noexcept
    {
        return m_FovY;
    }

    float Camera::GetAspectRatio() const noexcept
    {
        return m_Aspect;
    }

    void Camera::SetPosition(float x, float y, float z) noexcept
    {
        m_Transform.SetPosition(x, y, z);
    }

    void Camera::SetPosition(const DirectX::XMFLOAT3& position) noexcept
    {
        m_Transform.SetPosition(position);
    }

    
    void Camera::LookAt(const XMFLOAT3 & pos, const XMFLOAT3 & target,const XMFLOAT3 & up) noexcept
    {
        m_Transform.SetPosition(pos);
        m_Transform.LookAt(target, up);
    }

    void Camera::LookTo(const XMFLOAT3 & pos, const XMFLOAT3 & to, const XMFLOAT3 & up) noexcept
    {
        m_Transform.SetPosition(pos);
        m_Transform.LookTo(to, up);
    }

    void Camera::Strafe(float d) noexcept
    {
        m_Transform.Translate(m_Transform.GetRightAxis(), d);
    }

    void Camera::Walk(float d) noexcept
    {
        XMVECTOR rightVec = m_Transform.GetRightAxisXM();
        XMVECTOR frontVec = XMVector3Normalize(XMVector3Cross(rightVec, g_XMIdentityR1));
        XMFLOAT3 front;
        XMStoreFloat3(&front, frontVec);
        m_Transform.Translate(front, d);
    }

    void Camera::MoveForward(float d) noexcept
    {
        m_Transform.Translate(m_Transform.GetForwardAxis(), d);
    }

    void Camera::Translate(const DirectX::XMFLOAT3& dir, float magnitude) noexcept
    {
        m_Transform.Translate(dir, magnitude);
    }

    void Camera::Pitch(float rad) noexcept
    {
        XMFLOAT3 rotation = m_Transform.GetRotation();
        // 将绕x轴旋转弧度限制在[-7pi/18, 7pi/18]之间
        rotation.x += rad;
        if (rotation.x > XM_PI * 7 / 18)
            rotation.x = XM_PI * 7 / 18;
        else if (rotation.x < -XM_PI * 7 / 18)
            rotation.x = -XM_PI * 7 / 18;

        m_Transform.SetRotation(rotation);
    }

    void Camera::RotateY(float rad) noexcept
    {
        XMFLOAT3 rotation = m_Transform.GetRotation();
        rotation.y = XMScalarModAngle(rotation.y + rad);
        m_Transform.SetRotation(rotation);
    }


    void Camera::SetFrustum(float fovY, float aspect, float nearZ, float farZ) noexcept
    {
        m_FovY = fovY;
        m_Aspect = aspect;
        m_NearZ = nearZ;
        m_FarZ = farZ;
    }

    void Camera::SetViewPort(const D3D12_VIEWPORT& viewPort) noexcept
    {
        m_ViewPort = viewPort;
    }

    void Camera::SetViewPort(
        float topLeftX,
        float topLeftY,
        float width,
        float height,
        float minDepth,
        float maxDepth) noexcept
    {
        m_ViewPort = { topLeftX, topLeftY, width, height, minDepth, maxDepth };
    }
}
