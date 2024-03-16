/*
    Generate sokol-nim module.
*/
#include "sokolnim.h"
#include "util.h"
#include "fmt/format.h"
#include "pystring.h"
#include <stdio.h>

namespace shdc::formats::sokolnim {

using namespace util;

static std::string file_content;

#if defined(_MSC_VER)
#define L(str, ...) file_content.append(fmt::format(str, __VA_ARGS__))
#else
#define L(str, ...) file_content.append(fmt::format(str, ##__VA_ARGS__))
#endif

static const char* uniform_type_to_sokol_type_str(Uniform::Type type) {
    switch (type) {
        case Uniform::FLOAT:  return "uniformTypeFloat";
        case Uniform::FLOAT2: return "uniformTypeFloat2";
        case Uniform::FLOAT3: return "uniformTypeFloat3";
        case Uniform::FLOAT4: return "uniformTypeFloat4";
        case Uniform::INT:    return "uniformTypeInt";
        case Uniform::INT2:   return "uniformTypeInt2";
        case Uniform::INT3:   return "uniformTypeInt3";
        case Uniform::INT4:   return "uniformTypeInt4";
        case Uniform::MAT4:   return "uniformTypeMat4";
        default: return "FIXME";
    }
}

static const char* uniform_type_to_flattened_sokol_type_str(Uniform::Type type) {
    switch (type) {
        case Uniform::FLOAT:
        case Uniform::FLOAT2:
        case Uniform::FLOAT3:
        case Uniform::FLOAT4:
        case Uniform::MAT4:
             return "uniformTypeFloat4";
        case Uniform::INT:
        case Uniform::INT2:
        case Uniform::INT3:
        case Uniform::INT4:
            return "uniformTypeInt4";
        default: return "FIXME";
    }
}

static const char* img_type_to_sokol_type_str(ImageType::Enum type) {
    switch (type) {
        case ImageType::_2D:     return "imageType2d";
        case ImageType::CUBE:    return "imageTypeCube";
        case ImageType::_3D:     return "imageType3d";
        case ImageType::ARRAY:   return "imageTypeArray";
        default: return "INVALID";
    }
}

static const char* img_basetype_to_sokol_sampletype_str(ImageSampleType::Enum type) {
    switch (type) {
        case ImageSampleType::FLOAT:               return "imageSampleTypeFloat";
        case ImageSampleType::DEPTH:               return "imageSampleTypeDepth";
        case ImageSampleType::SINT:                return "imageSampleTypeSint";
        case ImageSampleType::UINT:                return "imageSamplerTypeUint";
        case ImageSampleType::UNFILTERABLE_FLOAT:  return "imageSamplerTypeUnfilterableFloat";
        default: return "INVALID";
    }
}

static const char* smp_type_to_sokol_type_str(SamplerType::Enum type) {
    switch (type) {
        case SamplerType::FILTERING:     return "samplerTypeFiltering";
        case SamplerType::COMPARISON:    return "samplerTypeComparison";
        case SamplerType::NONFILTERING:  return "samplerTypeNonfiltering";
        default: return "INVALID";
    }
}

static const char* sokol_backend(Slang::Enum slang) {
    switch (slang) {
        case Slang::GLSL410:      return "backendGlcore";
        case Slang::GLSL430:      return "backendGlcore";
        case Slang::GLSL300ES:    return "backendGles3";
        case Slang::HLSL4:        return "backendD3d11";
        case Slang::HLSL5:        return "backendD3d11";
        case Slang::METAL_MACOS:  return "backendMetalMacos";
        case Slang::METAL_IOS:    return "backendMetalIos";
        case Slang::METAL_SIM:    return "backendMetalSimulator";
        case Slang::WGSL:         return "backendWgsl";
        default: return "<INVALID>";
    }
}

static std::string to_nim_struct_name(const std::string& prefix, const std::string& struct_name) {
    return to_pascal_case(fmt::format("{}_{}", prefix, struct_name));
}

static void write_header(const Args& args, const Input& inp, const Spirvcross& spirvcross) {
    L("#\n");
    L("#   #version:{}# (machine generated, don't edit!)\n", args.gen_version);
    L("#\n");
    L("#   Generated by sokol-shdc (https://github.com/floooh/sokol-tools)\n");
    L("#\n");
    L("#   Cmdline: {}\n", args.cmdline);
    L("#\n");
    L("#   Overview:\n");
    L("#\n");
    for (const auto& item: inp.programs) {
        const Program& prog = item.second;

        const SpirvcrossSource* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
        const SpirvcrossSource* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
        assert(vs_src && fs_src);
        L("#       Shader program '{}':\n", prog.name);
        L("#           Get shader desc: shd.{}ShaderDesc(sg.queryBackend())\n", to_camel_case(fmt::format("{}_{}", mod_prefix(inp), prog.name)));
        L("#           Vertex shader: {}\n", prog.vs_name);
        L("#               Attribute slots:\n");
        const Snippet& vs_snippet = inp.snippets[vs_src->snippet_index];
        for (const VertexAttr& attr: vs_src->refl.inputs) {
            if (attr.slot >= 0) {
                L("#                   ATTR_{}{}_{} = {}\n", mod_prefix(inp), vs_snippet.name, attr.name, attr.slot);
            }
        }
        for (const UniformBlock& ub: vs_src->refl.uniform_blocks) {
            L("#               Uniform block '{}':\n", ub.struct_name);
            L("#                   Nim struct: {}\n", to_nim_struct_name(mod_prefix(inp), ub.struct_name));
            L("#                   Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.struct_name, ub.slot);
        }
        for (const Image& img: vs_src->refl.images) {
            L("#               Image '{}':\n", img.name);
            L("#                   Image Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("#                   Sample Type: {}\n", img_basetype_to_sokol_sampletype_str(img.sample_type));
            L("#                   Multisampled: {}\n", img.multisampled);
            L("#                   Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        for (const Sampler& smp: vs_src->refl.samplers) {
            L("#               Sampler '{}':\n", smp.name);
            L("#                   Type: {}\n", smp_type_to_sokol_type_str(smp.type));
            L("#                   Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), smp.name, smp.slot);
        }
        for (const ImageSampler& img_smp: vs_src->refl.image_samplers) {
            L("#               Image Sampler Pair '{}':\n", img_smp.name);
            L("#                   Image: {}\n", img_smp.image_name);
            L("#                   Sampler: {}\n", img_smp.sampler_name);
        }
        L("#           Fragment shader: {}\n", prog.fs_name);
        for (const UniformBlock& ub: fs_src->refl.uniform_blocks) {
            L("#               Uniform block '{}':\n", ub.struct_name);
            L("#                   Nim struct: {}\n", to_nim_struct_name(mod_prefix(inp), ub.struct_name));
            L("#                   Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), ub.struct_name, ub.slot);
        }
        for (const Image& img: fs_src->refl.images) {
            L("#               Image '{}':\n", img.name);
            L("#                   Image Type: {}\n", img_type_to_sokol_type_str(img.type));
            L("#                   Sample Type: {}\n", img_basetype_to_sokol_sampletype_str(img.sample_type));
            L("#                   Multisampled: {}\n", img.multisampled);
            L("#                   Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), img.name, img.slot);
        }
        for (const Sampler& smp: fs_src->refl.samplers) {
            L("#               Sampler '{}':\n", smp.name);
            L("#                   Type: {}\n", smp_type_to_sokol_type_str(smp.type));
            L("#                   Bind slot: SLOT_{}{} = {}\n", mod_prefix(inp), smp.name, smp.slot);
        }
        for (const ImageSampler& img_smp: fs_src->refl.image_samplers) {
            L("#               Image Sampler Pair '{}':\n", img_smp.name);
            L("#                   Image: {}\n", img_smp.image_name);
            L("#                   Sampler: {}\n", img_smp.sampler_name);
        }
        L("#\n");
    }
    L("#\n");
    L("import sokol/gfx as sg\n");
    for (const auto& header: inp.headers) {
        L("{}\n", header);
    }
    L("\n");
}

static void write_vertex_attrs(const Input& inp, const Spirvcross& spirvcross) {
    for (const SpirvcrossSource& src: spirvcross.sources) {
        if (src.refl.stage == ShaderStage::VS) {
            const Snippet& vs_snippet = inp.snippets[src.snippet_index];
            for (const VertexAttr& attr: src.refl.inputs) {
                if (attr.slot >= 0) {
                    const auto attrName = to_camel_case(fmt::format("ATTR_{}_{}_{}", mod_prefix(inp), vs_snippet.name, attr.name));
                    L("const {}* = {}\n", attrName, attr.slot);
                }
            }
        }
    }
    L("\n");
}

static void write_image_bind_slots(const Input& inp, const Spirvcross& spirvcross) {
    for (const Image& img: spirvcross.unique_images) {
        const auto slotName = to_camel_case(fmt::format("SLOT_{}_{}", mod_prefix(inp), img.name));
        L("const {}* = {}\n", slotName, img.slot);
    }
    L("\n");
}

static void write_sampler_bind_slots(const Input& inp, const Spirvcross& spirvcross) {
    for (const Sampler& smp: spirvcross.unique_samplers) {
        const auto slotName = to_camel_case(fmt::format("SLOT_{}_{}", mod_prefix(inp), smp.name));
        L("const {}* = {}\n", slotName, smp.slot);
    }
    L("\n");
}

static void write_uniform_blocks(const Input& inp, const Spirvcross& spirvcross, Slang::Enum slang) {
    for (const UniformBlock& ub: spirvcross.unique_uniform_blocks) {
        const auto slotName = to_camel_case(fmt::format("SLOT_{}_{}", mod_prefix(inp), ub.struct_name));
        L("const {}* = {}\n", slotName, ub.slot);
        L("type {}* {{.packed.}} = object\n", to_nim_struct_name(mod_prefix(inp), ub.struct_name));
        int cur_offset = 0;
        for (const Uniform& uniform: ub.uniforms) {
            // align the first item
            int next_offset = uniform.offset;
            if (next_offset > cur_offset) {
                L("    pad_{}: array[{}, uint8]\n", cur_offset, next_offset - cur_offset);
                cur_offset = next_offset;
            }
            const char* align = (0 == cur_offset) ? " {.align(16).}" : "";
            if (inp.ctype_map.count(uniform_type_str(uniform.type)) > 0) {
                // user-provided type names
                if (uniform.array_count == 1) {
                    L("    {}*{}: {}\n", uniform.name, align, inp.ctype_map.at(uniform_type_str(uniform.type)));
                }
                else {
                    L("    {}*{}: [{}]{}\n", uniform.name, align, uniform.array_count, inp.ctype_map.at(uniform_type_str(uniform.type)));
                }
            }
            else {
                // default type names (float)
                if (uniform.array_count == 1) {
                    switch (uniform.type) {
                        case Uniform::FLOAT:   L("    {}*{}: float32\n", uniform.name, align); break;
                        case Uniform::FLOAT2:  L("    {}*{}: array[2, float32]\n", uniform.name, align); break;
                        case Uniform::FLOAT3:  L("    {}*{}: array[3, float32]\n", uniform.name, align); break;
                        case Uniform::FLOAT4:  L("    {}*{}: array[4, float32]\n", uniform.name, align); break;
                        case Uniform::INT:     L("    {}*{}: int32\n", uniform.name, align); break;
                        case Uniform::INT2:    L("    {}*{}: array[2, int32]\n", uniform.name, align); break;
                        case Uniform::INT3:    L("    {}*{}: array[3, int32]\n", uniform.name, align); break;
                        case Uniform::INT4:    L("    {}*{}: array[4, int32]\n", uniform.name, align); break;
                        case Uniform::MAT4:    L("    {}*{}: array[16, float32]\n", uniform.name, align); break;
                        default:                 L("    INVALID_UNIFORM_TYPE\n"); break;
                    }
                }
                else {
                    switch (uniform.type) {
                        case Uniform::FLOAT4:  L("    {}*{}: array[{}, array[4, float32]]\n", uniform.name, align, uniform.array_count); break;
                        case Uniform::INT4:    L("    {}*{}: array[{}, array[4, int32]]\n", uniform.name, align, uniform.array_count); break;
                        case Uniform::MAT4:    L("    {}*{}: array[{}, array[16, float32]]\n", uniform.name, align, uniform.array_count); break;
                        default:                 L("    INVALID_UNIFORM_TYPE\n"); break;
                    }
                }
            }
            cur_offset += uniform_size(uniform.type, uniform.array_count);
        }
        /* pad to multiple of 16-bytes struct size */
        const int round16 = roundup(cur_offset, 16);
        if (cur_offset != round16) {
            L("    pad_{}: array[{}, uint8]\n", cur_offset, round16-cur_offset);
        }
        L("\n");
    }
}

static void write_shader_sources_and_blobs(const Input& inp,
                                           const Spirvcross& spirvcross,
                                           const Bytecode& bytecode,
                                           Slang::Enum slang)
{
    for (int snippet_index = 0; snippet_index < (int)inp.snippets.size(); snippet_index++) {
        const Snippet& snippet = inp.snippets[snippet_index];
        if ((snippet.type != Snippet::VS) && (snippet.type != Snippet::FS)) {
            continue;
        }
        int src_index = spirvcross.find_source_by_snippet_index(snippet_index);
        assert(src_index >= 0);
        const SpirvcrossSource& src = spirvcross.sources[src_index];
        int blob_index = bytecode.find_blob_by_snippet_index(snippet_index);
        const BytecodeBlob* blob = 0;
        if (blob_index != -1) {
            blob = &bytecode.blobs[blob_index];
        }
        std::vector<std::string> lines;
        pystring::splitlines(src.source_code, lines);
        // first write the source code in a comment block
        L("#\n");
        for (const std::string& line: lines) {
            L("#   {}\n", util::replace_C_comment_tokens(line));
        }
        L("#\n");
        if (blob) {
            std::string nim_name = to_camel_case(fmt::format("{}_{}_bytecode_{}", mod_prefix(inp), snippet.name, Slang::to_str(slang)));
            L("const {}: array[{}, uint8] = [\n", nim_name.c_str(), blob->data.size());
            const size_t len = blob->data.size();
            for (size_t i = 0; i < len; i++) {
                if ((i & 15) == 0) {
                    L("    ");
                }
                if (0 == i) {
                    L("{:#04x}'u8,", blob->data[i]);
                }
                else {
                    L("{:#04x},", blob->data[i]);
                }
                if ((i & 15) == 15) {
                    L("\n");
                }
            }
            L("\n]\n");
        }
        else {
            // if no bytecode exists, write the source code, but also a byte array with a trailing 0
            std::string nim_name = to_camel_case(fmt::format("{}_{}_source_{}", mod_prefix(inp), snippet.name, Slang::to_str(slang)));
            const size_t len = src.source_code.length() + 1;
            L("const {}: array[{}, uint8] = [\n", nim_name.c_str(), len);
            for (size_t i = 0; i < len; i++) {
                if ((i & 15) == 0) {
                    L("    ");
                }
                if (0 == i) {
                    L("{:#04x}'u8,", src.source_code[i]);
                }
                else {
                    L("{:#04x},", src.source_code[i]);
                }
                if ((i & 15) == 15) {
                    L("\n");
                }
            }
            L("\n]\n");
        }
    }
}

static void write_stage(const char* indent,
                        const char* stage_name,
                        const SpirvcrossSource* src,
                        const std::string& src_name,
                        const BytecodeBlob* blob,
                        const std::string& blob_name,
                        Slang::Enum slang)
{
    if (blob) {
        L("{}result.{}.bytecode = {}\n", indent, stage_name, blob_name);
    }
    else {
        L("{}result.{}.source = cast[cstring](addr({}))\n", indent, stage_name, src_name);
        const char* d3d11_tgt = nullptr;
        if (slang == Slang::HLSL4) {
            d3d11_tgt = (0 == strcmp("vs", stage_name)) ? "vs_4_0" : "ps_4_0";
        }
        else if (slang == Slang::HLSL5) {
            d3d11_tgt = (0 == strcmp("vs", stage_name)) ? "vs_5_0" : "ps_5_0";
        }
        if (d3d11_tgt) {
            L("{}result.{}.d3d11Target = \"{}\"\n", indent, stage_name, d3d11_tgt);
        }
    }
    assert(src);
    L("{}result.{}.entry = \"{}\"\n", indent, stage_name, src->refl.entry_point);
    for (int ub_index = 0; ub_index < UniformBlock::NUM; ub_index++) {
        const UniformBlock* ub = src->refl.find_uniform_block_by_slot(ub_index);
        if (ub) {
            L("{}result.{}.uniformBlocks[{}].size = {}\n", indent, stage_name, ub_index, roundup(ub->size, 16));
            L("{}result.{}.uniformBlocks[{}].layout = uniformLayoutStd140\n", indent, stage_name, ub_index);
            if (Slang::is_glsl(slang) && (ub->uniforms.size() > 0)) {
                if (ub->flattened) {
                    L("{}result.{}.uniformBlocks[{}].uniforms[0].name = \"{}\"\n", indent, stage_name, ub_index, ub->struct_name);
                    L("{}result.{}.uniformBlocks[{}].uniforms[0].type = {}\n", indent, stage_name, ub_index, uniform_type_to_flattened_sokol_type_str(ub->uniforms[0].type));
                    L("{}result.{}.uniformBlocks[{}].uniforms[0].arrayCount = {}\n", indent, stage_name, ub_index, roundup(ub->size, 16) / 16);
                }
                else {
                    for (int u_index = 0; u_index < (int)ub->uniforms.size(); u_index++) {
                        const Uniform& u = ub->uniforms[u_index];
                        L("{}result.{}.uniformBlocks[{}].uniforms[{}].name = \"{}.{}\"\n", indent, stage_name, ub_index, u_index, ub->inst_name, u.name);
                        L("{}result.{}.uniformBlocks[{}].uniforms[{}].type = {}\n", indent, stage_name, ub_index, u_index, uniform_type_to_sokol_type_str(u.type));
                        L("{}result.{}.uniformBlocks[{}].uniforms[{}].arrayCount = {}\n", indent, stage_name, ub_index, u_index, u.array_count);
                    }
                }
            }
        }
    }
    for (int img_index = 0; img_index < Image::NUM; img_index++) {
        const Image* img = src->refl.find_image_by_slot(img_index);
        if (img) {
            L("{}result.{}.images[{}].used = true\n", indent, stage_name, img_index);
            L("{}result.{}.images[{}].multisampled = {}\n", indent, stage_name, img_index, img->multisampled ? "true" : "false");
            L("{}result.{}.images[{}].imageType = {}\n", indent, stage_name, img_index, img_type_to_sokol_type_str(img->type));
            L("{}result.{}.images[{}].sampleType = {}\n", indent, stage_name, img_index, img_basetype_to_sokol_sampletype_str(img->sample_type));
        }
    }
    for (int smp_index = 0; smp_index < Sampler::NUM; smp_index++) {
        const Sampler* smp = src->refl.find_sampler_by_slot(smp_index);
        if (smp) {
            L("{}result.{}.samplers[{}].used = true\n", indent, stage_name, smp_index);
            L("{}result.{}.samplers[{}].samplerType = {}\n", indent, stage_name, smp_index, smp_type_to_sokol_type_str(smp->type));
        }
    }
    for (int img_smp_index = 0; img_smp_index < ImageSampler::NUM; img_smp_index++) {
        const ImageSampler* img_smp = src->refl.find_image_sampler_by_slot(img_smp_index);
        if (img_smp) {
            L("{}result.{}.imageSamplerPairs[{}].used = true\n", indent, stage_name, img_smp_index);
            L("{}result.{}.imageSamplerPairs[{}].imageSlot = {}\n", indent, stage_name, img_smp_index, src->refl.find_image_by_name(img_smp->image_name)->slot);
            L("{}result.{}.imageSamplerPairs[{}].samplerSlot = {}\n", indent, stage_name, img_smp_index, src->refl.find_sampler_by_name(img_smp->sampler_name)->slot);
            if (Slang::is_glsl(slang)) {
                L("{}result.{}.imageSamplerPairs[{}].glslName = \"{}\"\n", indent, stage_name, img_smp_index, img_smp->name);
            }
        }
    }
}

static void write_shader_desc_init(const char* indent, const Program& prog, const Input& inp, const Spirvcross& spirvcross, const Bytecode& bytecode, Slang::Enum slang) {
    const SpirvcrossSource* vs_src = find_spirvcross_source_by_shader_name(prog.vs_name, inp, spirvcross);
    const SpirvcrossSource* fs_src = find_spirvcross_source_by_shader_name(prog.fs_name, inp, spirvcross);
    assert(vs_src && fs_src);
    const BytecodeBlob* vs_blob = find_bytecode_blob_by_shader_name(prog.vs_name, inp, bytecode);
    const BytecodeBlob* fs_blob = find_bytecode_blob_by_shader_name(prog.fs_name, inp, bytecode);
    std::string vs_src_name, fs_src_name;
    std::string vs_blob_name, fs_blob_name;
    if (vs_blob) {
        vs_blob_name = to_camel_case(fmt::format("{}_{}_bytecode_{}", mod_prefix(inp), prog.vs_name, Slang::to_str(slang)));
    }
    else {
        vs_src_name = to_camel_case(fmt::format("{}{}_source_{}", mod_prefix(inp), prog.vs_name, Slang::to_str(slang)));
    }
    if (fs_blob) {
        fs_blob_name = to_camel_case(fmt::format("{}_{}_bytecode_{}", mod_prefix(inp), prog.fs_name, Slang::to_str(slang)));
    }
    else {
        fs_src_name = to_camel_case(fmt::format("{}_{}_source_{}", mod_prefix(inp), prog.fs_name, Slang::to_str(slang)));
    }

    // write shader desc
    for (int attr_index = 0; attr_index < VertexAttr::NUM; attr_index++) {
        const VertexAttr& attr = vs_src->refl.inputs[attr_index];
        if (attr.slot >= 0) {
            if (Slang::is_glsl(slang)) {
                L("{}result.attrs[{}].name = \"{}\"\n", indent, attr_index, attr.name);
            }
            else if (Slang::is_hlsl(slang)) {
                L("{}result.attrs[{}].semName = \"{}\"\n", indent, attr_index, attr.sem_name);
                L("{}result.attrs[{}].semIndex = {}\n", indent, attr_index, attr.sem_index);
            }
        }
    }
    write_stage(indent, "vs", vs_src, vs_src_name, vs_blob, vs_blob_name, slang);
    write_stage(indent, "fs", fs_src, fs_src_name, fs_blob, fs_blob_name, slang);
    const std::string shader_name = to_camel_case(fmt::format("{}_{}_shader", mod_prefix(inp), prog.name));
    L("{}result.label = \"{}\"\n", indent, shader_name);
}

ErrMsg gen(const Args& args, const Input& inp, const std::array<Spirvcross,Slang::NUM>& spirvcross, const std::array<Bytecode,Slang::NUM>& bytecode) {
    // first write everything into a string, and only when no errors occur,
    // dump this into a file (so we don't have half-written files lying around)
    file_content.clear();

    bool comment_header_written = false;
    bool common_decls_written = false;
    for (int i = 0; i < Slang::NUM; i++) {
        Slang::Enum slang = (Slang::Enum) i;
        if (args.slang & Slang::bit(slang)) {
            ErrMsg err = check_errors(inp, spirvcross[i], slang);
            if (err.has_error) {
                return err;
            }
            if (!comment_header_written) {
                write_header(args, inp, spirvcross[i]);
                comment_header_written = true;
            }
            if (!common_decls_written) {
                common_decls_written = true;
                write_vertex_attrs(inp, spirvcross[i]);
                write_image_bind_slots(inp, spirvcross[i]);
                write_sampler_bind_slots(inp, spirvcross[i]);
                write_uniform_blocks(inp, spirvcross[i], slang);
            }
            write_shader_sources_and_blobs(inp, spirvcross[i], bytecode[i], slang);
        }
    }

    // write access functions which return sg.ShaderDesc structs
    for (const auto& item: inp.programs) {
        const Program& prog = item.second;
        L("proc {}*(backend: sg.Backend): sg.ShaderDesc =\n", to_camel_case(fmt::format("{}_{}_shader_desc", mod_prefix(inp), prog.name)));
        L("  case backend:\n");
        for (int i = 0; i < Slang::NUM; i++) {
            Slang::Enum slang = (Slang::Enum) i;
            if (args.slang & Slang::bit(slang)) {
                L("    of {}:\n", sokol_backend(slang));
                write_shader_desc_init("      ", prog, inp, spirvcross[i], bytecode[i], slang);
            }
        }
        L("    else: discard\n");
        L("\n");
    }

    // write result into output file
    FILE* f = fopen(args.output.c_str(), "w");
    if (!f) {
        return ErrMsg::error(inp.base_path, 0, fmt::format("failed to open output file '{}'", args.output));
    }
    fwrite(file_content.c_str(), file_content.length(), 1, f);
    fclose(f);
    return ErrMsg();
}

} // namespace
