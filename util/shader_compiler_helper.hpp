#ifndef SHADER_COMPILER_HELPER_HPP
#define SHADER_COMPILER_HELPER_HPP

#include <iostream>
#include <string>
#include <cstring>

#include <shaderc/shaderc.hpp>

#include "log/log.hpp"

namespace LLShader{

    void CompilerShader(const std::string& sourceName,
                        const shaderc_shader_kind shaderKind, 
                        const std::string source);

    /// 编辑成 SPIR-V 中间代码
    /// [sourceName] 是 shader 名称
    /// [source] 是 shader 源代码
    std::string CompilerToAssembly(const std::string& sourceName, 
                                   const shaderc_shader_kind kind, 
                                   const std::string& source, 
                                   bool optimize = false);
    
    /// 编辑成 SPIR-V 二进制
    /// [sourceName] 是 shader 名称
    /// [source] 是 shader 源代码
    std::vector<uint32_t> CompilerToBinary(const std::string& sourceName, 
                                           const shaderc_shader_kind kind, 
                                           const std::string& source, 
                                           bool optimize = false);
    /// 编辑成 SPIR-V 二进制
    /// [sourceName] 是 shader 名称
    /// [source] 是 shader 源代码
    std::vector<uint32_t> AssembleShader(const std::string& sourceName, 
                                         shaderc_shader_kind kind, 
                                         const std::string& source,
                                         bool optimize = false);

    std::string preprocessGlslShader(   const std::string& source_name,
                                        shaderc_shader_kind kind,
                                        const std::vector<char>& source);
}

#endif