#include "Camera.h"

Camera::Camera()
    : m_position(0.0f, 0.0f, -5.0f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_yaw(0.0f)
    , m_pitch(0.3f)
    , m_distance(5.0f)
    , m_fov(XM_PIDIV4)
    , m_aspect(16.0f / 9.0f)
    , m_nearZ(0.1f)
    , m_farZ(1000.0f)
{
    UpdateVectors();
}

void Camera::SetPosition(const XMFLOAT3& pos) {
    m_position = pos;
    XMVECTOR posVec = XMLoadFloat3(&m_position);
    XMVECTOR targetVec = XMLoadFloat3(&m_target);
    m_distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(posVec, targetVec)));
}

void Camera::SetTarget(const XMFLOAT3& target) {
    m_target = target;
    UpdateVectors();
}

void Camera::SetAspect(float aspect) {
    m_aspect = aspect;
}

void Camera::SetFOV(float fov) {
    m_fov = fov;
}

void Camera::SetClipPlanes(float nearZ, float farZ) {
    m_nearZ = nearZ;
    m_farZ = farZ;
}

void Camera::Rotate(float yaw, float pitch) {
    m_yaw += yaw;
    m_pitch += pitch;
    m_pitch = XMMax(-XM_PIDIV2 + 0.1f, XMMin(XM_PIDIV2 - 0.1f, m_pitch));
    UpdateVectors();
}

void Camera::Zoom(float delta) {
    m_distance -= delta;
    m_distance = XMMax(0.5f, XMMin(100.0f, m_distance));
    UpdateVectors();
}

void Camera::Pan(float dx, float dy) {
    // Store temporaries in variables first (FIX for l-value error)
    XMFLOAT3 rightVec = GetRight();
    XMFLOAT3 upVec = GetUp();

    XMVECTOR right = XMLoadFloat3(&rightVec);
    XMVECTOR up = XMLoadFloat3(&upVec);
    XMVECTOR targetVec = XMLoadFloat3(&m_target);

    targetVec = XMVectorAdd(targetVec, XMVectorScale(right, dx * m_distance * 0.001f));
    targetVec = XMVectorAdd(targetVec, XMVectorScale(up, dy * m_distance * 0.001f));

    XMStoreFloat3(&m_target, targetVec);
    UpdateVectors();
}

void Camera::OrbitTarget(float deltaYaw, float deltaPitch) {
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;
    m_pitch = XMMax(-XM_PIDIV2 + 0.1f, XMMin(XM_PIDIV2 - 0.1f, m_pitch));
    UpdateVectors();
}

void Camera::LookAt(const XMFLOAT3& eye, const XMFLOAT3& target, const XMFLOAT3& up) {
    m_position = eye;
    m_target = target;
    m_up = up;

    XMVECTOR eyeVec = XMLoadFloat3(&eye);
    XMVECTOR targetVec = XMLoadFloat3(&target);
    m_distance = XMVectorGetX(XMVector3Length(XMVectorSubtract(eyeVec, targetVec)));
}

XMMATRIX Camera::GetViewMatrix() const {
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR target = XMLoadFloat3(&m_target);
    XMVECTOR up = XMLoadFloat3(&m_up);
    return XMMatrixLookAtLH(pos, target, up);
}

XMMATRIX Camera::GetProjectionMatrix() const {
    return XMMatrixPerspectiveFovLH(m_fov, m_aspect, m_nearZ, m_farZ);
}

XMFLOAT3 Camera::GetForward() const {
    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR target = XMLoadFloat3(&m_target);
    XMVECTOR forward = XMVector3Normalize(XMVectorSubtract(target, pos));
    XMFLOAT3 result;
    XMStoreFloat3(&result, forward);
    return result;
}

XMFLOAT3 Camera::GetRight() const {
    XMFLOAT3 forwardVec = GetForward();

    XMVECTOR forward = XMLoadFloat3(&forwardVec);
    XMVECTOR up = XMLoadFloat3(&m_up);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(forward, up));
    XMFLOAT3 result;
    XMStoreFloat3(&result, right);
    return result;
}

XMFLOAT3 Camera::GetUp() const {
    XMFLOAT3 forwardVec = GetForward();
    XMFLOAT3 rightVec = GetRight();

    XMVECTOR forward = XMLoadFloat3(&forwardVec);
    XMVECTOR right = XMLoadFloat3(&rightVec);
    XMVECTOR up = XMVector3Normalize(XMVector3Cross(right, forward));
    XMFLOAT3 result;
    XMStoreFloat3(&result, up);
    return result;
}

void Camera::Update(float deltaTime) {
    // Smooth camera movement can be added here
}

void Camera::Reset() {
    m_position = XMFLOAT3(0.0f, 0.0f, -5.0f);
    m_target = XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_up = XMFLOAT3(0.0f, 1.0f, 0.0f);
    m_yaw = 0.0f;
    m_pitch = 0.3f;
    m_distance = 5.0f;
    UpdateVectors();
}

void Camera::UpdateVectors() {
    // Calculate position from spherical coordinates
    float x = m_distance * cosf(m_pitch) * sinf(m_yaw);
    float y = m_distance * sinf(m_pitch);
    float z = m_distance * cosf(m_pitch) * cosf(m_yaw);

    XMVECTOR targetVec = XMLoadFloat3(&m_target);
    XMVECTOR offset = XMVectorSet(x, y, z, 0.0f);
    XMVECTOR posVec = XMVectorAdd(targetVec, offset);

    XMStoreFloat3(&m_position, posVec);
}