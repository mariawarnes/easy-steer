# Easy Steer 1.0.0 release validation

Prepared 2026-09-30.

- Windows x64 Release build succeeded with MSVC 19.50.
- CTest passed; 457 behaviour checks passed, including the new EasySteer INI section and legacy AutoFollow section compatibility.
- PE machine is AMD64 (0x8664). Exports: SKSEPlugin_Load, SKSEPlugin_Query, SKSEPlugin_Version.
- Generated plugin declaration identifies Easy Steer, version 1.0.0, author mlw05, using Address Library.
- DLL imports are Windows system libraries and the Microsoft Visual C++ runtime; dependency license notices are included.
- Release INI enables both perspectives and disables diagnostic sampling by default.
- User confirmed the 0.2.1 steering behaviour before this rename. The renamed 1.0.0 DLL was not re-run in Skyrim during packaging; full gameplay compatibility is not inferred from portable tests.
- Tested gameplay runtime: Steam Skyrim 1.7.104.0. Other SE/AE runtimes remain unverified.
- Thumbnail title and subtitle were visually checked; artwork uses the author's Sound Sight / Clear Sight visual style and the requested spaced EASY STEER title.

Important upgrade requirement: remove/disable AutoFollowCamera.dll before enabling EasySteer.dll. The new plugin detects the old DLL and declines to install duplicate hooks.
