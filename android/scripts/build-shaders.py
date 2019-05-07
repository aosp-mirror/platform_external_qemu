import os
import platform
import shutil
import tempfile

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
shader_source_names = {"Etc2RGB8", "Etc2RGBA8", "EacR11Unorm", "EacR11Snorm", "EacRG11Unorm", "EacRG11Snorm"}
image_types = {"2DArray", "3D"}

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
        os.write(tmp_file, "\n".join(shader_sources).replace("${type}", image_type))
        os.close(tmp_file)
        out_spv_path = os.path.join(shader_source_path, shader_source_name + "_" + image_type + ".spv")
        if os.path.exists(out_spv_path):
            os.remove(out_spv_path)
        ret = os.system(" ".join([glslangValidator_path, "--spirv-val", "--target-env", "vulkan1.1", "-V", "-o", out_spv_path, tmp_file_name]))
        if ret != 0:
            print "Compiling %s got return code %d" % (out_spv_path, ret)
            print "intermediate source file: %s" % tmp_file_name
        else:
            print "Compiled %s successfully" % out_spv_path
            os.remove(tmp_file_name)
