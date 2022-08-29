import os
import platform
import shutil
import tempfile
from gen_astc_unquant_map import genAllTables

script_path = os.path.dirname(os.path.abspath(__file__))
qemu_path = os.path.join(script_path, "..", "..")
prebuilts_path = os.path.join(qemu_path, "..", "..", "prebuilts")

if platform.system() == "Linux":
    platform_name = "linux-x86_64"
elif platform.system() == "Windows":
    platform_name = "windows-x86_64"
elif platform.system() == "Darwin":
    platform_name = "darwin-x86_64"

glslangValidator_path = os.path.join(prebuilts_path, "android-emulator-build", "common", "vulkan", platform_name, "glslangValidator")

shader_source_path = os.path.join(qemu_path, "android", "android-emugl", "host", "libs", "libOpenglRender", "vulkan")
shader_lib_name = "Etc2ShaderLib"
shader_source_names = {"Etc2RGB8", "Etc2RGBA8", "EacR11Unorm", "EacR11Snorm", "EacRG11Unorm", "EacRG11Snorm", "Astc"}
image_types = {"1DArray", "2DArray", "3D"}

unquant_table_string = genAllTables()

shader_sources = ["", ""]
file_name = os.path.join(shader_source_path, shader_lib_name + ".comp")
with open(file_name, 'r') as file:
    shader_sources[0] = file.read()

for shader_source_name in shader_source_names:
    file_name = os.path.join(shader_source_path, shader_source_name + ".comp")
    for image_type in image_types:
        with open(file_name, 'r') as file:
            shader_sources[1] = file.read()
        tmp_file, tmp_file_name = tempfile.mkstemp(suffix=".comp")
        if shader_source_name != "Astc":
            combined_shader_source = "\n".join(shader_sources)
        else:
            combined_shader_source = shader_sources[1]
        os.write(tmp_file, combined_shader_source.replace("${type}", image_type).replace("${UnquantTables}", unquant_table_string).encode())
        os.close(tmp_file)
        out_spv_path = os.path.join(shader_source_path, shader_source_name + "_" + image_type + ".spv")
        if os.path.exists(out_spv_path):
            os.remove(out_spv_path)
        ret = os.system(" ".join([glslangValidator_path, "--spirv-val", "--target-env", "vulkan1.1", "-V", "-o", out_spv_path, tmp_file_name]))
        if ret != 0:
            print("Compiling %s got return code %d" % (out_spv_path, ret))
            print("Please look at intermediate source file for debug: %s" % tmp_file_name)
        else:
            print("Compiled %s successfully" % out_spv_path)
            os.remove(tmp_file_name)
