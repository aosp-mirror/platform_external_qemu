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

import asyncio
import os
import sys
import time
from functools import update_wrapper

import click
import google.protobuf.text_format
from aemu.proto.snapshot_pb2 import Image
from aemu.proto.snapshot_service_pb2 import SnapshotDetails, SnapshotPackage
from snaptool.snapshot import AsyncSnapshotService, SnapshotService


def coro(f):
    f = asyncio.coroutine(f)

    def wrapper(*args, **kwargs):
        loop = asyncio.get_event_loop()
        return loop.run_until_complete(f(*args, **kwargs))

    return update_wrapper(wrapper, f)


def fmt_proto(msg):
    """Formats a protobuf message as a single line."""
    return google.protobuf.text_format.MessageToString(msg, as_one_line=True)


def epoch_fmt(epoch):
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(epoch))


def sizeof_fmt(num):
    for x in ["bytes", "KB", "MB", "GB", "TB"]:
        if num < 1024.0:
            return "%3.1f %s" % (num, x)
        num /= 1024.0


@click.group()
@click.option("--grpc", default="", help="Port or address of the grpc service to use.")
@click.pass_context
def cli(ctx, grpc):
    ctx.obj = AsyncSnapshotService(grpc)
    pass


@cli.command()
@click.pass_obj
@coro
async def list(snapshotService):
    snaps = await snapshotService.lists()
    if snaps:
        for snap in snaps:
            print(
                "{}: {} ({})".format(
                    snap.snapshot_id,
                    SnapshotDetails.LoadStatus.Name(snap.status),
                    sizeof_fmt(snap.size),
                )
            )
    else:
        print("No snapshots available.")


@cli.command()
@click.pass_obj
@click.argument("name")
@click.option("--all", "-a", default=False, is_flag=True, help="Print the raw data.")
@coro
async def info(snapshotService, name, all):
    snap = await snapshotService.info(name)
    if not snap:
        print("{} does not exist".format(name))

    if all:
        print(fmt_proto(snap))
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
@coro
async def pull(snapshotService, name, dest, compress):
    fmt = SnapshotPackage.Format.TAR
    if compress:
        fmt = SnapshotPackage.Format.TARGZ
    msg, fname = await snapshotService.pull(name, dest, fmt)


@cli.command()
@click.pass_obj
@click.argument("src")
@coro
async def push(snapshotService, src):
    if not os.path.exists(src):
        print(f"File {src} does not exist!")
        sys.exit(1)
    await snapshotService.push(src)


@cli.command()
@click.pass_obj
@click.argument("name")
@coro
async def load(snapshotService, name):
    await snapshotService.load(name)


@cli.command()
@click.pass_obj
@click.argument("name")
@click.argument("logical_name")
@click.argument("description")
@coro
async def update(snapshotService, name, logical_name, description):
    await snapshotService.update(name, description, logical_name)


@cli.command()
@click.pass_obj
@click.argument("name")
@coro
async def save(snapshotService, name):
    await snapshotService.save(name)


@cli.command()
@click.pass_obj
@click.argument("name")
@coro
async def delete(snapshotService, name):
    await snapshotService.delete(name)
