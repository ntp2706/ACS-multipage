#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <WebSocketsClient.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "FS.h"
#include "SD_MMC.h" 

#define SSID "NTP"
#define PASSWORD "qwert123"

const String FIREBASE_STORAGE_BUCKET = "accesscontrolsystem-4f265.appspot.com";

const String esp8266LocalIP = "192.168.193.100";

String Feedback=""; 
String Command="";
String cmd="";
String pointer="";
int flashValue = 0;

String nameSend;
String uniSend;
String roomSend;
String timestampSend;

String countDatabaseSend;
String countDatabaseReceive;

String countLogSend;
String countLogReceive;

byte receiveState=0;
byte cmdState=1;
byte strState=1;
byte questionstate=0;
byte equalstate=0;
byte semicolonstate=0;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiServer server(80);
WiFiClient client;
WebSocketsClient webSocket;

void sendCountLog() {
  if(!SD_MMC.begin()) {
    Serial.println("Không truy cập được thẻ SD");
    return;
  }
  fs::FS &fs = SD_MMC; 
  File fileLog = fs.open("/countLog.txt");
  if (fileLog) {
    countLogSend = fileLog.readStringUntil('\n');
  } else {
      Serial.println("Lỗi mở file countLog.txt");
    }
  fileLog.close();
  SD_MMC.end();
  String countLogTemp = "newCountLog:" + countLogSend;
  webSocket.sendTXT(countLogTemp);
  Serial.print("Đã gửi CountLog: ");
  Serial.println(countLogSend);

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);  
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  String message = String((char *)data).substring(0, len);

  if(!SD_MMC.begin()) {
    Serial.println("Không truy cập được thẻ SD");
    return;
  }

  if (message.startsWith("newCountDatabase:")) {
    countDatabaseReceive = message.substring(strlen("newCountDatabase:"));

    Serial.println("Đã nhận thông tin:");
    Serial.print("newCountDatabase: ");
    Serial.println(countDatabaseReceive);

    File file = SD_MMC.open("/countDatabase.txt", FILE_WRITE);
    if (file) {
      file.println(countDatabaseReceive);
      file.close();
      Serial.print("Đã cập nhật newCountDatabase: ");
      Serial.println(countDatabaseReceive);
    } else {
        Serial.println("Lỗi cập nhật newCountDatabase");
        file.close();
        webSocket.sendTXT("processError");
      }
  }

  if (message.startsWith("newCountLog:")) {
    countLogReceive = message.substring(strlen("newCountLog:"));

    Serial.println("Đã nhận thông tin:");
    Serial.print("newCountLog: ");
    Serial.println(countLogReceive);

    File file = SD_MMC.open("/countLog.txt", FILE_WRITE);
    if (file) {
      file.println(countLogReceive);
      file.close();
      countLogSend = "newCountLog:" + countLogReceive;
      webSocket.sendTXT(countLogSend);
      Serial.print("Đã cập nhật newCountLog: ");
      Serial.println(countLogReceive);
    } else {
        Serial.println("Lỗi cập nhật newCountLog");
        file.close();
        webSocket.sendTXT("processError");
      }
    sendCountLog();
  }
  
  SD_MMC.end();

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);
}

void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("Đã kết nối vào WebSocket");
      sendCountLog();
      break;
    case WStype_DISCONNECTED:
      Serial.println("Đã ngắt kết nối với WebSocket");
      break;
    case WStype_TEXT:
      handleWebSocketMessage(NULL, payload, length);
      break;
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 0;
    config.fb_count = 2;
  } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.jpeg_quality = 0;
      config.fb_count = 1;
    }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera lỗi 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  sensor_t * s = esp_camera_sensor_get();

  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);
    s->set_brightness(s, 1);
    s->set_saturation(s, 0);
  }

  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_hmirror(s, 1); 

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Đã kết nối WiFi.");
  Serial.println(WiFi.localIP());
  server.begin();

  webSocket.begin(esp8266LocalIP, 81, "/");
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(1000);

  while (!SD_MMC.begin()) {
    Serial.println("Không thể khởi tạo thẻ SD");
    delay(500);
  }
  Serial.println("Thẻ SD sẵn sàng");

  if (!SD_MMC.exists("/countDatabase.txt")) {
    Serial.println("Tạo file countDatabase.txt bắt đầu bằng 1");
    while (true) {
      File file = SD_MMC.open("/countDatabase.txt", FILE_WRITE);
      if (file) {
        file.println("1");
        file.close();
        Serial.println("Tạo file countDatabase.txt thành công");
        break;
      } else {
          Serial.println("Tạo lại file countDatabase.txt");
          delay(500);
        }
    }
  }

  delay(100);

  if (!SD_MMC.exists("/countLog.txt")) {
    Serial.println("Tạo file countLog.txt bắt đầu bằng 2");
    while (true) {
      File file = SD_MMC.open("/countLog.txt", FILE_WRITE);
      if (file) {
        file.println("2");
        file.close();
        Serial.println("Tạo file countLog.txt thành công");
        break;
      } else {
          Serial.println("Tạo lại file countLog.txt");
          delay(500);
        }
    }
  }

  SD_MMC.end();

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);
}

