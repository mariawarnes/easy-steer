#pragma once

#include <istream>
#include <string>
#include <vector>

namespace AutoFollow
{
enum class ManualMode { AutoOnly, TemporaryOverride, ManualAllowed };

struct Settings
{
    bool enabled = true;
    bool tankControls = true;
    bool firstPersonSteering = true;
    bool thirdPersonSteering = true;
    float turnSpeedDegrees = 120.0F;
    float steeringDeadzone = 0.18F;
    float followSpeed = 5.0F;
    float deadzoneDegrees = 8.0F;
    bool fixedPitch = false;
    float pitchDegrees = 12.0F;
    ManualMode manualMode = ManualMode::TemporaryOverride;
    float manualOverrideDelay = 2.0F;
    bool recenterWhileStationary = false;
    bool disableWhileAiming = true;
    bool followBodyFacing = true;
    bool debugLogging = false;
    int recenterKey = -1;
};

struct SettingsResult
{
    Settings values;
    std::vector<std::string> warnings;
};

// Invalid values retain their defaults (or the preceding valid duplicate value).
SettingsResult ReadSettings(std::istream& input);
}
