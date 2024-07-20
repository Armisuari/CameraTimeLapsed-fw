import sys
import json
import argparse
from typing import Dict, Any

import requests

from tutorial_modules import GOPRO_BASE_URL, logger


def get_media_list() -> Dict[str, Any]:
    """Read the media list from the camera and return as JSON

    Returns:
        Dict[str, Any]: complete media list as JSON
    """
    # Build the HTTP GET request
    url = GOPRO_BASE_URL + "/gopro/media/list"
    logger.info(f"Getting the media list: sending {url}")

    # Send the GET request and retrieve the response
    response = requests.get(url, timeout=10)
    # Check for errors (if an error is found, an exception will be raised)
    response.raise_for_status()
    logger.info("Command sent successfully")
    # Log response as json
    logger.info(f"Response: {json.dumps(response.json(), indent=4)}")

    return response.json()

def get_last_captured_media():
    media_list = json.dumps(get_media_list())
    media_list_json = json.loads(media_list)
    last_captured_media = media_list_json["media"][0]["fs"][-1]['n']
    print(last_captured_media)
    return last_captured_media

def download_last_captured_media():
    last_captured_media = get_last_captured_media()
    url = GOPRO_BASE_URL + "/videos/DCIM/100GOPRO/" + last_captured_media
    logger.info(f"Downloading the media: {last_captured_media}")
    print(url)

    with requests.get(url, stream=True, timeout=10) as request:
        request.raise_for_status()
        file = last_captured_media.split(".")[0] + ".jpg"
        with open("gdrive_auto_backup_files/images/"+file, "wb") as f:
            logger.info(f"receiving binary stream to {file}...")
            for chunk in request.iter_content(chunk_size=8192):
                f.write(chunk)

   # return response.json()
