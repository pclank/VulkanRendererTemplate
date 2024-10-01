#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

/// <summary>
/// Cubic Interpolation for glm::vec3
/// </summary>
/// <param name="p0"></param>
/// <param name="p1"></param>
/// <param name="p2"></param>
/// <param name="p3"></param>
/// <param name="t"></param>
/// <returns></returns>
glm::vec3 CubicInterpolate(
    const glm::vec3& p0, const glm::vec3& p1,
    const glm::vec3& p2, const glm::vec3& p3,
    float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    glm::vec3 a = (3.0f * p1 - 3.0f * p2 + p3 - p0) / 2.0f;
    glm::vec3 b = (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) / 2.0f;
    glm::vec3 c = (p2 - p0) / 2.0f;
    glm::vec3 d = p1;

    return a * t3 + b * t2 + c * t + d;
}

/// <summary>
/// Cubic Interpolation for glm::quaternion
/// </summary>
/// <param name="q0"></param>
/// <param name="q1"></param>
/// <param name="q2"></param>
/// <param name="q3"></param>
/// <param name="t"></param>
/// <returns></returns>
glm::quat CubicInterpolate(
    const glm::quat& q0, const glm::quat& q1,
    const glm::quat& q2, const glm::quat& q3,
    float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    glm::quat a = (q3 - q2 + 3.0f * (q1 - q0)) / 2.0f;
    glm::quat b = (q0 + q2 - 2.0f * q1) / 2.0f;
    glm::quat c = (q2 - q0) / 2.0f;
    glm::quat d = q1;

    return (a * t3) + (b * t2) + (c * t) + d;
}
