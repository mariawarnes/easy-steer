#include "AutoFollow/CameraHooks.h"
#include "AutoFollow/Settings.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fstream>
#include <filesystem>
#include <memory>

namespace
{
void OnMessage(SKSE::MessagingInterface::Message* message)
{
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        AutoFollow::RegisterInput();
        break;
    case SKSE::MessagingInterface::kPreLoadGame:
    case SKSE::MessagingInterface::kPostLoadGame:
    case SKSE::MessagingInterface::kNewGame:
        AutoFollow::ResetCamera();
        break;
    default: break;
    }
}
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    SKSE::Init(skse);
    try {
        if (auto directory = SKSE::log::log_directory()) {
            *directory /= "EasySteer.log";
            auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(directory->string(), true);
            auto logger = std::make_shared<spdlog::logger>("EasySteer", std::move(sink));
            spdlog::set_default_logger(std::move(logger));
            spdlog::flush_on(spdlog::level::info);
        }
        SKSE::log::info("Easy Steer 1.0.0; runtime {}", skse->RuntimeVersion().string());
        if (std::filesystem::exists("Data/SKSE/Plugins/AutoFollowCamera.dll")) {
            SKSE::log::error("Remove/disable AutoFollowCamera.dll before enabling Easy Steer. Easy Steer did not install hooks to avoid duplicate steering.");
            return false;
        }
        std::ifstream ini("Data/SKSE/Plugins/EasySteer.ini");
        if (!ini) {
            ini.clear();
            ini.open("Data/SKSE/Plugins/AutoFollowCamera.ini");
            if (ini) SKSE::log::warn("Using legacy AutoFollowCamera.ini; migrate your settings to EasySteer.ini");
        }
        if (!ini) SKSE::log::warn("INI missing; using defaults");
        const auto configuration = AutoFollow::ReadSettings(ini);
        for (const auto& warning : configuration.warnings) SKSE::log::warn("{}", warning);
        if (!SKSE::GetMessagingInterface()->RegisterListener(OnMessage)) {
            SKSE::log::error("Could not register SKSE message listener");
            return false;
        }
        AutoFollow::InstallHooks(configuration.values);
        SKSE::log::info("Loaded; enabled={}", configuration.values.enabled);
        return true;
    } catch (const std::exception& error) {
        SKSE::log::error("Plugin initialization failed: {}", error.what());
        return false;
    }
}
