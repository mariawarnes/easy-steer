#pragma once

#include "AutoFollow/Settings.h"
#include <optional>

namespace AutoFollow
{
constexpr float Pi = 3.14159265358979323846F;
constexpr float Radians(float degrees) { return degrees * Pi / 180.0F; }
float WrapAngle(float radians);
float SteeringAxis(float input, float deadzone);
float SteeringDelta(float axis, float speedDegrees, float deltaSeconds);

// Hip attachment positions give body orientation independently of head look and
// bone-axis conventions. Returns actor yaw for backward/combat/invalid poses.
float FacingFromHips(float actorYaw, bool moving, bool movingBack, bool weaponDrawn,
    float leftX, float leftY, float rightX, float rightY);

struct Frame
{
    bool allowed = false;
    bool moving = false;
    bool manualLook = false;
    bool recenter = false;
    bool holdWhenStationary = false;
    float deltaSeconds = 0.0F;
    float playerYaw = 0.0F;
    float cameraYaw = 0.0F;
    float cameraPitch = 0.0F;
};

struct Correction
{
    std::optional<float> yaw;
    std::optional<float> pitch;
};

class FollowController
{
public:
    Correction Update(const Settings& settings, const Frame& frame);
    void Reset();

private:
    float overrideRemaining_ = 0.0F;
    float lastYaw_ = 0.0F;
    bool haveYaw_ = false;
    bool following_ = false;
    bool waitForMovement_ = false;
    bool sawStationary_ = false;
    bool recentering_ = false;
};
}
