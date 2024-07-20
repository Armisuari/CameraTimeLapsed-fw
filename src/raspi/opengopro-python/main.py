import argparse
import asyncio
import subprocess

from open_gopro import Params, WirelessGoPro, constants, proto
from open_gopro.logger import setup_logging
from open_gopro.util import add_cli_args_and_parse, ainput
from rich.console import Console

import serial
import json
import os

import media_handler
import http_commands
import livestream
import config

console = Console()  # rich consoler printer

async def main(args: argparse.Namespace) -> None:
    setup_logging(__name__, args.log)
    ser = serial.Serial('/dev/ttyAMA0', 115200, timeout=0.5)
    while True:
        async with WirelessGoPro(args.identifier) as gopro:
            request_config(ser)
            console.print("[bright_cyan]Waiting for json command..")
            
            while True:
                if ser.in_waiting:
                    serial_string = ser.readline().decode("utf-8")
                    console.print(f"[bright_yellow]Received from serial: {serial_string}")
                    
                    if is_json(serial_string):	#only decode if serial_string in json format
                        json_data = json.loads(serial_string)
                    else:
                        json_data = ""
                        console.print("[red3]data is not in json format")
                        
                    if is_need_http_connection(json_data):
                        check_if_connected_to_gopro_AP()

                    if "capture" in json_data:
                        assert (await gopro.ble_command.set_shutter(shutter=Params.Toggle.ENABLE)).ok
                        media_handler.download_last_captured_media()
                        # console.print(f"[yellow]Connecting to {config.wifi_ssid}"
                        # os.system("sudo nmcli d wifi connect {} password {}".format(config.wifi_ssid, config.wifi_password))
                        # console.print(f"[yellow]Connected to {config.wifi_ssid}")
                        # os.chdir("gdrive_auto_backup_files")
                        # os.system("npm start") 
                    
                    if "reqConfig" in json_data:
                        request_config(ser)
                    if "stream" in json_data:
                        start_stream = json_data['stream']
                        if start_stream == 1:
                            await livestream.start(args, gopro)
                        elif start_stream == 0:
                            await livestream.stop(args, gopro)
                            break #break from while true so raspi could enable the gopro AP
                        
                    if "shutter" in json_data:	#check if "shutter" key exist in json
                        shutter = json_data['shutter']
                        if shutter != config.SHUTTER_VALUE:
                            command = config.GOPRO_BASE_URL + "/gp/gpControl/setting/" + config.SHUTTER_ID + "/" + str(shutter)
                            response = http_commands.send(command)
                            console.print(f"[yellow]Changing shutter speed into {config.SHUTTER[shutter]}")
                            config.SHUTTER_VALUE =  shutter
                    if "iso" in json_data:
                        iso = json_data['iso']
                        if iso != config.ISO_VALUE:
                            command = config.GOPRO_BASE_URL + "/gp/gpControl/setting/" + config.ISO_ID + "/" + str(iso)
                            response = http_commands.send(command)
                            console.print(f"[yellow]Changing ISO into {config.ISO[iso]}")
                            config.ISO_VALUE = iso
                    if "awb" in json_data:
                        awb = json_data['awb']
                        if awb != config.AWB_VALUE:
                            command = config.GOPRO_BASE_URL + "/gp/gpControl/setting/" + config.AWB_ID + "/" + str(awb)
                            response = http_commands.send(command)
                            console.print(f"[yellow]Changing AWB into {config.AWB[awb]}")
                            config.AWB_VALUE = awb
                    if "ev" in json_data:
                        ev = json_data['ev']
                        if ev != config.EV_VALUE:
                            command = config.GOPRO_BASE_URL + "/gp/gpControl/setting/" + config.EV_ID + "/" + str(ev)
                            response = http_commands.send(command)
                            console.print(f"[yellow]Changing EV into {config.EV[ev]}")
                            config.EV_VALUE = ev

def request_config(ser: serial):
    command = config.GOPRO_BASE_URL + "/gp/gpControl/status"
    settings_json = http_commands.send(command)
    console.print("[yellow]Getting current GoPro settings..")
    
    
    config.SHUTTER_VALUE = settings_json['settings'][config.SHUTTER_ID]  #int
    config.ISO_VALUE = settings_json['settings'][config.ISO_ID]
    config.AWB_VALUE = settings_json['settings'][config.AWB_ID]
    config.EV_VALUE = settings_json['settings'][config.EV_ID]
    
    #build JSON using dictionary
    settings_json = {
        "shutter" : config.SHUTTER[config.SHUTTER_VALUE],
        "iso" : config.ISO[config.ISO_VALUE],
        "awb" : config.AWB[config.AWB_VALUE],
        "ev" : config.EV[config.EV_VALUE],
    }
    json_string = json.dumps(settings_json)
    json_string = json_string.encode()
    console.print(f"[bright_yellow]Sent to serial: {json_string}")
    ser.write(json_string)

def is_need_http_connection(json_data):
    for command in config.need_http:
        if command in json_data:
            return True
    return False

def check_if_connected_to_gopro_AP():
    if is_connected_to_gopro_AP():
        print("Already connected to GoPro AP")
    else:
        print("Connecting to GoPro AP")
        os.system("sudo nmcli d wifi connect {} password {}".format(config.gopro_ssid, config.gopro_password))
    
def is_connected_to_gopro_AP():
    ps = subprocess.Popen(['iwgetid'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        output = subprocess.check_output(('grep', 'ESSID'), stdin=ps.stdout)
        if config.gopro_ssid.encode() in output:
            return True
        else:
            return False
    except subprocess.CalledProcessError:
        # grep did not match any lines
        print("No wireless networks connected")

def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Connect to the GoPro via BLE only, configure then start a Livestream, then display it with CV2."
    )
    parser.add_argument("--ssid", type=str, help="WiFi SSID to connect to.", default=config.wifi_ssid)
    parser.add_argument("--password", type=str, help="Password of WiFi SSID.", default=config.wifi_password)
    parser.add_argument("--url", type=str, help="RTMP server URL to stream to.", default=config.rtmp_URL)
    parser.add_argument("--encode", type=bool, help="Minimum bitrate.", default=0)
    parser.add_argument("--min_bit", type=int, help="Minimum bitrate.", default=1000)
    parser.add_argument("--max_bit", type=int, help="Maximum bitrate.", default=1000)
    parser.add_argument("--start_bit", type=int, help="Starting bitrate.", default=1000)
    parser.add_argument(
        "--resolution",
        help="Resolution.",
        choices=list(proto.EnumWindowSize.values()),
        default=proto.EnumWindowSize.WINDOW_SIZE_720,
    )
    parser.add_argument(
        "--fov", help="Field of View.", choices=list(proto.EnumLens.values()), default=proto.EnumLens.LENS_LINEAR
    )
    return add_cli_args_and_parse(parser, wifi=False)


def is_json(myJson):
    try:
        json.loads(myJson)
    except ValueError as e:
         return False
    return True

if __name__ == "__main__":
    asyncio.run(main(parse_arguments()))
