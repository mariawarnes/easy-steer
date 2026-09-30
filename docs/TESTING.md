# In-game acceptance checklist

Local validation (2026-09-30, Easy Steer 1.0.0): Windows x64 Release DLL built with MSVC 19.50; all 457 portable behaviour checks passed. New coverage includes analog deadzone scaling, finite-input guards, frame-rate-independent turn speed, turn-in-place camera following, idle settling at the new heading, manual override and suspension reset. These tests cannot validate Skyrim hook timing, actual locomotion/head alignment or combat.

Gameplay status: the user confirmed that 0.2.1 works, following earlier confirmation of third-person steering. Easy Steer 1.0.0 retains that steering behaviour and changes branding, default diagnostics and upgrade handling. The renamed DLL has been rebuilt and tested but has not been re-run in game during release preparation. Tests cover both default-on flags and independent opt-outs; first-person engine hook timing remains a live-game check. The 0.1.3 live trace confirmed that camera orbiting around animated hips left logical actor heading unchanged and returned to it when movement stopped. Tank mode now changes actual heading through the engine SetHeading method, remaps lateral input to steering, follows logical heading with weapons drawn and keeps the last follow state while idle. Body nodes=false is expected in tank mode.

Priority first-person tests: A/D turns the view in place; W+A/W+D moves along a curve; stopping holds the final view direction. Repeat with weapons drawn and precision aiming, then switch POV while holding/releasing movement. Test all four combinations of FirstPersonSteering and ThirdPersonSteering: each disabled perspective must retain normal strafing and look, without affecting the enabled perspective. Settings require a restart.

Third-person regression tests:

1. Hold A alone, then D alone: actor yaw in the log must change continuously; character turns in place with no sideways translation. Verify head and torso no longer retain the old direction.
2. Hold W+A/W+D: follow curved paths. Release all keys and verify camera settles behind the new heading, with no return to the initial direction.
3. Repeat with a melee weapon drawn, then attack and block. Confirm movement remains responsive and attacks face the new heading.
4. Aim a bow/crossbow or directed spell: normal aiming and lateral movement return; releasing aim resumes tank controls without stale turning.
5. Check S/backpedal, opposing keys and key releases; pause while holding a turn key, release it in the menu and resume. No stuck turn or movement.
6. Test remapped movement keys and controller left-stick X/Y, including drift/deadzone, diagonal inputs and device changes.
7. Manually orbit while stationary and moving; check each override mode, then first-person/menu/dialogue/save transitions.

| Scenario | Expected result |
|---|---|
| Walk and gradually turn facing | Camera trails and settles behind actor |
| Fast 180-degree turns and 0/360 crossing | Short path, no default snapping |
| Left/right and backpedal | Left/right turns actual actor; S moves backward along actor heading |
| Sprint/sneak | Same rules, no forced zoom |
| Stop moving | Tank follow finishes settling and retains new heading; legacy mode releases follow |
| Mouse/right-stick look | Manual control wins; timeout restarts on continued input |
| AutoOnly | Horizontal look ignored only in eligible third person; vertical look works |
| ManualAllowed | Look while moving waits for stop/start; look at rest resumes on movement |
| Recenter at rest or during override | Smooth yaw/pitch reset; manual input cancels it |
| Fixed pitch, stairs, uneven terrain | Smooth pitch, no terrain seeking; manual input wins |
| Draw/sheath melee weapon | No combat or crosshair changes; stable camera state |
| Bow draw/hold/release, crossbow aim/fire/reload | Precision aiming remains usable in every mode |
| Charged/held/released spells, dual casts, concentration, staff | Aim suspension and reliable return afterward |
| Self-target spell | No unnecessary aimed-spell suspension |
| First/third person and zoom transition | First person unchanged; smooth third-person resume |
| Horse/dragon mount/dismount | Mounted cameras untouched |
| Dialogue, scripted scene, killcam, furniture, death | Camera left to Skyrim |
| Pause, console, loading, save load, new game | No stale input or large catch-up on return |
| Interiors, walls, corridors | Vanilla collision behavior retained |
| Controller disconnect/reconnect and device switching | No persistent input lock or override |
| 30, 60, 144+ FPS and stalls | Similar steady-rate response; no large stall jump |
| Disabled/missing/malformed INI | No hooks when disabled; defaults and diagnostic warnings otherwise |
| Camera mods | Record conflicts; use only one automatic rotation owner |

Tank mode intentionally replaces lateral movement with steering. Full one-stick navigation remains an in-game acceptance requirement; portable camera tests alone do not establish it.

If follow does not run, check the plugin log and default INI with camera overhauls disabled. In a debugger, verify the runtime dispatches UpdateRotation during ordinary ThirdPersonState::Update and that weapon-state processing does not reset offsets afterward. Hook timing and spell/animation detection require actual game testing.
