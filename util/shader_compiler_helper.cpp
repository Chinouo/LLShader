#include "shader_compiler_helper.hpp"

namespace LLShader{

    std::string preprocessGlslShader(   const std::string& source_name,
                                        shaderc_shader_kind kind,
                                        const std::vector<char>& source){
    int a;
        shaderc::Compiler compiler;
        shaderc::CompileOptions opt;
        shaderc::PreprocessedSourceCompilationResult result = 
        compiler.PreprocessGlsl(source_name, kind, source.data(), opt);

        if(result.GetCompilationStatus() != shaderc_compilation_status_success){
            throw std::runtime_error("preprocess " + source_name + " failed. message:" + result.GetErrorMessage());
        }

        return {result.cbegin(), result.cend()};
    }
    

    void CompilerShader(const std::string& sourceName, const shaderc_shader_kind shaderKind, const std::string source){
        assert(false);
    };

    std::string CompilerToAssembly(const std::string& sourceName, const shaderc_shader_kind kind, const std::string& source, bool optimize){

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
            source, kind, sourceName.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            LogUtil::LogE(result.GetErrorMessage());
            return "";
        }

        return {result.cbegin(), result.cend()};
    }

    std::vector<uint32_t> CompilerToBinary(const std::string& sourceName, 
                                           shaderc_shader_kind kind,
                                           const std::string& source,
                                           bool optimize){
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(source, kind, sourceName.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << module.GetErrorMessage();
            return std::vector<uint32_t>();
        }

        return {module.cbegin(), module.cend()};
    }


    std::vector<uint32_t> AssembleShader(const std::string& sourceName, shaderc_shader_kind kind, const std::string& source, bool optimize){
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        //options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(source, kind, sourceName.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << module.GetErrorMessage();
            return std::vector<uint32_t>();
        }

        return {module.cbegin(), module.cend()};
    }

}