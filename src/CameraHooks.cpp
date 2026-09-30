#include "AutoFollow/CameraHooks.h"
#include "AutoFollow/FollowController.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <SKSE/InputMap.h>
#include <cmath>
#include <chrono>
#include <utility>

namespace AutoFollow
{
namespace
{
Settings settings;
FollowController controller;
bool manualPending = false;
bool recenterPending = false;
bool insideUpdate = false;
bool rotationUpdated = false;
std::uint64_t updateCount = 0;
std::uint64_t rotationCount = 0;
Frame lastFrame;
bool lastCorrected = false;
bool bodyNodesFound = false;
bool forwardHeld = false, backHeld = false, leftHeld = false, rightHeld = false;
float steerInput = 0.0F;
float cameraYawBeforeUpdate = 0.0F;

void ResetSteering()
{
    forwardHeld = backHeld = leftHeld = rightHeld = false;
    steerInput = 0.0F;
}

bool Aiming(RE::PlayerCharacter* player)
{
    const auto attack = player->AsActorState()->GetAttackState();
    if (attack >= RE::ATTACK_STATE_ENUM::kBowDraw || RE::PlayerCamera::GetSingleton()->bowZoomedIn) return true;
    for (const auto hand : {RE::MagicSystem::CastingSource::kLeftHand, RE::MagicSystem::CastingSource::kRightHand}) {
        const auto* caster = player->GetMagicCaster(hand);
        // Conservatively include ready/charging spells as well as released casts.
        if (caster && caster->currentSpell && caster->state != RE::MagicCaster::State::kNone &&
            caster->currentSpell->GetDelivery() != RE::MagicSystem::Delivery::kSelf) return true;
    }
    // Staff animation graphs use iState=10 while casting.
    std::int32_t animationState = 0;
    if (player->GetGraphVariableInt("iState", animationState) && animationState == 10) {
        for (bool left : {false, true}) {
            const auto* object = player->GetEquippedObject(left);
            const auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr;
            if (weapon && weapon->IsStaff()) return true;
        }
    }
    return false;
}

const char* SuspensionReason(RE::TESCameraState* state = nullptr, bool allowFirstPerson = false)
{
    if (!settings.enabled) return "disabled";
    const auto* camera = RE::PlayerCamera::GetSingleton();
    auto* player = RE::PlayerCharacter::GetSingleton();
    auto* ui = RE::UI::GetSingleton();
    const auto* controls = RE::PlayerControls::GetSingleton();
    const auto* map = RE::ControlMap::GetSingleton();
    if (!camera || !player || !ui || !controls || !map) return "missing game state";
    const bool firstPerson = allowFirstPerson && settings.firstPersonSteering && camera->IsInFirstPerson();
    if (!camera->IsInThirdPerson() && !firstPerson) return "unsupported camera mode";
    if (camera->IsInThirdPerson() && settings.tankControls && !settings.thirdPersonSteering)
        return "third-person steering disabled";
    if (state && camera->currentState.get() != state) return "inactive camera state";
    if (camera->cameraTarget.get().get() != player) return "alternate camera target";
    if (ui->GameIsPaused() || ui->IsMenuOpen("Dialogue Menu") || ui->IsMenuOpen("Loading Menu") ||
        ui->IsMenuOpen("Main Menu") || ui->IsMenuOpen("Console") || ui->IsMenuOpen("RaceSex Menu")) return "menu or paused";
    if (controls->blockPlayerInput || controls->data.povScriptMode || controls->data.remapMode ||
        !map->IsLookingControlsEnabled() || !map->IsMovementControlsEnabled()) return "scripted or disabled controls";
    if (player->IsOnMount() || player->IsInKillMove() ||
        player->AsActorState()->GetLifeState() != RE::ACTOR_LIFE_STATE::kAlive ||
        player->AsActorState()->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal ||
        player->AsActorState()->GetKnockState() != RE::KNOCK_STATE_ENUM::kNormal) return "mount, cinematic, furniture or incapacitated";
    if (!firstPerson) {
        const auto* third = static_cast<RE::ThirdPersonState*>(camera->currentState.get());
        if (third->toggleAnimCam || third->stateNotActive) return "animated or inactive camera";
    }
    if ((settings.disableWhileAiming || settings.tankControls) && Aiming(player)) return "aiming";
    return nullptr;
}

bool Allowed(RE::ThirdPersonState* state = nullptr)
{
    return SuspensionReason(state) == nullptr;
}

bool SteeringAllowed()
{
    // Precision aim always gets vanilla movement, even when the optional
    // camera-only aim exclusion is disabled.
    return settings.tankControls && SuspensionReason(nullptr, true) == nullptr &&
        !Aiming(RE::PlayerCharacter::GetSingleton());
}

void ApplySteering()
{
    if (!SteeringAllowed()) { ResetSteering(); return; }
    auto* player = RE::PlayerCharacter::GetSingleton();
    const float delta = SteeringDelta(steerInput, settings.turnSpeedDegrees,
        RE::BSTimer::GetSingleton()->realTimeDelta);
    if (delta != 0.0F) {
        // The engine setter updates the actual heading used by both POVs.
        float heading = WrapAngle(player->GetAngleZ() + delta);
        if (heading < 0) heading += 2.0F * Pi;
        player->SetHeading(heading);
    }
}

float FollowFacing(RE::PlayerCharacter* player)
{
    bodyNodesFound = false;
    const float actorYaw = player->GetAngleZ();
    if (settings.tankControls) return actorYaw;
    // ActorState moved in newer AE runtimes; inherited inline getters use the
    // compile-time base offset and must go through the versioned accessor.
    // The actor's movement flags describe its animation state, not which key
    // the player holds. In vanilla side steps movingBack can also be set.
    // Read the processed forward/back input to preserve actual backpedalling.
    const auto* controls = RE::PlayerControls::GetSingleton();
    const bool backpedalling = controls && controls->data.moveInputVec.y < -0.1F;
    if (!settings.followBodyFacing || !player->IsMoving() || player->AsActorState()->IsWeaponDrawn() ||
        backpedalling) return actorYaw;
    auto* root = player->Get3D(false);
    if (!root) return actorYaw;
    static const RE::BSFixedString leftName{"NPC L Thigh [LThg]"};
    static const RE::BSFixedString rightName{"NPC R Thigh [RThg]"};
    const auto* left = root->GetObjectByName(leftName);
    const auto* right = root->GetObjectByName(rightName);
    if (!left || !right) return actorYaw;
    bodyNodesFound = true;
    return FacingFromHips(actorYaw, true, false, false,
        left->world.translate.x, left->world.translate.y,
        right->world.translate.x, right->world.translate.y);
}

void LogDiagnostics(RE::ThirdPersonState* state)
{
    if (!settings.debugLogging) return;
    static auto nextLog = std::chrono::steady_clock::time_point{};
    const auto now = std::chrono::steady_clock::now();
    if (now < nextLog) return;
    nextLog = now + std::chrono::seconds(2);
    const auto* reason = SuspensionReason(state);
    if (reason) {
        SKSE::log::info("Diagnostic: suspended={}; updates={}; rotation calls={}", reason, updateCount, rotationCount);
        return;
    }
    auto* player = RE::PlayerCharacter::GetSingleton();
    const auto& move = RE::PlayerControls::GetSingleton()->data.moveInputVec;
    SKSE::log::info("Diagnostic: updates={}; rotation calls={}; rotation this frame={}; moving={}; body nodes={}; actor yaw={:.1f}; target yaw={:.1f}; camera yaw={:.1f}; engine yaw={:.1f}; free rotation={}; offsets={}; manual={}; correction={}; weapon drawn={}; animation moving back={}; movement input=({:.2f}, {:.2f})",
        updateCount, rotationCount, rotationUpdated, lastFrame.moving, bodyNodesFound,
        player->GetAngleZ() * 180.0F / Pi, lastFrame.playerYaw * 180.0F / Pi,
        lastFrame.cameraYaw * 180.0F / Pi, state->currentYaw * 180.0F / Pi,
        state->GetFreeRotationMode(), state->applyOffsets, lastFrame.manualLook, lastCorrected,
        player->AsActorState()->IsWeaponDrawn(), bool(player->AsActorState()->actorState1.movingBack), move.x, move.y);
}

struct Hooks
{
    static void FirstBegin(RE::FirstPersonState* state)
    {
        ResetCamera();
        originalFirstBegin(state);
    }
    static void FirstEnd(RE::FirstPersonState* state)
    {
        ResetCamera();
        originalFirstEnd(state);
    }
    static void FirstUpdate(RE::FirstPersonState* state, RE::BSTSmartPointer<RE::TESCameraState>& next)
    {
        // First person uses vanilla view construction directly from actor
        // heading; no orbit offsets, smoothing or pitch correction apply.
        if (SuspensionReason(state, true) == nullptr) ApplySteering();
        else ResetCamera();
        originalFirstUpdate(state, next);
        manualPending = recenterPending = false;
        if (settings.debugLogging) {
            static auto nextLog = std::chrono::steady_clock::time_point{};
            const auto now = std::chrono::steady_clock::now();
            if (now >= nextLog) {
                nextLog = now + std::chrono::seconds(2);
                const auto* reason = SuspensionReason(state, true);
                auto* player = RE::PlayerCharacter::GetSingleton();
                SKSE::log::info("First-person steering: suspended={}; turn input={:.2f}; actor yaw={:.1f}",
                    reason ? reason : "none", steerInput, player ? player->GetAngleZ() * 180.0F / Pi : 0.0F);
            }
        }
    }
    static void Begin(RE::ThirdPersonState* state)
    {
        ResetCamera();
        originalBegin(state);
    }
    static void End(RE::ThirdPersonState* state)
    {
        ResetCamera();
        originalEnd(state);
    }
    static void Update(RE::ThirdPersonState* state, RE::BSTSmartPointer<RE::TESCameraState>& next)
    {
        insideUpdate = true;
        rotationUpdated = false;
        ++updateCount;
        lastCorrected = false;
        cameraYawBeforeUpdate = state->currentYaw;
        if (!Allowed(state)) ResetCamera();
        ApplySteering();
        originalUpdate(state, next);
        LogDiagnostics(state);
        insideUpdate = false;
        // Never carry an input impulse into an unrelated camera state/frame.
        manualPending = false;
        recenterPending = false;
    }
    static void UpdateRotation(RE::ThirdPersonState* state)
    {
        bool applyCorrection = false;
        // Apply before the engine builds the rotation/translation and resolves
        // collisions. No node transforms, actor angles or zoom fields are written.
        if (insideUpdate && !rotationUpdated) {
            rotationUpdated = true;
            ++rotationCount;
            if (Allowed(state) && state->applyOffsets) {
                auto* player = RE::PlayerCharacter::GetSingleton();
                lastFrame = Frame{
                    .allowed = true,
                    .moving = player->IsMoving() || (settings.tankControls && steerInput != 0.0F),
                    .manualLook = std::exchange(manualPending, false),
                    .recenter = std::exchange(recenterPending, false),
                    .holdWhenStationary = settings.tankControls,
                    .deltaSeconds = RE::BSTimer::GetSingleton()->realTimeDelta,
                    .playerYaw = FollowFacing(player),
                    .cameraYaw = settings.tankControls ? cameraYawBeforeUpdate : player->GetAngleZ() + state->freeRotation.x,
                    .cameraPitch = player->GetAngleX() + state->freeRotation.y
                };
                const auto result = controller.Update(settings, lastFrame);
                lastCorrected = result.yaw.has_value();
                applyCorrection = result.yaw.has_value() || result.pitch.has_value();
                if (result.yaw) state->freeRotation.x = WrapAngle(*result.yaw - player->GetAngleZ());
                if (result.pitch) state->freeRotation.y = *result.pitch - player->GetAngleX();
            } else {
                controller.Reset();
            }
        }
        // Vanilla disables free rotation during locomotion and then ignores
        // freeRotation entirely. Enable its offset branch only while building
        // this camera rotation, restoring the locomotion mode before returning.
        const bool freeRotationEnabled = state->freeRotationEnabled;
        if (applyCorrection) state->freeRotationEnabled = true;
        originalRotation(state);
        if (applyCorrection) state->freeRotationEnabled = freeRotationEnabled;
    }
    static void Mouse(RE::LookHandler* handler, RE::MouseMoveEvent* event, RE::PlayerControlsData* data)
    {
        const bool active = event && data && Allowed();
        const bool blockHorizontal = active && settings.manualMode == ManualMode::AutoOnly;
        // Mask only this look-handler invocation; all other consumers see the
        // original event. Vertical look continues through Skyrim's normal path.
        const auto x = event ? event->mouseInputX : 0;
        if (blockHorizontal) event->mouseInputX = 0;
        originalMouse(handler, event, data);
        if (event) event->mouseInputX = x;
        if (active && ((!blockHorizontal && x != 0) || event->mouseInputY != 0)) manualPending = true;
    }
    static void MoveButton(RE::MovementHandler* handler, RE::ButtonEvent* event, RE::PlayerControlsData* data)
    {
        originalMoveButton(handler, event, data);
        if (!event || !data) return;
        if (!SteeringAllowed()) { ResetSteering(); return; }
        const auto* events = RE::UserEvents::GetSingleton();
        const auto& name = event->QUserEvent();
        const bool pressed = event->IsPressed();
        if (name == events->forward) forwardHeld = pressed;
        else if (name == events->back) backHeld = pressed;
        else if (name == events->strafeLeft) leftHeld = pressed;
        else if (name == events->strafeRight) rightHeld = pressed;
        else return;
        steerInput = float(rightHeld) - float(leftHeld);
        // Semantic actions respect remapped keys. Remove lateral translation;
        // forward/back remains relative to the actor rather than camera orbit.
        data->moveInputVec = RE::NiPoint2(0.0F,
            float(forwardHeld || data->autoMove) - float(backHeld));
    }
    static void MoveStick(RE::MovementHandler* handler, RE::ThumbstickEvent* event, RE::PlayerControlsData* data)
    {
        originalMoveStick(handler, event, data);
        if (!event || !data || !event->IsLeft()) return;
        if (!SteeringAllowed()) { ResetSteering(); return; }
        // Raw stick axes are independent of the lagging camera's orientation.
        steerInput = SteeringAxis(event->xValue, settings.steeringDeadzone);
        data->moveInputVec = RE::NiPoint2(0.0F, SteeringAxis(event->yValue, settings.steeringDeadzone));
    }
    static void Stick(RE::LookHandler* handler, RE::ThumbstickEvent* event, RE::PlayerControlsData* data)
    {
        const bool active = event && data && event->IsRight() && Allowed();
        const bool blockHorizontal = active && settings.manualMode == ManualMode::AutoOnly;
        const float x = event ? event->xValue : 0.0F;
        if (blockHorizontal) event->xValue = 0.0F;
        originalStick(handler, event, data);
        if (event) event->xValue = x;
        // Use the game's processed look vector so its controller dead zone wins.
        if (active && (std::abs(data->lookInputVec.x) > 0.0001F ||
                       std::abs(data->lookInputVec.y) > 0.0001F)) manualPending = true;
    }
    static inline REL::Relocation<decltype(Begin)> originalBegin;
    static inline REL::Relocation<decltype(FirstBegin)> originalFirstBegin;
    static inline REL::Relocation<decltype(FirstEnd)> originalFirstEnd;
    static inline REL::Relocation<decltype(FirstUpdate)> originalFirstUpdate;
    static inline REL::Relocation<decltype(End)> originalEnd;
    static inline REL::Relocation<decltype(Update)> originalUpdate;
    static inline REL::Relocation<decltype(UpdateRotation)> originalRotation;
    static inline REL::Relocation<decltype(Mouse)> originalMouse;
    static inline REL::Relocation<decltype(Stick)> originalStick;
    static inline REL::Relocation<decltype(MoveButton)> originalMoveButton;
    static inline REL::Relocation<decltype(MoveStick)> originalMoveStick;
};

class RecenterInput final : public RE::BSTEventSink<RE::InputEvent*>
{
public:
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* events, RE::BSTEventSource<RE::InputEvent*>*) override
    {
        if (!events || settings.recenterKey < 0 || !Allowed()) return RE::BSEventNotifyControl::kContinue;
        for (auto* event = *events; event; event = event->next) {
            const auto* button = event->AsButtonEvent();
            if (!button || !button->IsDown()) continue;
            auto code = button->GetIDCode();
            switch (button->GetDevice()) {
            case RE::INPUT_DEVICE::kKeyboard: break;
            case RE::INPUT_DEVICE::kMouse: code += SKSE::InputMap::kMacro_MouseButtonOffset; break;
            case RE::INPUT_DEVICE::kGamepad: code = SKSE::InputMap::GamepadMaskToKeycode(code); break;
            default: continue;
            }
            if (code == static_cast<std::uint32_t>(settings.recenterKey)) recenterPending = true;
        }
        return RE::BSEventNotifyControl::kContinue;
    }
};
RecenterInput input;
}

