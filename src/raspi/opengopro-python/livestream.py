import argparse
import asyncio
from typing import Any
from open_gopro import Params, WirelessGoPro, constants, proto
from rich.console import Console
from config import *

console = Console()  # rich consoler printer

async def start(args: argparse.Namespace, gopro: WirelessGoPro):
    # async with WirelessGoPro(args.identifier, enable_wifi=False) as gopro:
    await gopro.ble_command.set_shutter(shutter=Params.Toggle.DISABLE)
    await gopro.ble_command.register_livestream_status(
        register=[proto.EnumRegisterLiveStreamStatus.REGISTER_LIVE_STREAM_STATUS_STATUS]
    )

    console.print(f"[yellow]Connecting to {wifi_ssid}...")
    await gopro.connect_to_access_point(wifi_ssid, wifi_password)

    # Start livestream
    livestream_is_ready = asyncio.Event()

    async def wait_for_livestream_start(_: Any, update: proto.NotifyLiveStreamStatus) -> None:
        if update.live_stream_status == proto.EnumLiveStreamStatus.LIVE_STREAM_STATE_READY:
            livestream_is_ready.set()

    console.print("[yellow]Configuring livestream...")
    gopro.register_update(wait_for_livestream_start, constants.ActionId.LIVESTREAM_STATUS_NOTIF)
    await gopro.ble_command.set_livestream_mode(
        url=rtmp_URL,
        window_size=args.resolution,
        minimum_bitrate=args.min_bit,
        maximum_bitrate=args.max_bit,
        starting_bitrate=args.start_bit,    
        lens=args.fov,
    )

    # Wait to receive livestream started status
    console.print("[yellow]Waiting for livestream to be ready...\n")
    await livestream_is_ready.wait()

    # TODO Is this still needed?
    await asyncio.sleep(2)

    console.print("[yellow]Starting livestream")
    assert (await gopro.ble_command.set_shutter(shutter=Params.Toggle.ENABLE)).ok

    console.print("[yellow]Livestream is now streaming and should be available for viewing.")
            
async def stop(args: argparse.Namespace, gopro: WirelessGoPro):
    await gopro.ble_command.set_shutter(shutter=Params.Toggle.DISABLE)
    await gopro.ble_command.release_network()
