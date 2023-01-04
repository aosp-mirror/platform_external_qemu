# Copyright 2022 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the',  help='License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an',  help='AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import re
from pathlib import Path
import os


class FeatureNotFound(Exception):
    pass


class FeatureParser:
    """Parses available features from the CMakeLists.txt file.

    This can be used to explicitly enable/disable features.
    """
    def __init__(self, source: Path) -> None:
        super().__init__()
        self.features = {}
        self.features_help = {}
        self.alt_name = {}
        self.parse(source)

    def _find_alias(self, feature: str) -> str:
        prefixes = ["OPTION_", "FEATURE_"]
        for prefix in prefixes:
            if feature.startswith(prefix):
                return feature[len(prefix) :]

        return feature

    def feature_to_cmake_parameter(self, feature_str: str) -> str:
        """Returns a cmake parameter that can be passed on the the generator.

        Args:
            feature_str (str): Feature that should be turned on or off.

        Raises:
            FeatureNotFound: If the feature (or its alias) does not exist.

        Returns:
            str: Parameter that needs to be passed to cmake.
        """
        enable = True

        look_for = feature_str.upper()
        if look_for.startswith("NO-"):
            enable = False
            look_for = look_for[3:]

        if look_for in self.alt_name:
            look_for = self.alt_name[look_for]

        if look_for in self.features:
            return f"-D{look_for}={enable}"

        raise FeatureNotFound(f"Feature {look_for} does not exist.")

    def parse(self, source: Path) -> None:
        """Parses a CMakeLists file by extracting all the set(xxx ... CACHE ...) lines

        Note: This is pretty simplistic and assumes that the CMakeLists.txt uses this
        pattern for defining features.

        We treat OPTION_ and FEATURE_ as a prefix.

        Args:
            source (Path): Path to the CMakeLists.txt file.
        """
        set_re = r"set\(\s*(\w+)\s+(FALSE|TRUE)\s+CACHE\s+BOOL\s+\"(.*)\"\s*\)"
        with open(source, "r", encoding="utf-8") as cmake:
            lines = cmake.read()
            for match in re.findall(set_re, lines):
                feature = match[0]
                alias = self._find_alias(feature)
                self.features[feature] = match[1].upper() == "TRUE"
                self.features_help[alias] = match[2]
                self.alt_name[alias] = feature

    def list_help(self):
        """Prints a help message listing all the features that are available."""
        if os.sys == "win32":
            os.system("")  # Activate ansi colors!

        green = "\N{ESC}[32;20m"
        reset = "\N{ESC}[0m"

        print("The following set of features are available:\n")
        for feature, _ in self.alt_name.items():
            print(f"{green}{feature.lower()}{reset}: {self.features_help[feature]}")

        print()
        print("Features can be explicitly turned on or off, by using [no-]feature.")
        print("Providing the flag will override the default found in CMakeLists.txt.")
        print("Look at CMakeLists.txt for the defaults, as they can vary by OS.")
        print()
        print("For example add the parameter:")
        first = list(self.alt_name.keys())[0].lower()
        print(f"   `--feature no-{first}` to explicitly disable the {first} feature, overriding the default.")
        print(f"   `--feature {first}` to explicitly enable the {first} feature, overriding the default.")
