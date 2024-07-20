rtmp_URL = "rtmp://angkasatimelapse.com/live/mykey"
wifi_ssid = "BMZimages"
wifi_password = "bennamazarina"
gopro_ssid = "GoProHero9Angkasa"
gopro_password = "NPc-hg6-S5S"
stream_started = False
GOPRO_BASE_URL = "http://10.5.5.9:8080"

#======ID=====
SHUTTER_ID = "146"    #146 photo 145 video
SHUTTER = {
     0 : "AUTO",
     1 : "1/125",
     2 : "1/250",
     3 : "1/500",
     4 : "1/1000",
     5 : "1/2000",
}

ISO_ID = "75"         #75 photo 102 video ISO MIN
ISO = {
     5 : "3200",
     4 : "1600",
     0 : "800",
     1 : "400",
     2 : "200",
     3 : "100",
}

AWB_ID = "115"
AWB = {
    3 : "6500K",
    7 : "6000K",
    2 : "5500K",
    12 : "5000K",
    11 : "4500K",
    0 : "AUTO",
    4  : "NATIVE",
    5 : "4000K",
    10 : "3200K",
    9 : "2800K",
    8 : "2300K",
}

EV_ID = "118"
EV = {
    8 : "-2.0",
    7 : "-1.5",
    6 : "-1.0",
    5 : "-0.5",
    4 : "0.0",
    3 : "0.5",
    2 : "1.0",
    1 : "1.5",
    0 : "2.0",
}

SHUTTER_VALUE = 0
ISO_VALUE = 0
AWB_VALUE = 0
EV_VALUE = 0

need_http = ["capture","reqConfig","shutter","iso","awb","ev"]
