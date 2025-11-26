#pragma once
#include <DirectXMath.h>

using namespace DirectX;

class Camera {
public:
    Camera();

    void SetPosition(const XMFLOAT3& pos);
    void SetTarget(const XMFLOAT3& target);
    void SetAspect(float aspect);
    void SetFOV(float fov);
    void SetClipPlanes(float nearZ, float farZ);

    void Rotate(float yaw, float pitch);
    void Zoom(float delta);
    void Pan(float dx, float dy);

    void LookAt(const XMFLOAT3& eye, const XMFLOAT3& target, const XMFLOAT3& up);
    void OrbitTarget(float deltaYaw, float deltaPitch);

    XMMATRIX GetViewMatrix() const;
    XMMATRIX GetProjectionMatrix() const;
    XMFLOAT3 GetPosition() const { return m_position; }
    XMFLOAT3 GetTarget() const { return m_target; }
    XMFLOAT3 GetForward() const;
    XMFLOAT3 GetRight() const;
    XMFLOAT3 GetUp() const;

    void Update(float deltaTime);
    void Reset();

private:
    XMFLOAT3 m_position;
    XMFLOAT3 m_target;
    XMFLOAT3 m_up;

    float m_yaw;
    float m_pitch;
    float m_distance;
    float m_fov;
    float m_aspect;
    float m_nearZ;
    float m_farZ;

    void UpdateVectors();
};