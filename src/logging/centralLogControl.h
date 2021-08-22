#ifndef DHT_CENTRALLOGCONTROL_H
#define DHT_CENTRALLOGCONTROL_H
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>

#if true
#define LOG_GET const char * _func_ = SPDLOG_FUNCTION;
#define LOG_SET(name) const char * _func_ = name;
#define LOG_CAPTURE _func_
#define LOG_FUNCTION _func_
#else
#define LOG_GET
#define LOG_SET(name)
#define LOG_CAPTURE _func_{0}
#define LOG_FUNCTION SPDLOG_FUNCTION
#endif

#define LOG_LOGGER_CALL(logger, level, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, LOG_FUNCTION}, level, __VA_ARGS__)

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#define LOG_LOGGER_TRACE(logger, ...) LOG_LOGGER_CALL(logger, spdlog::level::trace, __VA_ARGS__)
#define LOG_TRACE(...) LOG_LOGGER_TRACE(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define LOG_LOGGER_TRACE(logger, ...) (void)0
#define LOG_TRACE(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#define LOG_LOGGER_DEBUG(logger, ...) LOG_LOGGER_CALL(logger, spdlog::level::debug, __VA_ARGS__)
#define LOG_DEBUG(...) LOG_LOGGER_DEBUG(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define LOG_LOGGER_DEBUG(logger, ...) (void)0
#define LOG_DEBUG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#define LOG_LOGGER_INFO(logger, ...) LOG_LOGGER_CALL(logger, spdlog::level::info, __VA_ARGS__)
#define LOG_INFO(...) LOG_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define LOG_LOGGER_INFO(logger, ...) (void)0
#define LOG_INFO(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#define LOG_LOGGER_WARN(logger, ...) LOG_LOGGER_CALL(logger, spdlog::level::warn, __VA_ARGS__)
#define LOG_WARN(...) LOG_LOGGER_WARN(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define LOG_LOGGER_WARN(logger, ...) (void)0
#define LOG_WARN(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#define LOG_LOGGER_ERROR(logger, ...) LOG_LOGGER_CALL(logger, spdlog::level::err, __VA_ARGS__)
#define LOG_ERROR(...) LOG_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define LOG_LOGGER_ERROR(logger, ...) (void)0
#define LOG_ERROR(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#define LOG_LOGGER_CRITICAL(logger, ...) LOG_LOGGER_CALL(logger, spdlog::level::critical, __VA_ARGS__)
#define LOG_CRITICAL(...) LOG_LOGGER_CRITICAL(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#define LOG_LOGGER_CRITICAL(logger, ...) (void)0
#define LOG_CRITICAL(...) (void)0
#endif


#endif //DHT_CENTRALLOGCONTROL_H
