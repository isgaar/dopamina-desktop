# Location: cmake/DopamineErrors.cmake
# Function: Source file manifest for error catalogs, structured logging,
# connection state machine, diagnostic probes, and system doctor components.
# All Rights Reserved isgaar

list(APPEND PROJECT_SOURCES
    src/core/errors/ErrorCode.cpp
    src/core/errors/ErrorCode.h
    src/core/errors/AppError.cpp
    src/core/errors/AppError.h
    src/core/errors/Redactor.cpp
    src/core/errors/Redactor.h
    src/core/logging/JsonLogger.cpp
    src/core/logging/JsonLogger.h
    src/core/connection/ConnectionReport.h
    src/core/connection/CircuitBreaker.h
    src/core/connection/ProbeRunner.cpp
    src/core/connection/ProbeRunner.h
    src/core/connection/ConnectionService.cpp
    src/core/connection/ConnectionService.h
    src/core/storage/WatchdogRunner.h
    src/core/platform/UiWatchdog.cpp
    src/core/platform/UiWatchdog.h
    src/core/diagnostics/SystemDoctor.cpp
    src/core/diagnostics/SystemDoctor.h
)
