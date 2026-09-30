# Easy Steer changelog

## 1.0.0 — 2026-09-30

First public release as **Easy Steer — One-Stick Movement and Auto-Follow Camera**.

- Character steering and camera following in third person; direct steering in first person.
- Both perspectives enabled by default, with independent switches for normal controls.
- Keyboard movement actions respect remapping; controller steering has an adjustable deadzone.
- Configurable turn speed, camera follow speed, manual-look override and optional recenter binding.
- Weapons-drawn steering; precision aiming temporarily uses normal controls.
- Renamed plugin, configuration and log to EasySteer.dll, EasySteer.ini and EasySteer.log.
- Diagnostics off by default. Legacy [AutoFollow] settings remain readable.
- Refuses to install hooks if AutoFollowCamera.dll is still present, preventing duplicate steering.

The steering behaviour is retained from the user-tested 0.2.1 prototype. The release rename and configuration migration were checked by rebuilding and automated tests.

## Prototype history

- 0.2.1: first-person steering and independent perspective switches.
- 0.2.0: actual character steering, turn in place, idle settling and weapons-drawn following.
- 0.1.x: experimental camera-only implementation, superseded by tank steering.
