def cc_interface_binary(
        name,
        win_def_file,
        data = None,
        tags = None,
        visibility = None,
        linkopts = None,
        mac_entitlements = "//external/qemu:platform/darwin-arm64/entitlements.plist",
        **kwargs):
    """Creates a C++ binary and a shared library with an interface library.

    This can be used to create plugins, i.e. dlls that can be loaded into the main executable. It will
    create an interface library to which your plugin can link. The name of the interface library
    is `${name}.if`

    All the symbols in the win_def_file will be exported from the executable.
    On darwin the executable will be signed with the provided entitlements.


    Args:
        name (str): The name of the C++ binary target.
        win_def_file (str): The path to the Windows DEF file for the shared library.
        data (list, optional): Additional data files to be included in the C++ binary target. Defaults to None.
        tags (list, optional): A list of tags to be applied to the targets. Defaults to None.
        visibility (list, optional): A list of labels that control the visibility of the targets. Defaults to None.
        linkopts (list, optional): Additional linker options for the shared library target. Defaults to None.
        mac_entitlements (str): The path to the entlitlements to be used (Darwin only)
        **kwargs: Additional arguments to be passed to the C++ binary and shared library targets.

    Returns:
        None

    Example:
        cc_interface_binary(
            name = "my_binary",
            win_def_file = "my_binary.def",
            data = ["my_data.txt"],
            visibility = ["//visibility:public"],
            linkopts = ["-lmylib"],
            deps = [":my_lib"],
        )

    In this example, the `cc_interface_binary` function creates:
    1. A C++ binary target named `my_binary`.
    2. An interface library target named `my_binary.if`.

    Other targets can link against the interface library `my_binary.if`, and can be loaded by `my_binary`
    """
    dll_name = "_%s_dll" % name
    interface_filegroup_name = "_%s.if" % name
    interface_import_name = "%s.if" % name

    local_linkopts = linkopts or []
    local_linkopts = local_linkopts + select({
        "@platforms//os:windows": [
            "/DEF:$(location %s)" % win_def_file,
        ],
        "@platforms//os:macos": [
            "-framework Cocoa",
            "-framework CoreVideo",
            "-framework CoreAudio",
            "-framework vmnet",
        ],
        "//conditions:default": [],
    })

    local_data = data or []
    local_data.append(win_def_file)
    native.cc_binary(
        name = name + "_std",
        tags = tags,
        visibility = visibility,
        win_def_file = win_def_file,
        linkopts = local_linkopts,
        data = local_data,
        **kwargs
    )

    # Intentionally omits data so that the main binary target can data-depend on
    # a shared library dynamically linked against it without creating a cyclic
    # dependency.
    native.cc_binary(
        name = dll_name,
        linkshared = True,
        tags = ["manual"],
        visibility = ["//visibility:private"],
        win_def_file = win_def_file,
        linkopts = linkopts,
        **kwargs
    )

    # Sign the binary on darwin.
    native.genrule(
        name = name + "_signer",
        srcs = [name + "_std", mac_entitlements],
        outs = [name + "_signed"],
        executable = True,
        exec_compatible_with = ["@platforms//os:macos"],
        target_compatible_with = ["@platforms//os:macos"],
        cmd = "cp -L $(location {name}_std) \"$@\" && /usr/bin/codesign -s - --entitlements $(location {entitlements}) \"$@\"".format(name = name, entitlements = mac_entitlements),
    )

    native.filegroup(
        name = interface_filegroup_name,
        srcs = [":" + dll_name],
        output_group = "interface_library",
        tags = ["manual"],
        visibility = ["//visibility:private"],
    )

    native.cc_import(
        name = interface_import_name,
        interface_library = ":" + interface_filegroup_name,
        system_provided = True,
        tags = ["manual"],
        visibility = visibility,
    )

    native.alias(name = name, actual = select({
        "@platforms//os:macos": ":" + name + "_signed",
        "//conditions:default": ":" + name + "_std",
    }), visibility = visibility)
