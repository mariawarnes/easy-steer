#include "AutoFollow/FollowController.h"
#include <algorithm>
#include <cmath>

namespace AutoFollow
{
float WrapAngle(float radians)
{
    return std::remainder(radians, 2.0F * Pi);
}

float SteeringAxis(float input, float deadzone)
{
    if (!std::isfinite(input) || !std::isfinite(deadzone) || deadzone < 0 || deadzone >= 1) return 0;
    const float magnitude = std::clamp(std::abs(input), 0.0F, 1.0F);
    if (magnitude <= deadzone) return 0;
    return std::copysign((magnitude - deadzone) / (1 - deadzone), input);
}

float SteeringDelta(float axis, float speedDegrees, float deltaSeconds)
{
    if (!std::isfinite(axis) || !std::isfinite(speedDegrees) || !std::isfinite(deltaSeconds) ||
        deltaSeconds <= 0 || speedDegrees <= 0) return 0;
    return std::clamp(axis, -1.0F, 1.0F) * Radians(speedDegrees) * std::min(deltaSeconds, 0.1F);
}

float FacingFromHips(float actorYaw, bool moving, bool movingBack, bool weaponDrawn,
    float leftX, float leftY, float rightX, float rightY)
{
    if (!moving || movingBack || weaponDrawn) return actorYaw;
    const float acrossX = rightX - leftX;
    const float acrossY = rightY - leftY;
    if (!std::isfinite(acrossX) || !std::isfinite(acrossY) ||
        acrossX * acrossX + acrossY * acrossY < 0.01F) return actorYaw;
    // Skyrim heading is clockwise from +Y. Right-minus-left gives the lateral
    // axis; its perpendicular (-acrossY, acrossX) points along body forward.
    const float bodyYaw = std::atan2(-acrossY, acrossX);
    if (std::abs(WrapAngle(bodyYaw - actorYaw)) > Radians(120.0F)) return actorYaw;
    return bodyYaw;
}

void FollowController::Reset()
{
    *this = FollowController{};
}

Correction FollowController::Update(const Settings& settings, const Frame& frame)
{
    if (!settings.enabled || !frame.allowed) {
        Reset();
        return {};
    }
    if (!std::isfinite(frame.deltaSeconds) || frame.deltaSeconds <= 0.0F ||
        !std::isfinite(frame.playerYaw) || !std::isfinite(frame.cameraYaw) ||
        !std::isfinite(frame.cameraPitch)) {
        haveYaw_ = false;
        return {};
    }
    // Long stalls must not cause an abrupt catch-up or expire an override at once.
    const float dt = std::min(frame.deltaSeconds, 0.1F);
    if (frame.manualLook) {
        haveYaw_ = false;
        following_ = false;
        recentering_ = false;
        if (settings.manualMode == ManualMode::TemporaryOverride) {
            overrideRemaining_ = settings.manualOverrideDelay;
        } else if (settings.manualMode == ManualMode::ManualAllowed) {
            waitForMovement_ = true;
            sawStationary_ = !frame.moving;
        }
        // Also respect vertical manual look in AutoOnly mode.
        if (!frame.recenter) {
            return {};
        }
    }
    if (frame.recenter) {
        overrideRemaining_ = 0.0F;
        waitForMovement_ = false;
        recentering_ = true;
        following_ = true;
        haveYaw_ = false;
    }
    if (overrideRemaining_ > 0.0F) {
        overrideRemaining_ = std::max(0.0F, overrideRemaining_ - dt);
        haveYaw_ = false;
        return {};
    }
    if (waitForMovement_) {
        if (!frame.moving) {
            sawStationary_ = true;
        }
        if (frame.moving && sawStationary_) {
            waitForMovement_ = false;
        } else {
            haveYaw_ = false;
            return {};
        }
    }
    if (!frame.moving && !settings.recenterWhileStationary && !recentering_ &&
        !(frame.holdWhenStationary && haveYaw_)) {
        haveYaw_ = false;
        following_ = false;
        return {};
    }

    const float yaw = haveYaw_ ? lastYaw_ : frame.cameraYaw;
    const float error = WrapAngle(frame.playerYaw - yaw);
    if (std::abs(error) > Radians(settings.deadzoneDegrees)) {
        following_ = true;
    }
    // Hysteresis: once outside the dead zone, settle behind the player instead
    // of repeatedly starting/stopping on the dead-zone boundary.
    const float speed = recentering_ ? std::max(settings.followSpeed, 12.0F) : settings.followSpeed;
    const float alpha = settings.followSpeed == 0.0F ? 1.0F : -std::expm1(-speed * dt);
    Correction result;
    const float newYaw = following_ ? WrapAngle(yaw + error * alpha) : WrapAngle(yaw);
    // Keeping the last world-space yaw compensates for changes in actor yaw;
    // the adapter converts it back to a relative camera-only offset.
    result.yaw = newYaw;
    lastYaw_ = newYaw;
    haveYaw_ = true;
    const bool yawSettled = std::abs(WrapAngle(frame.playerYaw - newYaw)) < Radians(0.1F);
    if (yawSettled) {
        following_ = false;
    }
    bool pitchSettled = true;
    if (settings.fixedPitch || recentering_) {
        const float target = Radians(settings.pitchDegrees);
        result.pitch = frame.cameraPitch + (target - frame.cameraPitch) * alpha;
        pitchSettled = std::abs(target - *result.pitch) < Radians(0.1F);
    }
    if (recentering_ && yawSettled && pitchSettled) {
        recentering_ = false;
    }
    return result;
}
}