static const char PROGMEM INDEX_HTML[] = R"rawliteral(

<!DOCTYPE HTML><html>
<head>
  <title>ACS Input Page</title>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="preconnect" href="https://fonts.gstatic.com">
  <link href="https://fonts.googleapis.com/css2?family=Roboto&display=swap" rel="stylesheet">
<style>
    body {font-family: 'Roboto', sans-serif; background-color: #f4f4f4;}
    .flashContainer {display: flex; flex-direction: column; align-items: center; margin: 28px auto auto 1024px;}
    .flashLabel img {width: 40px; height: 40px; cursor: pointer; border-radius: 10px; transition: transform 0.3s, box-shadow 0.3s;}
    .flashLabel img:hover {transform: scale(1.1); box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);}
    .barContainer {display: flex; margin-top: 0px; padding: 10px; background-color: #fff; border-radius: 10px; box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1); transition: opacity 0.3s, transform 0.3s;}
    .flashBar {width: 4px; height: 200px; -webkit-appearance: slider-vertical;}
    input[type="text"] {width: 520px; padding: 10px; margin-top: 10px; margin-bottom: 10px; background-color: #fff; border: 1px solid #ccc; box-sizing: border-box; border-radius: 4px; cursor: pointer; font-size: 20px; font-weight: bolder;}
    input[type="text"]:focus {outline: none; border-color: #4CAF50; box-shadow: 0 0 5px #4CAF50;}
    .input {display: flex; flex-direction: row; width: 900px; margin: -40px auto auto auto; padding: 20px; box-shadow: 0px 0px 20px rgba(0,0,0,0.2); background-color: #fff; border-radius: 5px;}
    .input > div {margin: 10px auto; padding: 10px;}
    .inputText {transform: translateX(20px);}
    .inputCamera {width: 320px; height: 300px; display: flex; flex-direction: column; background-color: #fff;}
    .inputCamera > div {padding: 0px; margin: 0px auto;}
    button {background-color: #4CAF50; margin: 10px auto; padding: 10px; color: #fff; border: none; border-radius: 5px; cursor: pointer;}
    button:hover {background-color: #45a049; transform: scale(1.1); font-weight: bold;}
    .submit {width: 520px; padding: 10px; margin-top: 10px; background-color: #fff; border: 2px solid #4CAF50; color: #4CAF50; box-sizing: border-box; border-radius: 4px; cursor: pointer; font-size: 15px;}
    .submit:hover {background-color: #4CAF50; color: #fff;}
    .showListContainer {width: 40px; height: 40px; margin: 20px auto auto auto; transform: translateX(-440px); text-align: center; background-color: #4CAF50; border: 2px solid #fff; border-radius: 10px; display: flex;}
    .showListBtn {width: 0; height: 0; border-left: 12px solid transparent; border-right: 12px solid transparent; border-top: 16px solid #fff; cursor: pointer; margin: auto auto auto auto;}
    .showListBtn input {display: none;}
    .list {display: flex; flex-direction: row; margin-top: 0px; width: 900px; margin: 12px auto; padding: 20px; box-shadow: 0px 0px 20px rgba(0,0,0,0.2); background-color: #fff; border-radius: 5px;}
    .list > div {margin: 10px auto;}
    img {width: 320px; height: 240px; background-color: #fff;}
    .hidden {display: none;}
  </style>
</head>
<body>
  <div class="flashContainer">
    <input type="checkbox" class="hidden" id="flashCheckbox" onchange="showBar(this)">
    <label class="flashLabel" for="flashCheckbox">
      <img src="https://firebasestorage.googleapis.com/v0/b/accesscontrolsystem-4f265.appspot.com/o/src%2Flight.jpg?alt=media">
    </label>
    <div class="barContainer hidden" id="barContainer">
      <input type="range" class="default-action flashBar" id="flash" min="0" max="255" value="0" orient="vertical">
    </div>
  </div>
  <div class="input" id="inputContainer">
    <div class="inputCamera">
      <img id="stream" src="" />
      <div class="feature">
        <button id="startStream">Stream</button>
        <button id="getStill">Still</button>
        <button id="saveImage" class="hidden">Save</button>
        <button id="stopStream">Stop</button>
        <button id="listImages" class="hidden">List</button>
      </div>
    </div>
    <form id="infoForm" onsubmit="updateTimestamp(event)">
      <div class="inputText">
        Họ và tên: <input id="name" type="text" name="name" autocomplete="off">
        <br>
        Trường: <input id="university" type="text" name="university" autocomplete="off">
        <br>
        Phòng: <input id="room" type="text" name="room" autocomplete="off">
        <br>
        <input type="hidden" id="timestamp" name="timestamp">
        <input class="submit" type="submit" value="Xác nhận">
      </div>
    </form>
  </div>
  <div class="showListContainer">
    <label class="showListBtn">
      <input type="checkbox" onchange="showList(this)">
    </label>
  </div>
  <div class="list hidden" id="listContainer">
    <div class="showImage">
      <img id="showContainer" src="" />
    </div>
    <div>
      <iframe id="ifr" height="240px" width="520px"></iframe>
    </div>
  </div>
  
<script>

function showBar(checkbox) {
    const barContainer = document.getElementById('barContainer');
    const inputContainer = document.getElementById('inputContainer');
    if (checkbox.checked) {
      barContainer.classList.remove('hidden');
      inputContainer.style.margin = '-264px auto auto auto';
    } else {
        barContainer.classList.add('hidden');
        inputContainer.style.margin = '-40px auto auto auto';
      }
  }

function showList(checkbox) {
  const listContainer = document.getElementById('listContainer');
  const listImagesBtn = document.getElementById('listImages');
  if (checkbox.checked) {
    listContainer.classList.remove('hidden');
    listImagesBtn.click();
  } else {
    listContainer.classList.add('hidden');
  }
}

document.addEventListener('DOMContentLoaded', function (event) {
  var baseHost = document.location.origin;

  const streamContainer = document.getElementById('stream');
  const viewContainer = document.getElementById('inputImage');
  const showContainer = document.getElementById('show');
  const form = document.getElementById('infoForm');
  const stillBtn = document.getElementById('getStill');
  const streamBtn = document.getElementById('startStream');
  const stopBtn = document.getElementById('stopStream');
  const saveBtn = document.getElementById('saveImage');
  const listBtn = document.getElementById('listImages');
  const ifr = document.getElementById('ifr');
  const flash = document.getElementById('flash');
     
  var timestampTemp;  
  var streamState = false;

  streamBtn.onclick = function (event) {
    streamState = true;
    streamContainer.src = location.origin+'/?getstill='+Math.random();
  };

  streamContainer.onload = function (event) {
    if (!streamState) return;
    streamBtn.click();
  };

  function updateValue(flash) {
      let value;
      value = flash.value;
      var query = `${baseHost}/?flash=${value}`;
      fetch(query)
    }

  flash.onchange = () => updateValue(flash);
  
  stopBtn.onclick = function (event) {
    streamState=false;    
    window.stop();
  }

  listBtn.onclick = function (event) {
    ifr.src = baseHost+'?listimages';
  }      
   
  stillBtn.onclick = () => {
    stopBtn.click();
    streamContainer.src = `${baseHost}/?getstill=${Date.now()}`;
  };

  saveBtn.onclick = function (event) {
    ifr.src = baseHost + '?saveimage=' + timestampTemp;
  };

  form.addEventListener('submit', function(event) {
    event.preventDefault();

    const name = document.getElementById('name').value;
    const university = document.getElementById('university').value;
    const room = document.getElementById('room').value;
    const timestamp = document.getElementById('timestamp');
    const now = new Date();

    timestampTemp = (now.getFullYear() * 10000000000 + (now.getMonth() + 1) * 100000000 + now.getDate() * 1000000 + now.getHours() * 10000 + now.getMinutes() * 100 + now.getSeconds()).toString();    

    timestamp.value = timestampTemp;

    ifr.src = baseHost+'?getinfo=' + 'name=' + encodeURIComponent(name) + '&uni=' + encodeURIComponent(university) + '&room=' + encodeURIComponent(room) + '&timestamp=' + timestamp.value;

    setTimeout(() => {
      saveBtn.click();
    }, 3000);

    setTimeout(() => {
      window.location.reload();
    }, 10000);
  });
  
});

</script>
</body>
</html>)rawliteral";

void mainpage() {
  String Data="";

  if (cmd!="")
    Data = Feedback;
  else
    Data = String((const char *)INDEX_HTML);
  
  for (int Index = 0; Index < Data.length(); Index = Index+1024) {
    client.print(Data.substring(Index, Index+1024));
  } 
}

camera_fb_t * last_fb = NULL;

void getStill() {

  if (last_fb != NULL) {
    esp_camera_fb_return(last_fb);
  }
  last_fb = esp_camera_fb_get();
  
  if (!last_fb) {
    Serial.println("Lỗi fb");
    delay(1000);
    ESP.restart();
  }
  
  uint8_t *fbBuf = last_fb->buf;
  size_t fbLen = last_fb->len;
  
  for (size_t n = 0; n < fbLen; n += 1024) {
    if (n + 1024 < fbLen) {
      client.write(fbBuf, 1024);
      fbBuf += 1024;
    } else if (fbLen % 1024 > 0) {
      size_t remainder = fbLen % 1024;
      client.write(fbBuf, remainder);
    }
  }

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);
}

String saveImage(String filename) {    
  String response = ""; 
  String path_jpg = "/" + filename + ".jpg";
  
  if (last_fb == NULL) {
    Serial.println("fb không có sẵn");
    return "<font color=red>No captured image available</font>";
  }

  if (!SD_MMC.begin()) {
    response = "Không truy cập được thẻ SD";
    return "<font color=red>SD mount failed</font>";
  }  
  
  fs::FS &fs = SD_MMC; 
  Serial.printf("Tên ảnh: %s\n", path_jpg.c_str());

  File file = fs.open(path_jpg.c_str(), FILE_WRITE);
  
  if (!file) {
    SD_MMC.end();
    return "<font color=red>Fail to open file</font>";
  } else {
    file.write(last_fb->buf, last_fb->len);
    Serial.println("Đã lưu thành công");
  }

  file.close();
  SD_MMC.end();

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);

  return response;
}

void postImage(String filename) {
  if(!SD_MMC.begin()) {
    Serial.println("Không truy cập được thẻ SD");
    return;
  }

  String path = filename + ".jpg";

  fs::FS &fs = SD_MMC; 
  File file = fs.open("/"+path);

  if (!file) {
    Serial.println("Không thể mở file");
    return;
  }

  int fileSize = file.size();
  uint8_t* buffer = (uint8_t*)malloc(fileSize);
  if (buffer == nullptr) {
    Serial.println("Lỗi buffer");
    file.close();
    return;
  }

  file.read(buffer, fileSize);

  delay(100);

  String url = "https://firebasestorage.googleapis.com/v0/b/" + FIREBASE_STORAGE_BUCKET + "/o?name=" + path + "&uploadType=media";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "image/jpeg");

  int httpResponseCode = http.POST(buffer, fileSize);

  delay(100);

  if (httpResponseCode > 0) {
    Serial.println("Tải lên thành công");
    String response = http.getString();
    Serial.println(response);
  } else {
      Serial.println("Tải lên thất bại");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println(response);
      webSocket.sendTXT("processError");
    }

  http.end();
  free(buffer);
  file.close();
  SD_MMC.end();

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);  
}

String urlDecode(const String &str) {
  String decoded = "";
  for (int i = 0; i < str.length(); ++i) {
    if (str[i] == '%') {
      if (i + 2 < str.length()) {
        int value;
        sscanf(str.substring(i + 1, i + 3).c_str(), "%x", &value);
        decoded += (char)value;
        i += 2;
      }
    } else if (str[i] == '+') {
        decoded += ' ';
      } else {
          decoded += str[i];
        }
  }
  return decoded;
}

void sendInfo(String encodeQuery) {
  if(!SD_MMC.begin()) {
    Serial.println("Không truy cập được thẻ SD");
    return;
  }

  fs::FS &fs = SD_MMC; 
  File fileDatabase = fs.open("/countDatabase.txt");
  if (fileDatabase) {
    countDatabaseSend = fileDatabase.readStringUntil('\n');
    fileDatabase.close();
  } else {
      Serial.println("Lỗi mở file countDatabase.txt");
      fileDatabase.close();
      webSocket.sendTXT("processError");
    }
  
  SD_MMC.end();

  String decodedQuery = urlDecode(encodeQuery);

  int start = 0;
  int end = decodedQuery.indexOf('&');
  while (end >= 0) {
    String pair = decodedQuery.substring(start, end);
    int pos = pair.indexOf('=');
    if (pos >= 0) {
      String key = pair.substring(0, pos);
      String value = pair.substring(pos + 1);
      if (key == "name") {
        nameSend = value;
      } else if (key == "uni") {
          uniSend = value;
        } else if (key == "room") {
            roomSend = value;
          } else if (key == "timestamp") {
              timestampSend = value;
            }
    }
    start = end + 1;
    end = decodedQuery.indexOf('&', start);
  }

  String pair = decodedQuery.substring(start);
    int pos = pair.indexOf('=');
    if (pos >= 0) {
      String key = pair.substring(0, pos);
      String value = pair.substring(pos + 1);
    if (key == "name") {
      nameSend = value;
    } else if (key == "uni") {
        uniSend = value;
      } else if (key == "room") {
          roomSend = value;
        } else if (key == "timestamp") {
            timestampSend = value;
          }
  }

  Serial.println("Đã nhận thông tin:");
  Serial.print("Số thứ tự: ");
  Serial.println(countDatabaseSend);
  Serial.print("Họ và tên: ");
  Serial.println(nameSend);
  Serial.print("Trường: ");
  Serial.println(uniSend);
  Serial.print("Phòng: ");
  Serial.println(roomSend);
  Serial.print("Tên ảnh: ");
  Serial.println(timestampSend);

  String message = nameSend + "," + uniSend + "," + roomSend + "," + timestampSend + "," + countDatabaseSend;
  webSocket.sendTXT(message);

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);
}

String listImages() {
  if(!SD_MMC.begin()){
    Serial.println("Không truy cập được thẻ SD");
    return "<font color=red>SD mount failed</font>";
  }  
  
  fs::FS &fs = SD_MMC; 
  File root = fs.open("/");
  if(!root){
    Serial.println("Không thể mở thư viện");
    return "<font color=red>Failed to open directory</font>";
  }

  String list = "";
  String css = "<style>body{font-family: 'Roboto', sans-serif;background-color: #f4f4f4;}table{width:100%;background-color:#fff;border-collapse:collapse;text-align:center;table-layout:fixed}td{border:none}tr:hover{background-color:#afffce;font-weight:bolder}.button{background-color:#4CAF50;margin:10px auto; padding:10px;color:#fff;border:none;border-radius:5px;cursor:pointer;}.button:hover{background-color:#45a049;transform:scale(1.1);font-weight:bold;}</style>";
  File file = root.openNextFile();
  while(file){
    if(!file.isDirectory()){
      String filename=String(file.name());
      if (filename=="/"+pointer+".jpg")
        list = "<tr><td><button class=\"button\" onclick=\'location.href = location.origin+\"?deleteimage="+String(file.name())+"\";\'>Delete</button></td><td><font>"+String(file.name())+"</font></td><td><button class=\"button\" onclick=\'parent.document.getElementById(\"showContainer\").src=location.origin+\"?showimage="+String(file.name())+"\";\'>Show</button></td></tr>"+list;
       else
        list = "<tr><td><button class=\"button\" onclick=\'location.href = location.origin+\"?deleteimage="+String(file.name())+"\";\'>Delete</button></td><td><font>"+String(file.name())+"</font></td><td><button class=\"button\" onclick=\'parent.document.getElementById(\"showContainer\").src=location.origin+\"?showimage="+String(file.name())+"\";\'>Show</button></td></tr>"+list;        
    }
    file = root.openNextFile();
  }
  if (list == "") list = " ";
  list = "<table border=1>"+list+"</table>";
  list = css + list;

  file.close();
  SD_MMC.end();

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);  
  
  return list;
}

String deleteImage(String filename) {
  if(!SD_MMC.begin()){
    Serial.println("Không truy cập được thẻ SD");
    return "SD mount failed";
  }  
  
  fs::FS &fs = SD_MMC;
  File file = fs.open("/"+filename);
  String message="";
  if(fs.remove("/"+filename)){
      message = "<font color=red>" + filename + " deleted</font>";
  } else {
      message = "<font color=red>" + filename + " failed</font>";
  }
  file.close();
  SD_MMC.end();

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);  

  return message;
}

void showImage() {
  if(!SD_MMC.begin()){
    Serial.println("Không truy cập được thẻ SD");
  }  

  fs::FS &fs = SD_MMC;
  File file = fs.open("/"+pointer);
  if(!file){
    Serial.println("Không thể mở file");
    SD_MMC.end();    
  }
  else {
    byte buf[1024];
    int i = -1;
    while (file.available()) {
      i++;
      buf[i] = file.read();
      if (i==(sizeof(buf)-1)) {
        client.write((const uint8_t *)buf, sizeof(buf));
        i = -1;
      }
      else if (!file.available())
        client.write((const uint8_t *)buf, (i+1));
    }

    client.println();

    file.close();
    SD_MMC.end();    
  }

  ledcAttachChannel(4, 5000, 8, 4);
  ledcWrite(4, flashValue);
}

void getCommand(char c) {
  if (c=='?') receiveState=1;
  if ((c==' ')||(c=='\r')||(c=='\n')) receiveState=0;
  
  if (receiveState==1)
  {
    Command=Command+String(c);
    
    if (c=='=') cmdState=0;
    if (c==';') strState++;
  
    if ((cmdState==1)&&((c!='?')||(questionstate==1))) cmd=cmd+String(c);
    if ((cmdState==0)&&(strState==1)&&((c!='=')||(equalstate==1))) pointer=pointer+String(c);
    
    if (c=='?') questionstate=1;
    if (c=='=') equalstate=1;
    if ((strState>=9)&&(c==';')) semicolonstate=1;
  }
}

void executeCommand() {
  if (cmd!="getstill") {
    Serial.println("cmd = " + cmd + ", pointer = " + pointer);
  }

  if (cmd=="getinfo") { 
    sendInfo(pointer);
  } else if (cmd=="saveimage") {
      saveImage(pointer);
      delay(100);
      postImage(pointer);
    } else if (cmd=="flash") {
        ledcAttachChannel(4, 5000, 8, 4);
        flashValue = pointer.toInt();
        ledcWrite(4, flashValue);  
      } else if (cmd=="listimages") {
          Feedback=listImages();
        } else if (cmd=="deleteimage") {
            Feedback=deleteImage(pointer)+"<br>"+listImages();
          } else Feedback="Command is not defined.";
  if (Feedback=="") Feedback = Command;  
}

void listenConnection() {
  Feedback=""; Command=""; cmd=""; pointer="";
  receiveState=0, cmdState=1, strState=1, questionstate=0, equalstate=0, semicolonstate=0;
  
  client = server.available();

  if (client) { 
    String currentLine = "";

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();             
        getCommand(c);
                
        if (c == '\n') {
          if (currentLine.length() == 0) {    
            if (cmd=="getstill") {
              getStill(); 
            } else if (cmd=="showimage") {
                showImage();            
              } else mainpage(); 
            Feedback="";
            break;
          } else {
              currentLine = "";
            }
        } else if (c != '\r') {
            currentLine += c;
          }

        if ((currentLine.indexOf("/?")!=-1)&&(currentLine.indexOf(" HTTP")!=-1)) {
          if (Command.indexOf("stop")!=-1) {
            client.println();
            client.stop();
          }
          currentLine="";
          Feedback="";
          executeCommand();
        }
      }
    }
    delay(1);
    client.stop();
  }
}

void loop() {  
  webSocket.loop();
  listenConnection();
}
