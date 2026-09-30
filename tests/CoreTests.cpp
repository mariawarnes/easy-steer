#include "AutoFollow/FollowController.h"
#include "AutoFollow/Settings.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace AutoFollow;
namespace
{
int checks = 0;
void Check(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
bool Near(float a, float b, float tolerance = 0.0001F) { return std::abs(a - b) < tolerance; }
Frame Moving(float camera = 0.0F, float player = 0.0F)
{
    return {.allowed = true, .moving = true, .deltaSeconds = 1.0F / 60.0F,
        .playerYaw = player, .cameraYaw = camera};
}

void AnglesAndMotion()
{
    Check(Near(WrapAngle(Radians(2) - Radians(358)), Radians(4)), "shortest positive wrap");
    Check(Near(WrapAngle(Radians(358) - Radians(2)), Radians(-4)), "shortest negative wrap");
    Settings s;
    s.deadzoneDegrees = 0;
    FollowController c;
    auto f = Moving(Radians(358), Radians(2));
    auto result = c.Update(s, f);
    Check(result.yaw.has_value(), "follow produces yaw");
    Check(WrapAngle(*result.yaw - f.cameraYaw) > 0, "wrap follows short path");
    Check(std::abs(WrapAngle(*result.yaw - f.cameraYaw)) < Radians(4), "no overshoot");
    Check(!result.pitch, "default does not write pitch");
    c.Reset();
    f = Moving(0, Pi);
    result = c.Update(s, f);
    Check(std::isfinite(*result.yaw) && std::abs(*result.yaw) < Pi, "180 turn is finite and smooth");

    s.deadzoneDegrees = 8;
    c.Reset();
    f = Moving(0, Radians(7));
    Check(Near(*c.Update(s, f).yaw, 0), "dead zone prevents initial motion");
    f.playerYaw = Radians(10);
    f.cameraYaw = Radians(3); // engine's player-relative camera moves with actor
    Check(*c.Update(s, f).yaw > 0, "accumulated turn exits dead zone");

    for (int fps : {30, 60, 144, 240}) {
        c.Reset();
        s.deadzoneDegrees = 0;
        f = Moving(0, Radians(90));
        f.deltaSeconds = 1.0F / static_cast<float>(fps);
        for (int i = 0; i < fps; ++i) f.cameraYaw = *c.Update(s, f).yaw;
        const float expected = Radians(90) * (1.0F - std::exp(-5.0F));
        Check(Near(f.cameraYaw, expected), "one-second response is frame-rate independent");
    }
    c.Reset();
    f = Moving();
    c.Update(s, f);
    f.playerYaw = Radians(90);
    f.cameraYaw = Radians(90);
    result = c.Update(s, f);
    Check(*result.yaw > 0 && *result.yaw < Radians(10), "actor turn uses previous world yaw for gentle follow");
    c.Reset();
    f = Moving();
    for (int i = 0; i < 60; ++i) Check(Near(*c.Update(s, f).yaw, 0), "translation without facing change never rotates camera");
    s.followSpeed = 0;
    c.Reset();
    f = Moving(0, Radians(90));
    Check(Near(*c.Update(s, f).yaw, f.playerYaw), "explicit zero speed snaps");
}

void ManualAndSuspension()
{
    Settings s;
    FollowController c;
    auto f = Moving(Radians(90), 0);
    f.manualLook = true;
    Check(!c.Update(s, f).yaw, "manual input immediately wins");
    f.manualLook = false;
    f.deltaSeconds = 0.1F;
    for (int i = 0; i < 19; ++i) Check(!c.Update(s, f).yaw, "override remains active before timeout");
    c.Update(s, f);
    Check(c.Update(s, f).yaw.has_value(), "follow resumes after timeout");
    f.manualLook = true;
    c.Update(s, f);
    f.manualLook = false;
    for (int i = 0; i < 10; ++i) c.Update(s, f);
    f.manualLook = true;
    c.Update(s, f);
    f.manualLook = false;
    for (int i = 0; i < 19; ++i) Check(!c.Update(s, f).yaw, "fresh input restarts timeout");

    s.manualMode = ManualMode::ManualAllowed;
    c.Reset();
    f.manualLook = true;
    c.Update(s, f);
    f.manualLook = false;
    for (int i = 0; i < 30; ++i) Check(!c.Update(s, f).yaw, "manual allowed waits for a new movement session");
    f.moving = false;
    Check(!c.Update(s, f).yaw, "stopping arms movement resume");
    f.moving = true;
    Check(c.Update(s, f).yaw.has_value(), "next movement resumes manual-allowed mode");
    c.Reset();
    f.moving = false;
    f.manualLook = true;
    c.Update(s, f);
    f.manualLook = false;
    f.moving = true;
    Check(c.Update(s, f).yaw.has_value(), "stationary manual look resumes on first movement");

    f.allowed = false;
    f.recenter = true;
    Check(!c.Update(s, f).yaw, "recenter cannot bypass suspended states");
    f.allowed = true;
    f.recenter = false;
    s.enabled = false;
    Check(!c.Update(s, f).yaw, "disabled is a hard boundary");
    s.enabled = true;
    f.moving = false;
    Check(!c.Update(s, f).yaw, "stationary default is untouched");
    s.recenterWhileStationary = true;
    Check(c.Update(s, f).yaw.has_value(), "stationary option follows");
    c.Reset();
    s.recenterWhileStationary = false;
    f.moving = true;
    f.deltaSeconds = std::numeric_limits<float>::quiet_NaN();
    Check(!c.Update(s, f).yaw, "NaN time is rejected");
    f.deltaSeconds = -1;
    Check(!c.Update(s, f).yaw, "negative time is rejected");
    f.deltaSeconds = 60;
    auto result = c.Update(s, f);
    Check(*result.yaw > Radians(50), "long stalls are clamped to avoid snaps");
    f.cameraYaw = std::numeric_limits<float>::infinity();
    Check(!c.Update(s, f).yaw, "non-finite angles are rejected");
}

void PitchAndRecenter()
{
    Settings s;
    FollowController c;
    auto f = Moving(Radians(90), 0);
    f.moving = false;
    f.recenter = true;
    auto result = c.Update(s, f);
    Check(result.yaw && result.pitch, "recenter works stationary and restores pitch");
    Check(*result.pitch > 0 && *result.pitch < Radians(s.pitchDegrees), "recenter pitch is smooth");
    f.recenter = false;
    for (int i = 0; i < 180; ++i) {
        if (result.yaw) f.cameraYaw = *result.yaw;
        if (result.pitch) f.cameraPitch = *result.pitch;
        result = c.Update(s, f);
    }
    Check(std::abs(f.cameraYaw) < Radians(0.1F), "recenter completes yaw");
    Check(Near(f.cameraPitch, Radians(s.pitchDegrees), Radians(0.1F)), "recenter completes pitch");
    Check(!result.yaw && !result.pitch, "finished recenter returns to stationary behavior");
    s.fixedPitch = true;
    f.moving = true;
    Check(c.Update(s, f).pitch.has_value(), "fixed pitch applies while following");
    f.manualLook = true;
    Check(!c.Update(s, f).pitch, "manual look overrides fixed pitch");
    f.recenter = true;
    Check(c.Update(s, f).yaw.has_value(), "explicit recenter cancels override");
    s.manualMode = ManualMode::AutoOnly;
    f.recenter = false;
    Check(!c.Update(s, f).pitch, "auto-only still respects manual vertical look");
}

void Configuration()
{
    std::istringstream empty;
    const auto defaults = ReadSettings(empty);
    Check(defaults.values.enabled && Near(defaults.values.followSpeed, 5), "missing config defaults");
    Check(defaults.warnings.empty(), "missing config is valid");
    std::istringstream valid("\xEF\xBB\xBF[AutoFollow]\r\nFollowSpeed=7.5 ; comment\r\nFollowDeadzone=10\nFixedPitch=true\nPitch=-20\nManualCameraMode=AutoOnly\nManualOverrideDelay=0.5\nRecenterWhileStationary=1\nDisableWhileAiming=0\nRecenterKey=273\nDisableInFirstPerson=true\n");
    const auto parsed = ReadSettings(valid);
    Check(parsed.warnings.empty(), "valid INI including BOM and CRLF");
    Check(Near(parsed.values.followSpeed, 7.5F) && parsed.values.fixedPitch && Near(parsed.values.pitchDegrees, -20), "numeric and boolean settings");
    Check(parsed.values.manualMode == ManualMode::AutoOnly && parsed.values.recenterKey == 273, "mode and controller binding");
    std::istringstream invalid("[Other]\nFollowSpeed=19\n[AutoFollow]\nFollowSpeed=nan\nPitch=inf\nFollowDeadzone=-1\nManualOverrideDelay=9\nEnabled=yes\nManualCameraMode=typo\nRecenterKey=282\nDisableInFirstPerson=false\nUnknown=1\nFollowSpeed=12junk\n");
    const auto rejected = ReadSettings(invalid);
    Check(rejected.warnings.size() == 10, "all invalid settings warn");
    Check(Near(rejected.values.followSpeed, 5) && Near(rejected.values.pitchDegrees, 12), "invalid numbers retain defaults");
    Check(rejected.values.enabled && rejected.values.recenterKey == -1, "invalid options retain defaults");
    std::istringstream duplicate("[AutoFollow]\nFollowSpeed=3\nFollowSpeed=-1\nRecenterKey=42\nRecenterKey=None\n");
    const auto repeated = ReadSettings(duplicate);
    Check(Near(repeated.values.followSpeed, 3) && repeated.values.recenterKey == -1, "duplicates preserve last valid value");
    std::istringstream blanks("[AutoFollow]\nPitch=\nRecenterKey=\n");
    const auto blankValues = ReadSettings(blanks);
    Check(blankValues.warnings.size() == 2, "empty numeric values warn without parsing null ranges");
    Check(Near(blankValues.values.pitchDegrees, 12) && blankValues.values.recenterKey == -1, "empty values preserve defaults");
    std::istringstream options("[AutoFollow]\nFollowBodyFacing=false\nDebugLogging=true\n");
    const auto bodyOptions = ReadSettings(options);
    Check(bodyOptions.warnings.empty() && !bodyOptions.values.followBodyFacing && bodyOptions.values.debugLogging, "body-facing and diagnostic options parse");
}

void VisibleBodyFacing()
{
    // Regression: logical actor/head stays at zero but the walking body turns.
    const float left = FacingFromHips(0, true, false, false, 0, -10, 0, 10);
    const float right = FacingFromHips(0, true, false, false, 0, 10, 0, -10);
    Check(Near(left, -Pi / 2), "left-facing animated body is detected despite unchanged logical yaw");
    Check(Near(right, Pi / 2), "right-facing animated body is detected despite unchanged logical yaw");
    Settings settings;
    FollowController camera;
    auto frame = Moving(0, left);
    auto result = camera.Update(settings, frame);
    Check(result.yaw && *result.yaw < 0 && *result.yaw > left, "camera follows visible left turn smoothly");
    camera.Reset();
    frame.playerYaw = right;
    result = camera.Update(settings, frame);
    Check(result.yaw && *result.yaw > 0 && *result.yaw < right, "camera follows visible right turn smoothly");
    Check(Near(FacingFromHips(0, true, true, false, 0, -10, 0, 10), 0), "backwards input preserves actor heading");
    Check(Near(FacingFromHips(0, true, false, true, 0, -10, 0, 10), 0), "combat strafing ignores animated pelvis turn");
    Check(Near(FacingFromHips(0, false, false, false, 0, -10, 0, 10), 0), "idle pose does not steer camera");
    Check(Near(FacingFromHips(0, true, false, false, 10, 0, -10, 0), 0), "rear-facing animation never flips camera 180 degrees");
    Check(Near(FacingFromHips(0, true, false, false, 1, 1, 1, 1), 0), "collapsed hip projection falls back safely");
    Check(Near(FacingFromHips(0, true, false, false, 0, 0, std::numeric_limits<float>::quiet_NaN(), 1), 0), "invalid pose falls back safely");
    const float yaw = Radians(359);
    const float acrossX = 20 * std::cos(yaw);
    const float acrossY = -20 * std::sin(yaw);
    const float wrapped = FacingFromHips(yaw, true, false, false, 5000, 9000, 5000 + acrossX, 9000 + acrossY);
    Check(std::abs(WrapAngle(wrapped - yaw)) < Radians(0.01F), "body heading handles wrap and world translation");
}
}

void TankSteering()
{
    Check(Near(SteeringAxis(0.1F, 0.18F), 0), "controller drift does not turn");
    Check(Near(SteeringAxis(-1, 0.18F), -1), "full left stick turns left");
    Check(Near(SteeringAxis(1, 0.18F), 1), "full right stick turns right");
    Check(Near(SteeringAxis(0.59F, 0.18F), 0.5F), "analog steering scales beyond deadzone");
    Check(Near(SteeringAxis(std::numeric_limits<float>::quiet_NaN(), 0.18F), 0), "invalid stick is neutral");
    Check(Near(SteeringDelta(1, 120, 2), Radians(12)), "long stall cannot jump heading");
    Check(Near(SteeringDelta(1, 120, -1), 0), "invalid time does not turn");
    for (int fps : {30, 60, 144, 240}) {
        float heading = 0;
        for (int i = 0; i < fps; ++i) heading += SteeringDelta(1, 120, 1.0F / fps);
        Check(Near(heading, Radians(120)), "tank turning is frame-rate independent");
    }
    Settings s;
    FollowController controller;
    auto frame = Moving();
    frame.holdWhenStationary = true;
    frame.deltaSeconds = 1.0F / 60;
    for (int i = 0; i < 60; ++i) {
        frame.playerYaw = WrapAngle(frame.playerYaw + SteeringDelta(-1, 120, frame.deltaSeconds));
        const auto result = controller.Update(s, frame);
        Check(result.yaw.has_value(), "turn-in-place produces camera follow");
        frame.cameraYaw = *result.yaw;
    }
    Check(Near(frame.playerYaw, Radians(-120)), "actual heading retains completed turn");
    Check(frame.cameraYaw > frame.playerYaw && frame.cameraYaw < 0, "camera trails actual left turn");
    frame.moving = false;
    for (int i = 0; i < 180; ++i) {
        const auto result = controller.Update(s, frame);
        Check(result.yaw.has_value(), "idle retains ownership after steering");
        frame.cameraYaw = *result.yaw;
    }
    Check(std::abs(WrapAngle(frame.cameraYaw - frame.playerYaw)) < Radians(0.1F), "stopping settles at new heading instead of original direction");
    frame.manualLook = true;
    Check(!controller.Update(s, frame).yaw, "manual look releases idle camera ownership");
    frame.manualLook = false;
    for (int i = 0; i < 180; ++i) controller.Update(s, frame);
    Check(!controller.Update(s, frame).yaw, "manual idle orbit remains until movement resumes");
    frame.moving = true;
    Check(controller.Update(s, frame).yaw.has_value(), "movement resumes follow after manual orbit");
    frame.allowed = false;
    Check(!controller.Update(s, frame).yaw, "aim or excluded state releases camera");
    frame.allowed = true;
    frame.moving = false;
    Check(!controller.Update(s, frame).yaw, "suspension discards stale idle heading");
    std::istringstream ini("[AutoFollow]\nTankControls=true\nTurnSpeed=95\nSteeringDeadzone=0.25\n");
    const auto parsed = ReadSettings(ini);
    Check(parsed.warnings.empty() && parsed.values.tankControls && Near(parsed.values.turnSpeedDegrees, 95) && Near(parsed.values.steeringDeadzone, 0.25F), "tank configuration parses");
    std::istringstream bad("[AutoFollow]\nTurnSpeed=nan\nSteeringDeadzone=1\nTankControls=maybe\n");
    Check(ReadSettings(bad).warnings.size() == 3, "unsafe steering configuration rejected");
    std::istringstream empty;
    const auto defaults = ReadSettings(empty).values;
    Check(defaults.firstPersonSteering && defaults.thirdPersonSteering, "both perspectives steer by default");
    std::istringstream firstOff("[AutoFollow]\nFirstPersonSteering=false\n");
    const auto firstOnly = ReadSettings(firstOff).values;
    Check(!firstOnly.firstPersonSteering && firstOnly.thirdPersonSteering, "first-person opt-out preserves third-person steering");
    std::istringstream thirdOff("[AutoFollow]\nThirdPersonSteering=false\n");
    const auto thirdOnly = ReadSettings(thirdOff).values;
    Check(thirdOnly.firstPersonSteering && !thirdOnly.thirdPersonSteering, "third-person opt-out preserves first-person steering");
    std::istringstream bothOff("[AutoFollow]\nFirstPersonSteering=false\nThirdPersonSteering=false\n");
    const auto neither = ReadSettings(bothOff);
    Check(neither.warnings.empty() && !neither.values.firstPersonSteering && !neither.values.thirdPersonSteering, "both perspectives can use vanilla controls");
    std::istringstream invalidPerspectives("[AutoFollow]\nFirstPersonSteering=maybe\nThirdPersonSteering=2\n");
    const auto invalid = ReadSettings(invalidPerspectives);
    Check(invalid.warnings.size() == 2 && invalid.values.firstPersonSteering && invalid.values.thirdPersonSteering, "invalid perspective flags retain defaults");
    std::istringstream branded("[EasySteer]\nTurnSpeed=95\nFirstPersonSteering=false\nDebugLogging=false\n");
    const auto renamed = ReadSettings(branded);
    Check(renamed.warnings.empty() && Near(renamed.values.turnSpeedDegrees, 95) && !renamed.values.firstPersonSteering, "EasySteer release section loads settings");
    Check(!renamed.values.debugLogging, "release configuration disables diagnostics");
    std::istringstream legacy("[AutoFollow]\nTurnSpeed=95\nFirstPersonSteering=false\n");
    const auto migrated = ReadSettings(legacy);
    Check(migrated.warnings.empty() && Near(migrated.values.turnSpeedDegrees, renamed.values.turnSpeedDegrees) && migrated.values.firstPersonSteering == renamed.values.firstPersonSteering, "legacy INI section remains compatible");
}

int main()
{
    try {
        AnglesAndMotion();
        ManualAndSuspension();
        PitchAndRecenter();
        Configuration();
        VisibleBodyFacing();
        TankSteering();
        std::cout << "Passed " << checks << " behavior checks\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAILED after " << checks << " checks: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
