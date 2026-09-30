# Auto-Follow Camera

## Summary

**Auto-Follow Camera** is a Skyrim Special Edition / Anniversary Edition camera mod that automatically keeps the third-person camera behind the player character.

Its primary purpose is to make Skyrim playable without continuously controlling the camera with a second analogue stick or mouse.

The player controls movement normally. As the character changes direction, the camera smoothly rotates to follow behind them.

The mod does not alter combat, targeting, movement speed, animations, input mappings, or gameplay mechanics.

---

## Core behaviour

### Normal movement

When the player moves:

1. Detect the player's facing/movement direction.
2. Determine the desired camera position directly behind the player.
3. Smoothly rotate the camera toward that position.
4. Continue following as the player changes direction.

Example:

```text
      CAMERA
        ↓

      PLAYER
        ↑
```

If the player turns left:

```text
CAMERA
    ↘

      PLAYER
        ↖
```

The camera smoothly swings around until it is once again behind the player.

---

## One-stick operation

The mod should allow normal exploration using only the movement stick.

### Controller

**Left stick**
- Move forward/backward.
- Steer left/right.

**Right stick**
- Not required for normal movement.
- Optional behaviour configurable by the user.

### Keyboard

Keyboard movement should work identically.

For example:

- W = forward
- A/D = steer/change movement direction
- camera automatically follows

Mouse-look can optionally remain available.

---

# Camera behaviour

## Horizontal follow

The camera continuously attempts to align itself behind the player's current facing direction.

The camera should not snap instantly unless explicitly configured to do so.

Instead:

```text
Current camera yaw
        ↓
interpolate
        ↓
Player-facing direction + 180°
```

---

## Follow delay

A small configurable delay prevents the camera from feeling rigid.

Example behaviour:

**Fast**
- Camera closely tracks every turn.

**Normal**
- Small amount of trailing movement.

**Slow**
- Camera noticeably swings around after the player turns.

Default should favour comfort and predictability rather than cinematic movement.

---

## Dead zone

Small changes in character direction should not cause constant camera movement.

A configurable angular dead zone should prevent jitter.

Example:

```text
FollowDeadzone = 8°
```

If the difference between the camera and player direction is less than this value, the camera does nothing.

---

# Vertical camera

Automatic following should primarily affect **yaw**, not pitch.

Default behaviour:

- maintain a fixed or user-selected vertical camera angle;
- don't bob vertically with every movement;
- don't automatically look at terrain;
- don't suddenly pitch upward or downward.

Optional manual vertical look may remain available.

---

# Manual camera control

Users should be able to choose how manual camera input interacts with Auto-Follow.

## Mode 1 — Auto only

Right-stick/mouse horizontal camera movement is ignored during normal third-person gameplay.

This provides the purest one-stick experience.

---

## Mode 2 — Temporary override

Manual camera movement temporarily overrides Auto-Follow.

Example:

1. Player moves right stick.
2. Auto-Follow pauses.
3. Player looks around normally.
4. After configurable inactivity, Auto-Follow smoothly recentres behind the player.

Default delay:

```text
ManualOverrideDelay = 2.0 seconds
```

This should probably be the default mode.

---

## Mode 3 — Manual allowed

The player can freely rotate the camera.

Auto-Follow only activates once the player starts moving again.

---

# Recentering

Provide an optional **Recenter Camera** action.

When activated:

- camera immediately or quickly rotates behind the player;
- camera returns to the configured pitch;
- no movement input is required.

This action should be bindable but not mandatory.

---

# Stationary behaviour

When the player stops moving:

**Default:** camera remains where it currently is.

It should not continuously hunt for alignment while the player is stationary.

Optional setting:

```text
RecenterWhileStationary = false
```

If enabled, the camera gradually moves behind the player even while standing still.

---

# Backwards movement

Walking backwards should **not** rotate the camera 180°.

The camera should remain behind the player's facing direction.

Therefore:

