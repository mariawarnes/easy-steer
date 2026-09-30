#pragma once
#include "AutoFollow/Settings.h"

namespace AutoFollow
{
void InstallHooks(const Settings& settings);
void RegisterInput();
void ResetCamera();
}
