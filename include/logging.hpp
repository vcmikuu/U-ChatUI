#pragma once

// Include the modloader header, which allows us to tell the modloader which mod
// this is, and the version etc.
#include "scotland2/shared/modloader.h"

// beatsaber-hook is a modding framework that lets us call functions and fetch
// field values from in the game It also allows creating objects, configuration,
// and importantly, hooking methods to modify their values
#include "beatsaber-hook/shared/utils/utils.h"
#include "paper2_scotland2/shared/logger.hpp"

/// @brief A logger, useful for printing debug messages
/// @return
static constexpr auto Logger = Paper::ConstLoggerContext(MOD_ID "_" VERSION);

#define INFO(str, ...) Paper::Logger::fmtLogTag<Paper::LogLevel::INF>(str, MOD_ID __VA_OPT__(, __VA_ARGS__))
//#define LOG_DEBUG(value...)
#define DEBUG(str, ...) Paper::Logger::fmtLogTag<Paper::LogLevel::DBG>(str, MOD_ID __VA_OPT__(, __VA_ARGS__))
//#define LOG_ERROR(value...)
#define ERROR(str, ...) Paper::Logger::fmtLogTag<Paper::LogLevel::ERR>(str, MOD_ID __VA_OPT__(, __VA_ARGS__))
#define WARNING(str, ...) Paper::Logger::fmtLogTag<Paper::LogLevel::WRN>(str, MOD_ID __VA_OPT__(, __VA_ARGS__))
#define CRITICAL(str, ...) Paper::Logger::fmtLogTag<Paper::LogLevel::CRIT>(str, MOD_ID __VA_OPT__(, __VA_ARGS__))


#define MOD_EXPORT __attribute__((visibility("default")))
#ifdef __cplusplus
#define MOD_EXPORT_FUNC extern "C" MOD_EXPORT
#else
#define MOD_EXPORT_FUNC MOD_EXPORT
#endif