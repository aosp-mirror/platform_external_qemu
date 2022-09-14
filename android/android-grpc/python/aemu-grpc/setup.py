"""
    Setup file for aemu-grpc.
    Use setup.cfg to configure your project.

    This file was generated with PyScaffold 4.3.
    PyScaffold helps you to put up the scaffold of your new Python project.
    Learn more under: https://pyscaffold.org/
"""
import os
import subprocess
import sys
from os import path

from setuptools import setup
from setuptools.command.build_py import build_py


class ProtoBuild(build_py):
    """
    This command automatically compiles all .proto files with `protoc` compiler
    and places generated files near them -- i.e. in the same directory.
    TODO: this should be release to the big PyPI for the general public to enjoy.
    """

    def run(self):
        here = path.abspath(path.dirname(__file__))
        for package in self.packages:
            packagedir = path.join(here, self.get_package_dir(package))

            for protofile in filter(
                lambda x: x.endswith(".proto"), os.listdir(packagedir)
            ):
                source = path.join(packagedir, protofile)
                output = source.replace(".proto", "_pb2.py")

                if not path.exists(output) or (
                    path.getmtime(source) > path.getmtime(output)
                ):
                    sys.stderr.write(f"Protobuf-compiling {source}\n")
                    subprocess.check_call(
                        [
                            sys.executable,
                            "-m",
                            "grpc_tools.protoc",
                            f"-I{packagedir}",
                            f"--python_out={packagedir}",
                            f"--grpc_python_out={packagedir}",
                            source,
                        ]
                    )
        super().run()


if __name__ == "__main__":
    try:
        setup(
            # use_scm_version={"version_scheme": "no-guess-dev", "root": "../../../"},
            cmdclass={"build_py": ProtoBuild},
        )
    except:  # noqa
        print(
            "\n\nAn error occurred while building the project, "
            "please ensure you have the most updated version of setuptools, "
            "setuptools_scm and wheel with:\n"
            "   pip install -U setuptools setuptools_scm wheel\n\n"
        )
        raise
