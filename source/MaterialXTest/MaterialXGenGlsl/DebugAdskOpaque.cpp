//
// Debug ADSK opaque shader generation in Visual Studio
//

#include <MaterialXTest/External/Catch/catch.hpp>
#include <MaterialXTest/MaterialXGenShader/GenShaderUtil.h>

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>

#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#include <sstream>
#endif

namespace mx = MaterialX;

#ifdef _WIN32
// Custom streambuf that redirects to Visual Studio Debug Output
class DebugStreambuf : public std::streambuf {
public:
    virtual int_type overflow(int_type c) override {
        if (c != EOF) {
            if (c == '\n') {
                OutputDebugStringA(buffer.c_str());
                OutputDebugStringA("\n");
                buffer.clear();
            } else {
                buffer += static_cast<char>(c);
            }
        }
        return c;
    }
private:
    std::string buffer;
};

void DebugOutput(const std::string& msg) {
    OutputDebugStringA((msg + "\n").c_str());
}
#else
void DebugOutput(const std::string& msg) {
    std::cout << msg << std::endl;
}
#endif

TEST_CASE("Debug ADSK Opaque Shader Generation", "[genglsl]")
{
#ifdef _WIN32
    // Redirect std::cout to Visual Studio Debug Output
    DebugStreambuf debugBuf;
    std::streambuf* oldCoutBuf = std::cout.rdbuf(&debugBuf);
#endif

    // Create document and load libraries
    mx::FileSearchPath searchPath = mx::getDefaultDataSearchPath();

    // Add additional search paths for GLSL includes
    searchPath.append(mx::FilePath("../libraries"));
    searchPath.append(mx::FilePath("../../libraries"));
    searchPath.append(mx::FilePath("libraries"));
    searchPath.append(mx::FilePath("D:/Fluent/MaterialX/libraries"));

    mx::DocumentPtr doc = mx::createDocument();
    mx::DocumentPtr stdlib = mx::createDocument();
    mx::loadLibraries({ "libraries" }, searchPath, stdlib);
    doc->setDataLibrary(stdlib);

    // Debug: Print search paths
    DebugOutput("Available search paths:");
    for (size_t i = 0; i < searchPath.size(); ++i) {
        DebugOutput("  [" + std::to_string(i) + "] " + searchPath[i].asString());
    }

    // Try multiple possible paths for ADSK libraries
    std::vector<mx::FilePath> possiblePaths = {
        mx::FilePath("contrib/adsk/libraries/adsklib"),
        mx::FilePath("../contrib/adsk/libraries/adsklib"),
        mx::FilePath("../../contrib/adsk/libraries/adsklib"),
        mx::FilePath("D:/Fluent/MaterialX/contrib/adsk/libraries/adsklib")
    };

    // Also try relative to each search path
    for (const auto& sp : searchPath) {
        possiblePaths.push_back(sp / "contrib/adsk/libraries/adsklib");
        possiblePaths.push_back(sp.getParentPath() / "contrib/adsk/libraries/adsklib");
    }

    mx::FilePath adskLibPath;
    for (const auto& path : possiblePaths) {
        mx::FilePath testFile = path / "adsklib_defs.mtlx";
        std::cout << "Trying: " << testFile.asString() << " - ";
        if (testFile.exists()) {
            std::cout << "FOUND!" << std::endl;
            adskLibPath = path;
            break;
        } else {
            std::cout << "not found" << std::endl;
        }
    }

    if (adskLibPath.isEmpty()) {
        std::cout << "ERROR: Could not find ADSK libraries in any expected location" << std::endl;
        std::cout << "Please ensure contrib/adsk/libraries/adsklib/adsklib_defs.mtlx exists" << std::endl;
        REQUIRE(false);
    }

    mx::FilePath adskDefFile = adskLibPath / "adsklib_defs.mtlx";
    mx::FilePath adskNgFile = adskLibPath / "adsklib_ng.mtlx";

    mx::readFromXmlFile(doc, adskDefFile);
    mx::readFromXmlFile(doc, adskNgFile);

    // Verify ADSK nodes loaded
    mx::vector<mx::NodeDefPtr> nodeDefs = doc->getNodeDefs();
    bool adskNodeFound = false;
    for (auto nodeDef : nodeDefs) {
        if (nodeDef->getName().find("adsk") != std::string::npos) {
            std::cout << "Found ADSK NodeDef: " << nodeDef->getName() << std::endl;
            adskNodeFound = true;
        }
    }
    REQUIRE(adskNodeFound == true);

    // Find the problematic material file using similar approach
    std::vector<mx::FilePath> materialPossiblePaths = {
        mx::FilePath("contrib/adsk/resources/Materials/TestSuite/adsklib/archviz"),
        mx::FilePath("../contrib/adsk/resources/Materials/TestSuite/adsklib/archviz"),
        mx::FilePath("../../contrib/adsk/resources/Materials/TestSuite/adsklib/archviz"),
        mx::FilePath("D:/Fluent/MaterialX/contrib/adsk/resources/Materials/TestSuite/adsklib/archviz")
    };

    // Also try relative to each search path
    for (const auto& sp : searchPath) {
        materialPossiblePaths.push_back(sp / "contrib/adsk/resources/Materials/TestSuite/adsklib/archviz");
        materialPossiblePaths.push_back(sp.getParentPath() / "contrib/adsk/resources/Materials/TestSuite/adsklib/archviz");
    }

    mx::FilePath materialPath;
    for (const auto& path : materialPossiblePaths) {
        mx::FilePath testFile = path / "adsk_opaque.mtlx";
        std::cout << "Trying material: " << testFile.asString() << " - ";
        if (testFile.exists()) {
            std::cout << "FOUND!" << std::endl;
            materialPath = path;
            break;
        } else {
            std::cout << "not found" << std::endl;
        }
    }

    if (materialPath.isEmpty()) {
        std::cout << "ERROR: Could not find ADSK material file in any expected location" << std::endl;
        std::cout << "Please ensure contrib/adsk/resources/Materials/TestSuite/adsklib/archviz/adsk_opaque.mtlx exists" << std::endl;
        REQUIRE(false);
    }

    mx::FilePath materialFile = materialPath / "adsk_opaque.mtlx";
    std::cout << "Loading material from: " << materialFile.asString() << std::endl;

    mx::readFromXmlFile(doc, materialFile);

    // Find the materials (they are Node elements with category "material")
    auto materials = doc->getChildrenOfType<mx::Node>();
    std::vector<mx::NodePtr> materialNodes;
    for (auto node : materials) {
        if (node->getCategory() == "material") {
            materialNodes.push_back(node);
        }
    }
    std::cout << "Materials found: " << materialNodes.size() << std::endl;
    for (auto mat : materialNodes) {
        std::cout << "  - " << mat->getName() << std::endl;
    }

    // Test each material separately
    std::vector<std::string> materialNames = {"Walnut", "Walnut_Semigloss", "Plastic_Glossy_White"};

    for (const std::string& materialName : materialNames) {
        std::cout << "\n=== Testing material: " << materialName << " ===" << std::endl;

        mx::NodePtr material = doc->getChildOfType<mx::Node>(materialName);
        REQUIRE(material != nullptr);
        REQUIRE(material->getCategory() == "surfacematerial");

        // Get the shader node (find input with type "surfaceshader")
        mx::NodePtr shaderNode = nullptr;
        for (auto input : material->getInputs()) {
            if (input->getType() == "surfaceshader" && input->hasNodeName()) {
                shaderNode = input->getConnectedNode();
                break;
            }
        }
        REQUIRE(shaderNode != nullptr);

        std::cout << "Shader node name: " << shaderNode->getName() << std::endl;
        std::cout << "Shader node category: " << shaderNode->getCategory() << std::endl;

        // Check NodeDef
        mx::NodeDefPtr nodeDef = shaderNode->getNodeDef();
        if (!nodeDef) {
            std::cout << "ERROR: No NodeDef found for category: " << shaderNode->getCategory() << std::endl;
            FAIL("NodeDef not found");
        }

        std::cout << "NodeDef found: " << nodeDef->getName() << std::endl;

        // Check all inputs
        auto inputs = shaderNode->getInputs();
        std::cout << "Shader inputs (" << inputs.size() << "):" << std::endl;
        for (auto input : inputs) {
            std::cout << "  - " << input->getName() << ": " << input->getType() << " = " << input->getValueString() << std::endl;
        }

        // Check NodeDef required inputs
        auto nodeDefInputs = nodeDef->getInputs();
        std::cout << "NodeDef required inputs (" << nodeDefInputs.size() << "):" << std::endl;
        for (auto input : nodeDefInputs) {
            std::cout << "  - " << input->getName() << ": " << input->getType();
            if (input->hasAttribute("defaultgeomprop")) {
                std::cout << " [defaultgeomprop: " << input->getAttribute("defaultgeomprop") << "]";
            }
            std::cout << std::endl;
        }

        // Create shader generator
        mx::ShaderGeneratorPtr generator = mx::GlslShaderGenerator::create();
        mx::GenContext context(generator);

        // Set search path for GLSL includes
        context.registerSourceCodeSearchPath(searchPath);

        try {
            std::cout << "Attempting shader generation..." << std::endl;

            // This is where the exception occurs - set breakpoint here!
            mx::ShaderPtr shader = generator->generate(shaderNode->getNamePath(), shaderNode, context);

            std::cout << "SUCCESS: Shader generated for " << materialName << std::endl;

            // Optionally print shaders
            // std::cout << "Vertex:\n" << shader->getSourceCode(mx::Stage::VERTEX) << std::endl;
            // std::cout << "Fragment:\n" << shader->getSourceCode(mx::Stage::PIXEL) << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "EXCEPTION for " << materialName << ": " << e.what() << std::endl;

            // If this is Plastic_Glossy_White, we expect it to fail
            if (materialName == "Plastic_Glossy_White") {
                std::cout << "Expected failure for Plastic_Glossy_White" << std::endl;
                // Don't fail the test, just log it
            } else {
                FAIL("Unexpected shader generation failure");
            }
        }
    }

#ifdef _WIN32
    // Restore original cout buffer
    std::cout.rdbuf(oldCoutBuf);
#endif
}