```text
Player facing north
Player walks south/backwards

Camera remains south of player
looking north
```

Camera direction should primarily follow **character facing**, rather than raw movement vector.

---

# Strafing

Strafing should not cause the camera to swing sideways.

Example:

Player faces north and strafes east.

Camera remains behind the player, facing north.

This keeps camera behaviour predictable.

---

# Sprinting

No special camera behaviour required.

Optionally, follow responsiveness may increase slightly while sprinting.

This should be disabled by default.

---

# Sneaking

Same behaviour as normal movement.

No forced zoom or camera-position change.

---

# Weapons drawn

Auto-Follow should continue functioning normally.

The mod should not:

- add lock-on;
- change aiming;
- alter weapon targeting;
- alter crosshair behaviour;
- modify attack direction.

Those concerns should remain outside this mod's scope.

---

# Bows and spells

Manual aiming must remain possible.

Recommended behaviour:

When Skyrim enters an aiming state:

- temporarily suspend horizontal Auto-Follow;
- allow normal camera aiming;
- resume Auto-Follow after aiming ends.

Applicable examples:

- bows;
- crossbows;
- aimed spells;
- other gameplay states requiring precise camera control.

This prevents Auto-Follow from fighting the player's aim.

---

# First-person mode

The mod should do nothing in first person.

Switching to first person temporarily disables Auto-Follow.

Returning to third person resumes it.

---

# Mounted gameplay

Initial version:

**Disable Auto-Follow while mounted.**

Horse and dragon camera behaviour can be supported later if testing shows it is reliable.

---

# Conversations

Auto-Follow should suspend while:

- dialogue is active;
- scripted conversation cameras are active;
- killcams/cinematics are active.

Resume afterwards.

---

# Menus

No camera calculations should occur while menus pause gameplay.

---

# Camera collisions

Continue using Skyrim's normal camera collision handling.

The mod should not replace collision detection.

If Skyrim moves the camera closer because of a wall, Auto-Follow should continue controlling only its direction.

---

# Configuration

Keep configuration deliberately small.

Suggested settings:

```ini
[AutoFollow]

Enabled=true

FollowSpeed=5.0
FollowDeadzone=8.0

FixedPitch=false
Pitch=12.0

ManualCameraMode=TemporaryOverride
ManualOverrideDelay=2.0

RecenterWhileStationary=false

DisableWhileAiming=true
DisableInFirstPerson=true

RecenterKey=None
```

An MCM can expose the same settings if desired.

---

# Suggested MCM

## Auto-Follow Camera

**Enabled**
On / Off

**Follow speed**
Slow ←→ Fast

**Follow dead zone**
Small ←→ Large

**Manual camera**
- Disabled
- Temporary override
- Always allowed

**Resume after manual look**
0–5 seconds

**Vertical camera**
- Skyrim default
- Fixed angle

**Fixed vertical angle**
Slider

**Recenter while standing still**
On / Off

**Suspend while aiming**
On / Off

**Recenter key**
Bind

---

# Defaults

Recommended default configuration:

```text
Auto-Follow: ON
Follow speed: Medium
Dead zone: 8°
Manual camera: Temporary override
Override timeout: 2 seconds
Vertical camera: Skyrim default
Recenter stationary: OFF
Suspend while aiming: ON
```

This gives the player normal Skyrim camera control when they deliberately ask for it, while removing the need to continuously manage the camera.

---

# Accessibility goals

The mod should support players who:

- prefer one-stick navigation;
- find simultaneous movement and camera control difficult;
- use alternative controller configurations;
- have limited range of movement;
- play one-handed;
- find continuous camera correction tiring;
- simply prefer automatic third-person cameras.

The mod should not assume why somebody wants one-stick control.

---

# UX principles

## Predictable

The camera should always behave according to simple rules.

Turning the character left should reliably result in the camera moving left behind them.

---

## Gentle

Avoid sudden camera snaps by default.

