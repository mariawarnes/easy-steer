# Easy Steer

**One-Stick Movement and Auto-Follow Camera** · Version 1.0.0 · by mlw05.

Explore Skyrim using WASD or one movement stick. Left/right turns your character, forward/back moves along that heading, and the third-person camera follows. No ESP/ESL or Papyrus is required. Gameplay confirmed by the author on Steam Skyrim 1.7.104.0; other SE/AE runtimes are not independently validated.

## Perspective controls

FirstPersonSteering=true and ThirdPersonSteering=true are the defaults. Set either to false in the INI to restore vanilla controls in that perspective independently. Restart Skyrim after changing settings. With ThirdPersonSteering=false, this plugin also leaves third-person camera/look behaviour alone. Keep TankControls=true when using these independent switches.

First person turns the actual character before Skyrim builds its normal view. It has no trailing camera lag or orbit offsets; normal mouse/right-stick look and pitch remain available. Precision aiming and excluded gameplay states restore vanilla controls in both perspectives. POV transitions clear pending input so a held turn cannot leak into an unrelated camera state.

The author confirmed that the 0.2.1 prototype works in game. Version 1.0.0 retains its steering behaviour and introduces the Easy Steer name, release configuration and upgrade safeguards.

## Tank steering

TankControls=true is the new default. Left/right movement actions turn the actual character at a configurable rate; forward/back actions move along that heading. A/D alone turns in place, W+A/W+D curves while moving, and releasing movement retains the new facing. The camera trails actual heading and finishes settling after stopping. Keyboard remappings are respected. On a controller, left-stick X turns and Y moves forward/back, with a configurable axis deadzone.

Weapons-drawn movement remains eligible, including melee. Active bow/crossbow and directed-spell aiming temporarily restore vanilla movement and camera control. This aim boundary is mandatory in tank mode. Facing changes affect attack direction. This release deliberately changes movement-control semantics to satisfy the requested one-stick steering; it supersedes the original camera-only restriction in docs/SPEC.md. It does not add target lock, sidestep modifiers or new animations.

Manual camera controls retain the selected override policy. Looking around while stationary releases automatic camera ownership until movement/turning resumes (unless stationary recentering is enabled). Set TankControls=false to restore the legacy camera-only mode; FollowBodyFacing applies only in that mode. Disable other movement/camera controllers while testing.

Precision aiming and vertical look may still require mouse/right-stick input. Easy Steer simplifies navigation; it is not a complete one-input control scheme for every action.

## Behavior

- Defaults: speed 5, 8-degree dead zone, untouched pitch; tank follow finishes settling after stopping.
- A turn outside the dead zone starts following; it settles within 0.1 degree to prevent jitter at the boundary.
- TemporaryOverride: manual look suspends following for two seconds. Continued input restarts the timeout.
- AutoOnly: ignores horizontal mouse/right-stick look in eligible gameplay. Vertical look remains available. Excluded states and precision aiming retain normal controls.
- ManualAllowed: manual look suspends following until a new movement session. If already moving, stop and start again.
- Recenter: optional binding smoothly restores yaw and configured pitch, even while stationary. Manual input cancels a running recenter.
- Mounts, alternate camera states/targets, dialogue, menus that pause, loading, scripted camera control, furniture, death and knockdown are excluded.
- Aim suspension covers bow/crossbow attack states, bow zoom, non-self spell ready/charging/casting states, and staff casting. This conservative detection may suspend slightly longer than necessary.
- Suspensions, camera transitions and save loading discard transient tracking/override state. Resume starts from the current camera. No elapsed-time catch-up occurs through menus.

## Install

1. Install SKSE and Address Library for SKSE Plugins matching the game's executable runtime. Windows x64 SE/AE only; VR and original Skyrim are excluded.
2. If upgrading from Auto-Follow Camera, disable/uninstall the old mod and deploy its removal first. `AutoFollowCamera.dll` must no longer be present; Easy Steer refuses to install hooks while that DLL remains.
3. Install `EasySteer-1.0.0.zip` through Vortex or Mod Organizer 2, or place its `SKSE` folder under Skyrim's `Data`. The resulting files are `Data/SKSE/Plugins/EasySteer.dll` and `EasySteer.ini`. Enable the mod and deploy in Vortex. Start through SKSE. Check `EasySteer.log` under Documents/My Games/Skyrim Special Edition/SKSE.
4. Edit the INI and restart Skyrim to apply changes. `Enabled=false` installs no hooks. Remove the DLL and INI to uninstall; no persistent save data is written.

Do not enable two automatic rotation controllers together. SmoothCam, True Directional Movement camera auto-rotation, and free-look mods may compete for camera state. Compatibility with camera overhauls and individual SE/AE runtime versions is not yet validated.

## Configuration

See [the annotated INI](config/EasySteer.ini). Edit it in your mod manager's staging folder, then deploy and restart. Copy custom values from the old INI into the new `[EasySteer]` section. The legacy `[AutoFollow]` section is also accepted; if EasySteer.ini is absent, the plugin can read AutoFollowCamera.ini. Keys and values are case-sensitive. Invalid values warn in the log and retain the default or preceding valid duplicate value.

