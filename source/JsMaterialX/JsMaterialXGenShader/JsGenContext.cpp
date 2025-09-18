//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include "../Helpers.h"

#include <MaterialXCore/Unit.h>
#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenShader/ShaderGenerator.h>
#include <MaterialXGenShader/HwShaderGenerator.h>
#include <MaterialXGenShader/DefaultColorManagementSystem.h>
#include <MaterialXFormat/Util.h>

#include <iostream>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

namespace ems = emscripten;
namespace mx = MaterialX;

// Debug logging functions for JavaScript
void logToConsole(const std::string& message)
{
    EM_ASM({
        console.log('MaterialX Debug: ' + UTF8ToString($0));
    }, message.c_str());
}

void logErrorToConsole(const std::string& message)
{
    EM_ASM({
        console.error('MaterialX Error: ' + UTF8ToString($0));
    }, message.c_str());
}

void logWarningToConsole(const std::string& message)
{
    EM_ASM({
        console.warn('MaterialX Warning: ' + UTF8ToString($0));
    }, message.c_str());
}

// Debug wrapper for shader generation
mx::ShaderPtr generateShaderWithDebug(mx::ShaderGenerator& generator, const std::string& name, mx::ElementPtr element, mx::GenContext& context)
{
    try
    {
        logToConsole("Starting shader generation for: " + name);
        logToConsole("Element category: " + element->getCategory());
        logToConsole("Generator target: " + generator.getTarget());

        mx::ShaderPtr shader = generator.generate(name, element, context);

        logToConsole("Shader generation successful for: " + name);
        logToConsole("Shader has " + std::to_string(shader->numStages()) + " stages");

        return shader;
    }
    catch (const std::exception& e)
    {
        logErrorToConsole("Shader generation failed for " + name + ": " + std::string(e.what()));
        throw;
    }
}

/// Initialize the given generation context
void initContext(mx::GenContext& context, mx::FileSearchPath searchPath, mx::DocumentPtr stdLib, mx::UnitConverterRegistryPtr unitRegistry)
{
    // Register the search path for shader source code.
    context.registerSourceCodeSearchPath(searchPath);

    // Set shader generation options.
    context.getOptions().targetColorSpaceOverride = "lin_rec709";
    context.getOptions().fileTextureVerticalFlip = false;
    context.getOptions().hwMaxActiveLightSources = 1;
    context.getOptions().hwSpecularEnvironmentMethod = mx::SPECULAR_ENVIRONMENT_FIS;
    context.getOptions().hwDirectionalAlbedoMethod = mx::DIRECTIONAL_ALBEDO_ANALYTIC;

    // Initialize color management.
    mx::DefaultColorManagementSystemPtr cms = mx::DefaultColorManagementSystem::create(context.getShaderGenerator().getTarget());
    cms->loadLibrary(stdLib);
    context.getShaderGenerator().setColorManagementSystem(cms);

    // Initialize unit management.
    mx::UnitSystemPtr unitSystem = mx::UnitSystem::create(context.getShaderGenerator().getTarget());
    unitSystem->loadLibrary(stdLib);
    unitSystem->setUnitConverterRegistry(unitRegistry);
    context.getShaderGenerator().setUnitSystem(unitSystem);
    context.getOptions().targetDistanceUnit = "meter";
}

/// Tries to load the standard libraries and initialize the given generation context. The loaded libraries are added to the returned document
mx::DocumentPtr loadStandardLibraries(mx::GenContext& context)
{
    logToConsole("Starting loadStandardLibraries");

    mx::DocumentPtr stdLib;
    mx::LinearUnitConverterPtr _distanceUnitConverter;
    mx::StringVec _distanceUnitOptions;
    mx::UnitConverterRegistryPtr unitRegistry(mx::UnitConverterRegistry::create());
    mx::FilePathVec libraryFolders = { "libraries" };
    mx::FileSearchPath searchPath;
    searchPath.append("/");

    logToConsole("Search path: " + searchPath.asString());

    // Initialize the standard library.
    try
    {
        stdLib = mx::createDocument();
        logToConsole("Loading libraries from folders");
        mx::StringSet _xincludeFiles = mx::loadLibraries(libraryFolders, searchPath, stdLib);
        logToConsole("Loaded " + std::to_string(_xincludeFiles.size()) + " library files");
        if (_xincludeFiles.empty())
        {
            logWarningToConsole("Could not find standard data libraries on the given search path: " + searchPath.asString());
        }
    }
    catch (std::exception& e)
    {
        logErrorToConsole("Failed to load standard data libraries: " + std::string(e.what()));
        return nullptr;
    }

    // Initialize unit management.
    mx::UnitTypeDefPtr distanceTypeDef = stdLib->getUnitTypeDef("distance");
    _distanceUnitConverter = mx::LinearUnitConverter::create(distanceTypeDef);
    unitRegistry->addUnitConverter(distanceTypeDef, _distanceUnitConverter);
    mx::UnitTypeDefPtr angleTypeDef = stdLib->getUnitTypeDef("angle");
    mx::LinearUnitConverterPtr angleConverter = mx::LinearUnitConverter::create(angleTypeDef);
    unitRegistry->addUnitConverter(angleTypeDef, angleConverter);

    // Create the list of supported distance units.
    auto unitScales = _distanceUnitConverter->getUnitScale();
    _distanceUnitOptions.resize(unitScales.size());
    for (auto unitScale : unitScales)
    {
        int location = _distanceUnitConverter->getUnitAsInteger(unitScale.first);
        _distanceUnitOptions[location] = unitScale.first;
    }

    logToConsole("Initializing context with generator target: " + context.getShaderGenerator().getTarget());
    initContext(context,searchPath, stdLib, unitRegistry);
    logToConsole("Standard libraries loaded successfully");

    return stdLib;
}

EMSCRIPTEN_BINDINGS(GenContext)
{
    ems::class_<mx::GenContext>("GenContext")
        .constructor<mx::ShaderGeneratorPtr>()
        .smart_ptr<std::shared_ptr<mx::GenContext>>("GenContextPtr")
        .function("getOptions", PTR_RETURN_OVERLOAD(mx::GenOptions& (mx::GenContext::*)(), &mx::GenContext::getOptions), ems::allow_raw_pointers())
        ;

    ems::function("loadStandardLibraries", &loadStandardLibraries);

    // Debug logging functions
    ems::function("logToConsole", &logToConsole);
    ems::function("logErrorToConsole", &logErrorToConsole);
    ems::function("logWarningToConsole", &logWarningToConsole);

    // Debug shader generation
    ems::function("generateShaderWithDebug", &generateShaderWithDebug, ems::allow_raw_pointers());
}
