#include "AutoFollow/Settings.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <string_view>

namespace AutoFollow
{
namespace
{
std::string_view Trim(std::string_view value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    return value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
}
bool ParseBool(std::string_view text, bool& destination)
{
    if (text == "true" || text == "1") destination = true;
    else if (text == "false" || text == "0") destination = false;
    else return false;
    return true;
}
bool ParseFloat(std::string_view text, float& destination, float low, float high)
{
    if (text.empty()) return false;
    float value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() ||
        !std::isfinite(value) || value < low || value > high) return false;
    destination = value;
    return true;
}
}

SettingsResult ReadSettings(std::istream& input)
{
    SettingsResult result;
    auto& s = result.values;
    std::string line;
    bool inSection = false;
    unsigned lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.starts_with("\xEF\xBB\xBF")) line.erase(0, 3);
        auto text = Trim(std::string_view(line).substr(0, line.find_first_of(";#")));
        if (text.empty()) continue;
        if (text.front() == '[' && text.back() == ']') {
            const auto section = Trim(text.substr(1, text.size() - 2));
            inSection = section == "EasySteer" || section == "AutoFollow";
            continue;
        }
        if (!inSection) continue;
        const auto separator = text.find('=');
        const auto key = Trim(text.substr(0, separator));
        const auto value = separator == std::string_view::npos ? std::string_view{} : Trim(text.substr(separator + 1));
        bool valid = false;
        if (key == "Enabled") valid = ParseBool(value, s.enabled);
        else if (key == "TankControls") valid = ParseBool(value, s.tankControls);
        else if (key == "FirstPersonSteering") valid = ParseBool(value, s.firstPersonSteering);
        else if (key == "ThirdPersonSteering") valid = ParseBool(value, s.thirdPersonSteering);
        else if (key == "TurnSpeed") valid = ParseFloat(value, s.turnSpeedDegrees, 1.0F, 360.0F);
        else if (key == "SteeringDeadzone") valid = ParseFloat(value, s.steeringDeadzone, 0.0F, 0.9F);
        else if (key == "FollowSpeed") valid = ParseFloat(value, s.followSpeed, 0.0F, 30.0F);
        else if (key == "FollowDeadzone") valid = ParseFloat(value, s.deadzoneDegrees, 0.0F, 90.0F);
        else if (key == "FixedPitch") valid = ParseBool(value, s.fixedPitch);
        else if (key == "Pitch") valid = ParseFloat(value, s.pitchDegrees, -80.0F, 80.0F);
        else if (key == "ManualOverrideDelay") valid = ParseFloat(value, s.manualOverrideDelay, 0.0F, 5.0F);
        else if (key == "RecenterWhileStationary") valid = ParseBool(value, s.recenterWhileStationary);
        else if (key == "DisableWhileAiming") valid = ParseBool(value, s.disableWhileAiming);
        else if (key == "FollowBodyFacing") valid = ParseBool(value, s.followBodyFacing);
        else if (key == "DebugLogging") valid = ParseBool(value, s.debugLogging);
        else if (key == "DisableInFirstPerson") {
            // Legacy option governs orbit/follow only; first-person steering
            // is separately configured and never applies third-person offsets.
            valid = value == "true" || value == "1";
        } else if (key == "ManualCameraMode") {
            valid = true;
            if (value == "AutoOnly") s.manualMode = ManualMode::AutoOnly;
            else if (value == "TemporaryOverride") s.manualMode = ManualMode::TemporaryOverride;
            else if (value == "ManualAllowed") s.manualMode = ManualMode::ManualAllowed;
            else valid = false;
        } else if (key == "RecenterKey") {
            if (value.empty()) {
                result.warnings.push_back("Line " + std::to_string(lineNumber) + ": empty RecenterKey; retained default/previous value.");
                continue;
            }
            int code = -1;
            const auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), code);
            valid = value == "None" || (ec == std::errc{} && end == value.data() + value.size() && code >= 1 && code <= 281);
            if (valid) s.recenterKey = value == "None" ? -1 : code;
        }
        if (!valid) result.warnings.push_back("Line " + std::to_string(lineNumber) + ": invalid or unknown setting '" + std::string(key) + "'; retained default/previous value.");
    }
    return result;
}
}
