# Copyright 2023 - The Android Open Source Project
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

import logging
from pathlib import Path
import platform
import os
import zipfile
import aemu.prebuilts.angle as angle
import aemu.prebuilts.qt as qt
import aemu.prebuilts.moltenvk as moltenvk

HOST_OS = platform.system().lower()

_prebuilts_dir_name = "prebuilts"
_prebuilts_zip_name = "sdk-repo-{target}-prebuilts-{build_number}.zip"
_prebuilt_funcs = {
    'qt': qt.buildPrebuilt,
    # Add more prebuilts here
}

if HOST_OS == "linux":
    _prebuilt_funcs.update({
        # TODO: Build angle for all platforms
        'angle': angle.buildPrebuilt,
    })

if HOST_OS == "darwin":
    _prebuilt_funcs.update({
        'moltenvk': moltenvk.buildPrebuilt,
    })

def buildPrebuilts(args):
    # out/prebuilts
    prebuilts_dir = os.path.join(args.out, _prebuilts_dir_name)
    build_list = args.prebuilts if args.prebuilts else _prebuilt_funcs.keys()
    logging.info("Prebuilts list: " + str(build_list))
    for prebuilt in build_list:
        logging.info(">> Building prebuilt [%s]", prebuilt)
        _prebuilt_funcs.get(prebuilt)(args, prebuilts_dir)
    logging.info("Done building prebuilts list. Prebuilts located at [%s]", prebuilts_dir)

    # zip entire prebuilts/ dir and put it in the destination directory
    prebuilts_zip = os.path.join(args.dist,
                                 _prebuilts_zip_name.format(target=args.target,
                                                            build_number=args.sdk_build_number))
    logging.info("Creating %s", prebuilts_zip)
    with zipfile.ZipFile(prebuilts_zip, "w", zipfile.ZIP_DEFLATED, allowZip64=True) as zipf:
        search_dir = Path(prebuilts_dir)
        for fname in search_dir.glob("**/*"):
            arcname = fname.relative_to(search_dir)
            logging.info("Adding %s as %s", fname, arcname)
            zipf.write(fname, arcname)
