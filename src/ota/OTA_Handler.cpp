#include "OTA_Handler.h"
#include <Update.h>

OtaHandler otaHandler;

const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

/*
 * Server Index Page
 */
const char *serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>" CONFIG_MAIN_FW_VERSION_STRING
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'><input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form); "
    "$.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', "
    "function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}}, "
    "false);"
    "return xhr;"
    "},"
    "success: function(d, s) {"
    "$('#prg').html(d);"
    "console.log(d)"
    "},"
    "error: function (a, b, c) {}"
    "});"
    "});"
    "</script>";

// Setup Function
void OtaHandler::setup(onOtaStateChange_t onStart /*=nullptr*/, onOtaStateChange_t onFinish /*=nullptr*/)
{
    ;
    server = new WebServer(80);

    _onStartCb = onStart;
    _onFinishedCb = onFinish;

    // return index page which is stored in serverIndex
    server->on("/", HTTP_GET, [&]()
               {
        server->sendHeader("Connection", "close");
        server->send(200, "text/html", loginIndex); });

    server->on("/serverIndex", HTTP_GET, [&]()
               {
        server->sendHeader("Connection", "close");
        server->send(200, "text/html", serverIndex); });

    // handling uploading firmware file
    server->on(
        "/update", HTTP_POST,
        [&]()
        {
            server->sendHeader("Connection", "close");
            String UpdateInfo = "Update Success";

            if (Update.hasError())
            {
                UpdateInfo = "Update Failed: ";
                UpdateInfo += Update.errorString();
                Serial.println(UpdateInfo);
            }

            server->send(200, "text/plain", UpdateInfo);

            if (!Update.hasError())
            {
                delay(50);
                ESP.restart();
            }

            if (_onFinishedCb)
            {
                _onFinishedCb();
            }
        },
        [&]()
        {
            HTTPUpload &upload = server->upload();

            if (upload.status == UPLOAD_FILE_START)
            {
                Serial.printf("Update: %s\n", upload.filename.c_str());
                Update.begin(UPDATE_SIZE_UNKNOWN);
                if (_onStartCb)
                {
                    _onStartCb();
                }
            }
            else if (upload.status == UPLOAD_FILE_WRITE && !Update.hasError())
            {
                // flashing firmware to ESP
                if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
                {
                    Update.printError(Serial);
                }
            }
            else if (upload.status == UPLOAD_FILE_END)
            {
                if (Update.end(true))
                { // true to set the size to the current progress
                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
                }
                else
                {
                    Update.printError(Serial);
                }
            }
        });

    server->begin();
    Serial.println("OTA sever started");
}

WebServer *OtaHandler::getServer()
{
    return server;
}

void OtaHandler::task(void *pv)
{
    vTaskDelay(1000);
    while (1)
    {
        otaHandler.getServer()->handleClient();
        // Serial.println("otaHandler.getServer()->handleClient()");
    }
    delete otaHandler.server;
    vTaskDelete(NULL);
}