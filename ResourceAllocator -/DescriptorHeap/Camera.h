#pragma once
#ifndef __CAMERA__H__
#define __CAMERA__H__

#include <d3d12.h>
#include "Transform.h"

namespace DSM {
    class Camera
    {
    public:
        const Transform& GetTransform() const noexcept;
        
        
        DirectX::XMMATRIX GetViewMatrixXM() const noexcept;
        DirectX::XMMATRIX GetProjMatrixXM(bool reversedZ = false) const noexcept;
        DirectX::XMMATRIX GetViewProjMatrixXM(bool reversedZ = false) const noexcept;

        // 获取视口
        D3D12_VIEWPORT GetViewPort() const noexcept;

        float GetNearZ() const noexcept;
        float GetFarZ() const noexcept;
        float GetFovY() const noexcept;
        float GetAspectRatio() const noexcept;

        void SetPosition(float x, float y, float z) noexcept;
        void SetPosition(const DirectX::XMFLOAT3& position) noexcept;
        // 设置摄像机的朝向
        void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target,const DirectX::XMFLOAT3& up) noexcept;
        void LookTo(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& to, const DirectX::XMFLOAT3& up) noexcept;
        // 平移
        void Strafe(float d) noexcept;
        // 直行(平面移动)
        void Walk(float d) noexcept;
        // 前进(朝前向移动)
        void MoveForward(float d) noexcept;
        // 移动
        void Translate(const DirectX::XMFLOAT3& dir, float magnitude) noexcept;
        // 上下观察
        // 正rad值向上观察
        // 负rad值向下观察
        void Pitch(float rad) noexcept;
        // 左右观察
        // 正rad值向右观察
        // 负rad值向左观察
        void RotateY(float rad) noexcept;

        // 设置视锥体
        void SetFrustum(float fovY, float aspect, float nearZ, float farZ) noexcept;

        // 设置视口
        void SetViewPort(const D3D12_VIEWPORT& viewPort) noexcept;
        void SetViewPort(
            float topLeftX,
            float topLeftY,
            float width,
            float height,
            float minDepth = 0.0f,
            float maxDepth = 1.0f) noexcept;
    
    protected:

        // 摄像机的变换
        Transform m_Transform = {};
    
        // 视锥体属性
        float m_NearZ = 0.0f;
        float m_FarZ = 0.0f;
        float m_Aspect = 0.0f;
        float m_FovY = 0.0f;

        // 当前视口
        D3D12_VIEWPORT m_ViewPort = {};

    };
}

#endif