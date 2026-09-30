# Classic Tomb Raider controls and the Auto-Follow mismatch

Research date: 2026-09-28. No gameplay code changed in this research pass.

## Findings

The original PlayStation Tomb Raider manual assigns left/right to turning Lara, forward to running, and separate shoulder buttons to sidestepping. Look is a separate mode. Its combat description includes automatic target selection and camera tracking of the selected enemy. Drawing weapons does not simply disable camera behaviour.

Source: [original PlayStation manual, mirrored transcription](https://manuals.plus/m/7d5d2a5c21a4ba9894a5cf0b5acf55602543e707217896e5627fc9debf5ce115), sections In-Game Controls, Moving, Shooting and Looking Around. The transcription has inconsistencies in some face-button labels; only the consistent directional, sidestep and camera descriptions are used here.

TRX is a reverse-engineered PC implementation with enhancements, not the original PS1 source. Its explicit TR1 camera mode provides corroborating implementation evidence:

- Land movement updates a turn rate from left/right input.
- Chase camera computes its desired angle from character yaw plus a camera offset, then resolves placement and smooths movement.
- Combat and look have separate camera paths. Combat incorporates target angles when targeting, or character plus torso/head orientation otherwise.

Sources, pinned to commit b6b706f523ae229b0ded5c47e8140fa356e19ab2:

- [Land movement](https://github.com/LostArtefacts/TRX/blob/b6b706f523ae229b0ded5c47e8140fa356e19ab2/src/trx/game/lara/state/land.c)
- [Camera implementation: M_Chase, M_Combat, M_Look](https://github.com/LostArtefacts/TRX/blob/b6b706f523ae229b0ded5c47e8140fa356e19ab2/src/trx/game/camera/box_camera.c)

## What the Skyrim evidence establishes

The 0.1.3 log at 16:22:17 shows actor yaw 78.1 degrees, body target -20.1 degrees and engine camera yaw -7.4 degrees. At 16:22:19 movement has stopped and actor, target and camera yaw are all 78.1 degrees. This confirms a camera orbit around an unchanged character heading. It does not measure the head bone directly; the user's observed head direction is consistent with that unchanged heading.

`FollowFacing` samples hip positions without rotating the actual actor. It falls back to actor heading when stationary or when weapons are drawn. `FollowController::Update` also discards its tracked yaw and yields camera control when stationary by default. Together these permit the reported return to the original view. Weapons-drawn samples show `body nodes=false`, target equal to unchanged actor yaw and a nominal correction that has no new heading to follow.

The processed movement vector can also contain a backward component during the user's side-step test, so treating every negative Y value as explicit backward intent remains unsuitable for a reliable steering model.

## Recommended replacement design

Use actual character steering as the authoritative direction, with a camera that trails it. Remove animated hip orientation as the primary steering signal.

For a faithful tank-style mode, A/D or left-stick X controls turn rate; W/S or left-stick Y controls forward/back movement relative to the character. A/D alone turns in place. Forward plus a turn produces a curved path. A separate strafe modifier can preserve sidesteps. Mouse/free look requires an explicit policy so a camera orbit does not unexpectedly rewrite the steering heading.

The actor retains its new heading when movement stops. The camera may finish settling behind it without returning to a stored, pre-turn heading. Camera construction must retain Skyrim's collision and zoom handling.

Weapon-equipped locomotion must remain eligible for steering/following. Separate that from active bow/crossbow aiming and directed spell casting, where manual aiming should take priority. Changing facing also changes Skyrim's attack direction; simply deleting the weapon exclusion does not implement a coherent combat model. Do not copy Tomb Raider's target lock without a separate requirement.

The original specification asks for one-stick steering while also prohibiting movement/input/gameplay changes. Vanilla lateral movement plus camera-only offsets cannot satisfy both. The recommended design explicitly changes lateral controls into turning while preserving engine locomotion speeds, animations and attack execution as far as possible. Head/aim behaviour must then be verified with the real heading updated, before considering any direct head-tracking override.

## Acceptance cases for an implementation

1. A/D changes actual actor yaw; head/aim and body no longer retain the pre-turn direction during ordinary exploration.
2. W+A and W+D produce continuous turns; letting go preserves the final heading.
3. Idle camera does not snap to the pre-turn direction, including after manual look.
4. The same behaviour works with a melee weapon drawn; attacks use the intended new facing.
5. Bow/crossbow aiming and directed spells preserve controlled aiming and resume follow smoothly afterward.
6. Backpedalling, turn-in-place, controller dead zones and framerate-independent turn speed behave consistently.
7. First person, menus, dialogue, mounts, scripted scenes, save loading and camera transitions release the steering state cleanly.

These are design requirements, not claims that the current DLL implements them. The existing portable camera tests do not validate Skyrim input interception, actor rotation, head alignment or camera ownership at idle.
