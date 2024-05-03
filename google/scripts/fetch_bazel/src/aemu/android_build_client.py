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

import io
import json
import logging
import platform
import shutil
import subprocess
from getpass import getuser
from pathlib import Path

import googleapiclient.discovery
import googleapiclient.http
from oauth2client.client import AccessTokenCredentials
from tqdm import tqdm


def raise_if_none(x, msg):
    if x is None:
        raise ValueError(msg)


class AndroidBuildClient(object):
    """A client to talk to go/ab"""

    def __init__(self, token):
        if not token:
            if platform.system() != "Linux":
                raise ValueError(
                    """You will have to manually generate a token on glinux by running:

~/go/bin/oauth2l fetch --sso $USER@google.com androidbuild.internal

Pass the generated token in using the --token option.
"""
                )
            else:
                token = self.obtain_token()

        logging.debug("Using token: %s", token)
        credentials = AccessTokenCredentials(token, "aemu-build-client/1.0")
        self.service = googleapiclient.discovery.build(
            "androidbuildinternal",
            "v3",
            discoveryServiceUrl="https://www.googleapis.com/discovery/v1/apis/androidbuildinternal/v3/rest",
            credentials=credentials,
        )

    def obtain_token(self):
        token_proc = shutil.which("oauth2l")
        if not token_proc:
            token_proc = shutil.which("oauth2l", path=Path.home() / "go" / "bin")
        raise_if_none(
            token_proc,
            "Unable to find oauth2l on the path or in ~/go/bin, please install it. See http://go/oauth2l for details.",
        )
        try:
            return subprocess.check_output(
                [
                    token_proc,
                    "fetch",
                    "--sso",
                    f"{getuser()}@google.com",
                    "androidbuild.internal",
                ],
                timeout=60,
            ).decode("utf-8")
        except subprocess.TimeoutExpired:
            logging.error("Timeout while trying to retrieve token, have you run gcert?")

    def get_latest_build_id(self, branch, build_target):
        raise_if_none(branch, "branch")

        request = self.service.build().list(
            branch=branch,
            target=build_target,
            buildType="submitted",
            successful=True,
            sortingType="buildId",
            fields="builds/buildId",
            maxResults=1,
        )
        response = request.execute()
        if "builds" in response:
            builds = response["builds"]
            if len(builds) == 1:
                return builds[0]["buildId"]
            else:
                raise RuntimeError('No builds found for "%s" branch' % (branch))
        else:
            raise RuntimeError(
                "No builds found for %s/%s, response=%s"
                % (branch, build_target, json.dumps(response))
            )

    def list_builds(
        self, branch, target, buildId=None, endBuildId=None, results=None, success=True
    ):
        """Returns a list of build ids that meets the criteria.

        Args:
            client: the Android Build Client to work with.
            branch: the branch
            target: the target
            results: The max number of builds to fetch.
            endBuildId: The latest buildId in the range, or none if we should use results
            success: look for successful or failed builds.
        Returns:
            A build id
        """
        logging.debug(
            f'Listing submitted builds buildType="submitted", branch={branch}, target={target}, buildId={buildId}, successful={success}, maxResults={results}'
        )
        if endBuildId:
            result = (
                self.service.build()
                .list(
                    buildType="submitted",
                    branch=branch,
                    target=target,
                    startBuildId=buildId,
                    endBuildId=endBuildId,
                    successful=success,
                    maxResults=results,
                )
                .execute()
            )
        else:
            result = (
                self.service.build()
                .list(
                    buildType="submitted",
                    branch=branch,
                    target=target,
                    buildId=buildId,
                    successful=success,
                    maxResults=results,
                )
                .execute()
            )

        logging.debug("Server response: %s", result)
        builds = result.get("builds", [])
        if not builds:
            return None
        return [x["buildId"] for x in builds]

    def list_artifacts(self, bid, build_target):
        raise_if_none(bid, "no bid provided")
        raise_if_none(build_target, "build_target should not be none")

        request = self.service.buildartifact().list(
            buildId=bid,
            target=build_target,
            attemptId="latest",
            fields="artifacts/name",
            maxResults=1000,
        )
        response = request.execute()
        if "artifacts" in response:
            return map(lambda a: a["name"], response["artifacts"])
        else:
            raise RuntimeError(
                "No artifacts found for %s at %s, response=%s"
                % (build_target, bid, json.dumps(response))
            )

    def fetch_bits(self, dst, bid, build_target, artifact):
        """Downloads the artifact pointed by the bid/build_target/artifact path from go/ab
        into dest.

        Args:
          dst: destination path
          bid: build id
          build_target: build target
          artifact: artifact name

        Returns:
          None
        """

        raise_if_none(dst, "dst")
        raise_if_none(bid, "bid")
        raise_if_none(build_target, "build_target")
        raise_if_none(artifact, "artifact")

        request = self.service.buildartifact().get(
            buildId=bid, target=build_target, attemptId="latest", resourceId=artifact
        )
        response = request.execute()
        total_size = int(response.get("size", 0))

        request = self.service.buildartifact().get_media(
            buildId=bid, target=build_target, attemptId="latest", resourceId=artifact
        )

        with tqdm(
            total=total_size, unit="B", unit_scale=True, unit_divisor=1024, miniters=1
        ) as progress_bar:
            with io.FileIO(dst, mode="wb") as fh:
                downloader = googleapiclient.http.MediaIoBaseDownload(fh, request)
                done = False
                while not done:
                    status, done = downloader.next_chunk(num_retries=3)
                    if status:
                        progress_bar.update(status.resumable_progress - progress_bar.n)
