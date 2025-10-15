//
// Debug ADSK opaque shader generation in Visual Studio
//

#include <MaterialXTest/External/Catch/catch.hpp>
#include <MaterialXTest/MaterialXGenShader/GenShaderUtil.h>

#include <MaterialXCore/Document.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXGenShader/GenContext.h>
#include <MaterialXGenShader/Shader.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>
#include <MaterialXGenGlsl/EsslShaderGenerator.h>
#include <MaterialXGenGlsl/WgslShaderGenerator.h>


#include <iostream>
#include <fstream>
#include <filesystem>
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

void loadAdskLib(mx::DocumentPtr doc)
{
    mx::FilePath adskLibPath = mx::FilePath("D:/Fluent/MaterialX/contrib/adsk/libraries/adsklib");
    mx::FilePath adskDefFile = adskLibPath / "adsklib_defs.mtlx";
    mx::FilePath adskNgFile = adskLibPath / "adsklib_ng.mtlx";

    mx::readFromXmlFile(doc, adskDefFile);
    mx::readFromXmlFile(doc, adskNgFile);
}

void writeShaderToFile(const std::string& shaderCode, const std::string& filePath)
{
    // Create directory if it doesn't exist
    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    // Write shader code to file
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << shaderCode;
        file.close();
        std::cout << "Shader written to: " << filePath << std::endl;
    } else {
        std::cout << "ERROR: Could not write shader to: " << filePath << std::endl;
    }
}

struct Config
{
    std::string materialFile;
    std::string materialName;
};

const std::map<std::string, Config> Configs = {
    { "metal", Config{"adsk_metal.mtlx", "Copper_Polished"} },
    { "opaque", Config{ "adsk_opaque.mtlx", "Walnut" } },
};

auto config = Configs.at("opaque");

TEST_CASE("Debug ADSK Opaque Shader Generation", "[genglsl]")
{


    // Create document and load libraries
    mx::FileSearchPath searchPath = mx::getDefaultDataSearchPath();

    // Add additional search paths for GLSL includes
    searchPath.append(mx::FilePath("D:/Fluent/MaterialX/libraries"));

    mx::DocumentPtr doc = mx::createDocument();
    mx::DocumentPtr stdlib = mx::createDocument();
    mx::loadLibraries({ "libraries" }, searchPath, stdlib);
    doc->setDataLibrary(stdlib);

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

    // Load adsk material file
    auto materialPath = mx::FilePath("D:/Fluent/MaterialX/contrib/adsk/resources/Materials/TestSuite/adsklib/archviz");
    //mx::FilePath materialFile = materialPath / "adsk_opaque.mtlx";
    mx::FilePath materialFile = materialPath / config.materialFile;
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
    std::vector<std::string> materialNames = { config.materialName };
    //{"Walnut", "Walnut_Semigloss", "Plastic_Glossy_White"};

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
        //mx::ShaderGeneratorPtr generator = mx::EsslShaderGenerator::create();
        mx::ShaderGeneratorPtr generator = mx::WgslShaderGenerator::create();
        mx::GenContext context(generator);

        // Set search path for GLSL includes
        context.registerSourceCodeSearchPath(searchPath);

        try {
            std::cout << "Attempting shader generation..." << std::endl;

            // This is where the exception occurs - set breakpoint here!
            mx::ShaderPtr shader = generator->generate(shaderNode->getNamePath(), shaderNode, context);

            std::cout << "SUCCESS: Shader generated for " << materialName << std::endl;

            // Optionally print shaders
            std::cout << "Vertex:\n" << shader->getSourceCode(mx::Stage::VERTEX) << std::endl;
            std::cout << "Fragment:\n" << shader->getSourceCode(mx::Stage::PIXEL) << std::endl;

            // Write shaders to files
            const std::string outputDir = "D:/Fluent/MaterialX/generated_shaders/";
            const std::string vertexFileName = outputDir + materialName + "_vertex.glsl";
            const std::string fragmentFileName = outputDir + materialName + "_fragment.glsl";

            writeShaderToFile(shader->getSourceCode(mx::Stage::VERTEX), vertexFileName);
            writeShaderToFile(shader->getSourceCode(mx::Stage::PIXEL), fragmentFileName);
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
}
