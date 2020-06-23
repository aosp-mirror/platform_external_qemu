#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from __future__ import absolute_import, division, print_function

import os
import sys
import time

import click

from aemu.proto.snapshot_pb2 import Image
from aemu.proto.snapshot_service_pb2 import SnapshotPackage
from snaptool.snapshot import SnapshotService

def epoch_fmt(epoch):
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(epoch))


def sizeof_fmt(num):
    for x in ["bytes", "KB", "MB", "GB", "TB"]:
        if num < 1024.0:
            return "%3.1f %s" % (num, x)
        num /= 1024.0


@click.group()
@click.option(
    "--grpc", default="", help="Port of the grpc service to use."
)
@click.pass_context
def cli(ctx, grpc):
    ctx.obj = SnapshotService(grpc)
    pass


@cli.command()
@click.pass_obj
def list(snapshotService):
    snaps = snapshotService.lists()
    if snaps:
        for snap in snaps:
            print(snap)
    else:
        print("No snapshots available.")


@cli.command()
@click.pass_obj
@click.argument("name")
@click.option("--all", "-a", default=False, is_flag=True, help="Print the raw data.")
def info(snapshotService, name, all):
    snap = snapshotService.info(name)
    if not snap:
        print("{} does not exist".format(name))

    if all:
        print(snap)
    else:
        print(
            "{} : Created: {}".format(
                snap.snapshot_id, epoch_fmt(snap.details.creation_time)
            )
        )
        total_size = 0
        for img in snap.details.images:
            if img.present:
                print(
                    "  {:30}   {:>10}".format(
                        Image.Type.Name(img.type), sizeof_fmt(img.size)
                    )
                )
                total_size += img.size
        print("  {:30}   {:>10}".format("Total:", sizeof_fmt(total_size)))
        print("  {:10} {:>7}".format("Cpu cores:", snap.details.config.cpu_core_count))
        print(
            "  {:10} {:>7}".format(
                "Memory:", sizeof_fmt(snap.details.config.ram_size_bytes)
            )
        )


@cli.command()
@click.pass_obj
@click.argument("name")
@click.argument("dest", default=".")
@click.option("--compress/--no-compress", default=False)
def pull(snapshotService, name, dest, compress):
    fmt = SnapshotPackage.Format.TAR
    if compress:
        fmt = SnapshotPackage.Format.TARGZ
    msg, fname = snapshotService.pull(name, dest, fmt)


@cli.command()
@click.pass_obj
@click.argument("src")
def push(snapshotService, src):
    if not os.path.exists(src):
        print("File {} does not exist!".format(src))
        sys.exit(1)
    snapshotService.push(src)


@cli.command()
@click.pass_obj
@click.argument("name")
def load(snapshotService, name):
    snapshotService.load(name)


@cli.command()
@click.pass_obj
@click.argument("name")
def save(snapshotService, name):
    snapshotService.save(name)

@cli.command()
@click.pass_obj
@click.argument("name")
def delete(snapshotService, name):
    snapshotService.delete(name)
