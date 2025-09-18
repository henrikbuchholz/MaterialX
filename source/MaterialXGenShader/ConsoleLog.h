//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#ifndef MATERIALX_CONSOLELOG_H
#define MATERIALX_CONSOLELOG_H

/// @file
/// Console logging utilities for MaterialX
/// Provides cross-platform logging that works in both native and WebAssembly environments

#include <MaterialXGenShader/Export.h>

#include <string>
#include <iostream>

MATERIALX_NAMESPACE_BEGIN

#ifdef EMSCRIPTEN
#include <emscripten/emscripten.h>
#endif

/// Console logging utilities for MaterialX
/// These functions provide cross-platform logging that works in both native and WebAssembly environments
namespace ConsoleLog
{
    /// Log a message to the console (info level)
    inline void logInfo(const std::string& msg)
    {
#ifdef EMSCRIPTEN
        EM_ASM({ console.log('MaterialX: ' + UTF8ToString($0)); }, msg.c_str());
#else
        std::cout << "MaterialX: " << msg << std::endl;
#endif
    }

    /// Log a message to the console (warning level)
    inline void logWarning(const std::string& msg)
    {
#ifdef EMSCRIPTEN
        EM_ASM({ console.warn('MaterialX Warning: ' + UTF8ToString($0)); }, msg.c_str());
#else
        std::cerr << "MaterialX Warning: " << msg << std::endl;
#endif
    }

    /// Log a message to the console (error level)
    inline void logError(const std::string& msg)
    {
#ifdef EMSCRIPTEN
        EM_ASM({ console.error('MaterialX Error: ' + UTF8ToString($0)); }, msg.c_str());
#else
        std::cerr << "MaterialX Error: " << msg << std::endl;
#endif
    }

    /// Log a message to the console (debug level)
    inline void logDebug(const std::string& msg)
    {
#ifdef EMSCRIPTEN
        EM_ASM({ console.debug('MaterialX Debug: ' + UTF8ToString($0)); }, msg.c_str());
#else
        std::cout << "MaterialX Debug: " << msg << std::endl;
#endif
    }

    /// Log a message with custom prefix to the console
    inline void logCustom(const std::string& prefix, const std::string& msg)
    {
#ifdef EMSCRIPTEN
        EM_ASM({ console.log(UTF8ToString($0) + ': ' + UTF8ToString($1)); }, prefix.c_str(), msg.c_str());
#else
        std::cout << prefix << ": " << msg << std::endl;
#endif
    }
}

/// Convenience macros for console logging
#define MX_LOG_INFO(msg) MaterialX::ConsoleLog::logInfo(msg)
#define MX_LOG_WARN(msg) MaterialX::ConsoleLog::logWarning(msg)
#define MX_LOG_ERROR(msg) MaterialX::ConsoleLog::logError(msg)
#define MX_LOG_DEBUG(msg) MaterialX::ConsoleLog::logDebug(msg)
#define MX_LOG_CUSTOM(prefix, msg) MaterialX::ConsoleLog::logCustom(prefix, msg)

MATERIALX_NAMESPACE_END

#endif // MATERIALX_CONSOLELOG_H