---

## Low effort

The player shouldn't need to repeatedly press a recenter button.

The entire point is for recentering to happen automatically.

---

## Non-invasive

Do not modify:

- movement;
- combat;
- targeting;
- animation;
- AI;
- controls unrelated to camera movement.

---

# Compatibility goals

The mod should ideally:

- require SKSE;
- work with current Skyrim SE/AE runtime families;
- avoid editing vanilla records;
- avoid ESP/ESL files unless genuinely necessary;
- operate entirely through camera/input hooks where practical.

Avoid making SmoothCam or another camera overhaul a hard dependency.

Compatibility with major camera mods can be investigated separately.

---

# Conflicts

Likely conflicts include mods that directly control:

- third-person camera yaw;
- camera interpolation;
- camera state transitions;
- free-look behaviour;
- camera input.

If another mod owns the third-person camera each frame, only one mod should generally control automatic rotation.

---

# Technical architecture

Recommended implementation:

**SKSE plugin**

Responsibilities:

1. Observe player/camera state.
2. Detect whether Auto-Follow should currently run.
3. Obtain player facing yaw.
4. Calculate desired camera yaw.
5. Measure angular difference.
6. Ignore differences inside the dead zone.
7. Smoothly interpolate toward the target.
8. Detect manual camera input.
9. Temporarily suspend following when appropriate.

Pseudo-code:

```cpp
if (!enabled)
    return;

if (!IsThirdPerson())
    return;

if (ShouldSuspend())
    return;

if (ManualCameraActive()) {
    lastManualInputTime = now;
    return;
}

if (now - lastManualInputTime < manualOverrideDelay)
    return;

float playerYaw = GetPlayerYaw();
float desiredYaw = playerYaw + PI;

float difference =
    ShortestAngleDifference(cameraYaw, desiredYaw);

if (abs(difference) < followDeadzone)
    return;

cameraYaw =
    SmoothTowards(
        cameraYaw,
        desiredYaw,
        followSpeed,
        deltaTime
    );
```

The important part is using the **shortest angular path**, particularly around the 0°/360° boundary.

---

# MVP

Version 0.1 only needs:

1. Third-person detection.
2. Player-facing detection.
3. Automatic horizontal camera following.
4. Smooth interpolation.
5. Dead zone.
6. Suspension in first person.
7. Suspension while aiming.
8. Manual-look temporary override.
9. INI configuration.

Do **not** build MCM, horse support, elaborate presets, camera collision replacement, target lock, or combat assistance into the MVP.

---

# Testing scenarios

Test:

- walking straight;
- gradual turns;
- rapid 180° turns;
- strafing;
- walking backwards;
- sprinting;
- sneaking;
- standing still;
- drawing/sheathing a weapon;
- bow aiming;
- spell aiming;
- entering dialogue;
- switching first/third person;
- entering/exiting interiors;
- tight corridors;
- stairs;
- uneven terrain;
- camera collision with walls;
- controller unplug/reconnect;
- mouse + keyboard;
- controller;
- low and high frame rates.

---

# Acceptance criteria

The MVP is successful when:

1. A player can walk around Skyrim in third person without touching the camera stick during normal exploration.
2. The camera naturally settles behind the player's facing direction.
3. Strafing/backpedalling do not cause unwanted rotation.
4. Camera movement is smooth and doesn't jitter.
5. Precision aiming remains usable.
6. The player can manually look around without fighting the mod.
7. Auto-Follow reliably resumes afterwards.
8. First-person gameplay remains unchanged.
9. The mod does not alter movement or combat mechanics.

---

# Out of scope

For the initial mod:

- target lock;
- auto aim;
- enemy selection;
- movement assists;
- pathfinding;
- automatic interaction;
- combat changes;
- first-person camera changes;
- cinematic camera modes;
- photo mode;
- camera presets based on visual style.

The mod does one thing:

> **Move the character normally; let the camera follow automatically.**