| Setting | Default | Meaning |
|---|---|---|
| Enabled | true | Install hooks at launch |
| TankControls | true | Enable steering system; false selects legacy camera-only mode |
| FirstPersonSteering | true | false restores normal first-person controls |
| ThirdPersonSteering | true | false restores normal third-person controls and camera |
| TurnSpeed | 120 | Degrees per second at full steering, 1-360 |
| SteeringDeadzone | 0.18 | Controller axis deadzone, 0-0.9 |
| FollowSpeed | 5.0 | Response in 1/seconds, 0–30; 0 explicitly selects instant snapping |
| FollowDeadzone | 8.0 | Start following outside this angle, 0–90 degrees |
| FixedPitch | false | Gradually restore configured pitch while following |
| Pitch | 12.0 | -80 to 80 degrees; positive looks down; also used by recenter |
| ManualCameraMode | TemporaryOverride | AutoOnly, TemporaryOverride, ManualAllowed |
| ManualOverrideDelay | 2.0 | 0–5 seconds of active updates |
| RecenterWhileStationary | false | Follow at rest |
| DisableWhileAiming | true | Suspend during precision aiming |
| FollowBodyFacing | true | Legacy camera-only mode body tracking; ignored in tank mode |
| DebugLogging | false | Log hook activity and headings every two seconds for troubleshooting |
| DisableInFirstPerson | true | Legacy orbit exclusion; does not disable FirstPersonSteering |
| RecenterKey | None | Optional decimal SKSE input code |

Recenter codes: keyboard scan codes 1–255, mouse buttons/wheel 256–265, controller 266–281 (273 is right-stick click). Choose a spare binding; its original game action is not consumed. Controller override detection uses Skyrim's processed look vector and existing dead zone.

## Build and test

Requires Git, CMake 3.25+, recent Visual Studio C++ desktop tools with C++23 support, Windows SDK, and bootstrapped vcpkg. Dependencies are pinned. Allow several GB of disk space for the CommonLib source build.

```powershell
$env:VCPKG_ROOT = 'C:/dev/vcpkg'
cmake --preset plugin
cmake --build --preset plugin --parallel 4
ctest --preset plugin
cmake --install build/plugin --config Release --component EasySteer --prefix dist/EasySteer
Compress-Archive -Path dist/EasySteer/* -DestinationPath dist/EasySteer-1.0.0.zip -Force
```

The distribution root corresponds to the game's Data directory. Dependencies link statically, using the dynamic Microsoft C++ runtime; Microsoft's x64 Visual C++ Redistributable may be needed.

Core tests require no Skyrim installation, vcpkg or SKSE headers:

```powershell
cmake --preset tests
cmake --build --preset tests
ctest --preset tests
```

## Architecture

The portable `FollowController` tracks world-space yaw while following and uses `1-exp(-speed*dt)` smoothing with shortest-angle differences. The frame delta is capped at 0.1 seconds to avoid jumps after stalls. Pitch is untouched by default.

`CameraHooks` chains third-person Begin, End, Update and UpdateRotation plus LookHandler's mouse/stick methods. It applies corrections at most once per ordinary camera update, before engine rotation construction and collision handling. Relative camera offsets `freeRotation.x/y` are written and `freeRotationEnabled` is temporarily enabled during rotation construction, then restored; tank steering updates actor heading through the engine setter before camera Update. Behind-player view yaw matches player yaw, so the relative target is zero, not 180 degrees. The camera's spatial position is behind the actor, but its view direction faces forward.

Tests cover angles, frame rates, dead-zone behavior, overrides, recentering, pitch, suspension, invalid data and INI parsing. They cannot verify in-game virtual dispatch, input routing, collision ordering or animation detection. See the [validation record](docs/TESTING.md). Internal AutoFollow namespace/core target names are retained; public plugin metadata, filenames, configuration, logs and packaging use Easy Steer. Third-party license notices are included in docs/licenses (docs/EasySteer/licenses in the main archive). The promotional thumbnail is AI-generated artwork, not a gameplay screenshot.

For a no-follow report, enable `DebugLogging=true`, restart Skyrim, then walk forward, left and right for a few seconds. The log reports camera-update/rotation-hook counts, whether hip nodes were found, logical actor yaw, body target yaw, camera yaw, manual input and any suspension reason. This distinguishes a missed engine hook from a missing facing signal. Disable the option after diagnosis.

API reference: [CommonLibSSE-NG pinned source](https://github.com/alandtse/CommonLibSSE-NG/tree/a898f469851c464d05137bb74b069dd234897643), including [ThirdPersonState](https://github.com/alandtse/CommonLibSSE-NG/blob/a898f469851c464d05137bb74b069dd234897643/include/RE/T/ThirdPersonState.h). Camera conventions were cross-checked against [True Directional Movement](https://github.com/ersh1/TrueDirectionalMovement); no code from that mod is included and it is not a dependency.