void ResetCamera()
{
    controller.Reset();
    ResetSteering();
    manualPending = false;
    recenterPending = false;
}

void RegisterInput()
{
    static bool registered = false;
    if (!registered) {
        if (auto* manager = RE::BSInputDeviceManager::GetSingleton()) {
            manager->AddEventSink(&input);
            registered = true;
        }
    }
}

void InstallHooks(const Settings& configuration)
{
    settings = configuration;
    if (!settings.enabled) return;
    REL::Relocation<std::uintptr_t> third{RE::VTABLE_ThirdPersonState[0]};
    Hooks::originalBegin = third.write_vfunc(0x01, Hooks::Begin);
    Hooks::originalEnd = third.write_vfunc(0x02, Hooks::End);
    Hooks::originalUpdate = third.write_vfunc(0x03, Hooks::Update);
    Hooks::originalRotation = third.write_vfunc(0x0E, Hooks::UpdateRotation);
    REL::Relocation<std::uintptr_t> look{RE::VTABLE_LookHandler[0]};
    const auto stickSlot = REL::VersionShift(0x02, RE::PlayerInputHandler::kAE1799AddedVFuncCount, SKSE::RUNTIME_SSE_1_7_99);
    const auto mouseSlot = REL::VersionShift(0x03, RE::PlayerInputHandler::kAE1799AddedVFuncCount, SKSE::RUNTIME_SSE_1_7_99);
    Hooks::originalStick = look.write_vfunc(stickSlot, Hooks::Stick);
    Hooks::originalMouse = look.write_vfunc(mouseSlot, Hooks::Mouse);
    if (settings.tankControls) {
        if (settings.firstPersonSteering) {
            REL::Relocation<std::uintptr_t> first{RE::VTABLE_FirstPersonState[0]};
            Hooks::originalFirstBegin = first.write_vfunc(0x01, Hooks::FirstBegin);
            Hooks::originalFirstEnd = first.write_vfunc(0x02, Hooks::FirstEnd);
            Hooks::originalFirstUpdate = first.write_vfunc(0x03, Hooks::FirstUpdate);
            SKSE::log::info("First-person steering hooks installed");
        }
        REL::Relocation<std::uintptr_t> movement{RE::VTABLE_MovementHandler[0]};
        const auto buttonSlot = REL::VersionShift(0x04, RE::PlayerInputHandler::kAE1799AddedVFuncCount, SKSE::RUNTIME_SSE_1_7_99);
        Hooks::originalMoveStick = movement.write_vfunc(stickSlot, Hooks::MoveStick);
        Hooks::originalMoveButton = movement.write_vfunc(buttonSlot, Hooks::MoveButton);
        SKSE::log::info("Tank steering enabled; movement stick slot={}, button slot={}, turn speed={}", stickSlot, buttonSlot, settings.turnSpeedDegrees);
    }
    SKSE::log::info("Camera and look hooks installed; stick slot={}, mouse slot={}, body facing={}, diagnostics={}",
        stickSlot, mouseSlot, settings.followBodyFacing, settings.debugLogging);
}
}
