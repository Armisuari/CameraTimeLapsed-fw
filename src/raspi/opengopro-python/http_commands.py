import json
import requests

def send(command):
    response = requests.get(command, timeout = 10)
    response.raise_for_status()
    response_string = json.dumps(response.json())
    response_json = json.loads(response_string)
    return response_json
