#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WebSocketsServer.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP_Google_Sheet_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define SSID "NTP"
#define PASSWORD "qwert123"

#define SS_PIN 15
#define RST_PIN 2 

#define BUZZER 5

#define PROJECT_ID "acslogging"
#define CLIENT_EMAIL "acslogging@acslogging.iam.gserviceaccount.com"
#define SPREADSHEET_ID "1N-3VsKg-hy7StoVNuFZ-kVZwt19gJ9FTmrlcraYMD7Q"
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQCgYlxWOt3SgYpb\n9BB8rzZ9yoDWBax6SWr3Wni0QHrREOUq9CgHnn/+ByuPKkdv3difmOONQKRJcg7v\nIvWHw3n/hV+iaD2bhTOCf69Br7eHHpoZfCW1OUX3YN9dRg7BF+VJYiwPuLAql2bt\n3cvRvzrHSJ14tGXH+tacSDDBP1iZ/1jce4YptAU+fGBHtoy8fQnpsbkgPsIi2Pg7\nPh/Ki/cvIhYPR2bZ6o9sEdTv8mBvrbiNr/P3GPDsInCm5U/xLbZvhxZu5WqHArtv\nXpTbwzwJRaVbrDuHBgANBLg6AjYFC6og4nJqMkPLdDZTBdoGNibSvfxLPonndvJ0\n4AgyjpsxAgMBAAECggEABBBd+9rfwEvjQj950/j6Gd+h2lp4M3yNAvHGTXX1Trsd\nfXREl+XUoy6SFF17RCrESI3+lReSWiCTX4ycmVkzDA4YfWRkDD7orlm3KoRN85Tr\nTXJDf9cwzksEeefyq4KkHxMHYAobX6botyYHuju83OwafmKe0h2JZtoe7ySgZX2E\nQR21dnCehNH6yiWINwaH4bG6kt8gJYPfarenkFOPa6eK7T8qAHRaSdq8P1/nUx4+\nBeos9Ucl+5IpT6WbqKk0rQJiTRWn2HI3WWNX1hQB3G2bvE+9KtesjQTJ2JfO1JsB\n8uj+biJL7LqiiHpl5gl3MJ3Aq1vDk1g7IFdG6QYKQQKBgQDN6XLCg7sXsx2ppgyk\n8My/Sy+UbvaPxRqo0t4red0BEy/TLu2mbcinkqZX/RJMb+4bEfrA9iFPG4/IhD5o\neckVi0LiQrxLinrY9ij9m0GDqMGWrMukHj+HaifjvZ8sXKY3Ju1ZOSme88z7NJIM\nQnsSGzneutVnQpsSEvIV5ks5kQKBgQDHZc+n0antiNObToDS/p6YyBNgoCPBMSF2\nzPjCba9KF7ZvYjckqkfoKgqQc7+AKEgtmPekWzEN+yzzDC0v6wh3d4R1niYClhUu\n+l1jFNOqPi6DM3U0LR75j3Gw84ckVzLlWww6QhdWd8u55ay6BeLHx2wtRWw1m6iQ\nuqmsVy13oQKBgASPFZ5e4kaNDawS5BbGyhG2LXCA4G6gc42nVYnq4czDSvzG/jYA\nGkAzjAFIth4BIGlzBXU+PdCNkKpk2yjfAWe7tJhj191oTH3/PTsYM+QhWV/npX6V\nuNxbqlRtf1exGXEBKKIgFN7TEQfCyzUAR89H0QkwY8csf1hwGPIJLkMxAoGAYhBB\nMxCbGLYNE7llA9+zUgI7/W5khzPJeGrQAJb++Vp2H4tXAVI5cQWUEnzKdpXZAYvU\nZuFuW6jm71VSt9lIXbDK1SmFGgqDuono+byZaIWSTHY3MwTp0eRpDpSGJyo3XrML\nbW9pmN3rtK4u54HTiVsbaZxV3nuCSEN1BMwPEWECgYAej2ILoV0COWuc9bQBYQmW\nSkEM1Tuk0ktb3Pagmv5oNlUY7cStcosz9fNi2MyrMnrmhor8DqOA84yRiJvBMJDm\nSJNSwIIvi4OGv37XWbYoBDwSJPlf70rSfVvGPHpfheoc6DcmwmPXn845DpgRW9H4\nu3sNdmsZSC9sOnpZm53mEw==\n-----END PRIVATE KEY-----\n";

const String FIREBASE_STORAGE_BUCKET = "accesscontrolsystem-4f265.appspot.com";

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiServer server(80);
WiFiClient client;
WebSocketsServer webSocket = WebSocketsServer(81);
uint8_t clientID;

bool addNewUser = false;
bool readInfo = true;

unsigned long buzzerStartTime = 0;
unsigned long buzzerTime = 0;
bool buzzerActive = false;

int blockNum; 
byte blockData[16];
byte bufferLen = 18;
byte readBlockData[18];

String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};

String name;
String university;
String room;
String timestamp;
String imagesrc;

String nameReceive;
String uniReceive;
String roomReceive;
String timestampReceive;

String countDatabaseReceive;
String countDatabaseSend;

String countLogReceive;
String countLogSend;

String displayGreen = "none";
String displayGreenNotify = "none";
String displayGreenInfo = "none";
String greenTitle;
String greenContent;

String displayRed = "none";
String redTitle;
String redContent;

void writeStringToBlocks(int startBlock, String data);
String readStringFromBlocks(int startBlock, int blockCount);
void writeStringToBlock(int block, String data);
String readStringFromBlock(int block);
void writeDataToBlock(int blockNum, byte blockData[]);
void readDataFromBlock(int blockNum, byte readBlockData[]);
void writeIntToBlock(int block, int data);
int readIntFromBlock(int block);
void getCurrentTime();
void databaseInit();
void logInit();
void writeToDatabase(int row);
void writeToLog(int row);
void tokenStatusCallback(TokenInfo info);
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

void responseError() {
  addNewUser = false;
  readInfo = true;
  displayRed = "block";
  redTitle = "Lỗi xử lý dữ liệu";
  redContent = "Vui lòng nhập lại thông tin và chụp lại ảnh";
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  String message = String((char *)data).substring(0, len);
  if (message.startsWith("newCountLog:")) {
    countLogReceive = message.substring(strlen("newCountLog:"));
    Serial.println("Đã nhận thông tin:");
    Serial.print("newCountLog: ");
    Serial.println(countLogReceive);
  } else if (message.startsWith("processError")) {
      responseError();
    } else {
        Serial.println("Đã nhận nội dung: " + message);

        int index1 = message.indexOf(',');
        int index2 = message.indexOf(',', index1 + 1);
        int index3 = message.indexOf(',', index2 + 1);
        int index4 = message.indexOf(',', index3 + 1);

        nameReceive = message.substring(0, index1);
        uniReceive = message.substring(index1 + 1, index2);
        roomReceive = message.substring(index2 + 1, index3);
        timestampReceive = message.substring(index3 + 1, index4);
        countDatabaseReceive = message.substring(index4 + 1);

        displayGreen = "block";
        greenTitle = "Đã nhận thông tin";
        displayGreenInfo = "block";
        displayGreenNotify = "none";

        Serial.println("Thông tin sau phân tích:");
        Serial.print("Họ và tên: ");
        Serial.println(nameReceive);
        Serial.print("Trường: ");
        Serial.println(uniReceive);
        Serial.print("Phòng: ");
        Serial.println(roomReceive);
        Serial.print("Tên ảnh: ");
        Serial.print(timestampReceive);
        Serial.println(".jpg");
        Serial.print("countDatabase: ");
        Serial.println(countDatabaseReceive);

        addNewUser = true;
        readInfo = false;

        Serial.println("Sẵn sàng thêm người dùng mới");
      }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);

  WiFi.begin(SSID, PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Đã kết nối WiFi");
  Serial.println(WiFi.localIP());
  server.begin();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  timeClient.begin();
  timeClient.setTimeOffset(7*3600);

  GSheet.setTokenCallback(tokenStatusCallback);
  GSheet.setPrerefreshSeconds(10 * 60);
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

  databaseInit();
  logInit();

  SPI.begin();            
  mfrc522.PCD_Init();     
  delay(4);               
  mfrc522.PCD_DumpVersionToSerial();  

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void updatepage(bool isAjaxRequest) {
  String updateHTML;
  if (isAjaxRequest) {
    updateHTML = 
      "{\"imagesrc\":\"" + imagesrc + "\", \"name\":\"" + name + "\", \"university\":\"" + university + "\", \"room\":\"" + room + "\","
      "\"greenTitle\":\"" + greenTitle + "\", \"greenContent\":\"" + greenContent + "\", \"redTitle\":\"" + redTitle + "\", \"redContent\":\"" + redContent + "\","
      "\"nameReceive\":\"" + nameReceive + "\", \"uniReceive\":\"" + uniReceive + "\", \"roomReceive\":\"" + roomReceive + "\", \"timestampReceive\":\"" + timestampReceive + "\","
      "\"displayRed\":\"" + displayRed + "\", \"displayGreen\":\"" + displayGreen + "\", \"displayGreenNotify\":\"" + displayGreenNotify + "\", \"displayGreenInfo\":\"" + displayGreenInfo + "\"}";
  } else {
    String head = 
      "<title>ACS Info Page</title>"
      "<meta charset=\"UTF-8\">"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
      "<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\">"
      "<link href=\"https://fonts.googleapis.com/css2?family=Roboto&display=swap\" rel=\"stylesheet\">";
    
    String css = 
      "<style>"
        "body {font-family: 'Montserrat', sans-serif; background-color: #f4f4f4;}"
        ".notify {max-width: 400px; padding: 20px; margin: 20px auto; border-radius: 10px; box-shadow: 0px 0px 20px rgba(0,0,0,0.2); background-color: #fff; transition: all 0.3s ease; box-shadow: 0px 0px 20px rgba(0,0,0,0.2);}"
        ".notifyMainRed {color: #FF6060;}"
        ".notifyMainGreen {color: #4CAF50;}"
        ".notifySub {font-weight: normal;}"
        ".info {display: grid; grid-template-columns: 1fr 2fr; grid-template-rows: auto 1fr; width: 940px; height: 340px; padding: 20px; margin: 10px auto; box-shadow: 0px 0px 20px rgba(0,0,0,0.2); border-radius: 5px; background-color: #fff;}"
        ".info h2 {text-align: center; border-radius: 5px; padding: 10px; background-color: #4CAF50; color: #fff;}"
        ".info p {margin: 0 0 10px; line-height: 1.6; margin-left: 70px; font-size: 30px; text-align: left; font-weight: bolder;}"
        ".info label {display: block; margin-left: 20px; margin-bottom: 5px; font-size: 20px; text-align: left;}"
        ".infoTitle {grid-column-start: 1; grid-column-end: 3;}"
        ".infoImage { width: 320px; height: 240px; background-color: #ccc;}"
        ".infoText {width: 600px; height: 320px;}"
        ".resetBtnContainer {margin: auto auto; transform: translateX(440px); width: fit-content; text-align: center;}"
        ".options {display: none;}"
        ".resetBtn {width: 80px; background-color: #ff8484; margin: 10px auto; padding: 10px; color: #fff; border: none; border-radius: 5px; cursor: pointer;}"
        ".resetBtn:hover {background-color: #ff4141; transform: scale(1.1); font-weight: bold;}"
        ".agreeBtn {width: 76px; background-color: #fff; margin: 10px auto; padding: 8px; color: #FF6060; border: 2px solid #FF6060; border-radius: 5px; cursor: pointer;}"
        ".agreeBtn:hover {background-color: #FF6060; color: #fff; transform: scale(1.1); font-weight: bold;}"
        ".cancelBtn {width: 76px; background-color: #fff; margin: 10px auto; margin-right: 10px; padding: 8px; color: #45a049; border: 2px solid #45a049; border-radius: 5px; cursor: pointer;}"
        ".cancelBtn:hover {background-color: #45a049; color: #fff; transform: scale(1.1); font-weight: bold;}"
      "</style>";

    String body = 
      "<div class=\"notify\" id=\"notifyGeneral\" style=\"display:" + displayRed + ";\">"
        "<h2 class=\"notifyMainRed\" id=\"redTitle\">" + redTitle + "</h2>"
        "<h3 class=\"notifySub\" id=\"redNotify\">" + redContent + "</h3>"
      "</div>"
      "<div class=\"notify\" id=\"notifyInfo\" style=\"display:" + displayGreen + ";\">"
        "<h2 class=\"notifyMainGreen\" id=\"greenTitle\">" + greenTitle + "</h2>"
          "<h3 class=\"notifySub\" id=\"greenNotify\" style=\"display:" + displayGreenNotify + ";\">" + greenContent + "</h3>"
          "<h3 class=\"notifySub\" id=\"greenInfo\" style=\"display:" + displayGreenInfo + ";\">" + nameReceive + "<br>" + uniReceive + "<br>" + roomReceive + "<br>" + timestampReceive + ".jpg</h3>"
      "</div>"
      "<div class=\"info\" id=\"infoContainer\">"
        "<div class=\"infoTitle\">"
          "<h2>Thông Tin Cá Nhân</h2>"
        "</div>"
        "<div class=\"infoImage\">"
          "<img id=\"infoImage\" src=\"\"/>"
        "</div>"
        "<div class=\"infoText\">"
          "<label>Họ và tên:</label>"
            "<p id=\"nameDisplay\">" + name + "</p>"
          "<label>Trường:</label>"
            "<p id=\"universityDisplay\">" + university + "</p>"
          "<label>Phòng:</label>"
            "<p id=\"roomDisplay\">" + room + "</p>"
        "</div>"
      "</div>"
      "<div class=\"resetBtnContainer\">"
        "<button class=\"resetBtn\" id=\"resetBtn\">Reset</button>"
        "<div class=\"options\" id=\"optionContainer\">"
            "<button class=\"cancelBtn\" id=\"cancelBtn\">Hủy bỏ</button>"
            "<button class=\"agreeBtn\" id=\"agreeBtn\">Đồng ý</button>"
        "</div>"
      "</div>";
    
    String jvscript = 
      "<script>"

        "function updateDisplayGreen() {"
          "var xhttp = new XMLHttpRequest();"
          "xhttp.open('GET', '/hideGreen', true);"
          "xhttp.send();"
        "}"

        "function updateDisplayRed() {"
          "var xhttp = new XMLHttpRequest();"
          "xhttp.open('GET', '/hideRed', true);"
          "xhttp.send();"
        "}"

        "setInterval(function() {"
          "var xhttp = new XMLHttpRequest();"
          "xhttp.onreadystatechange = function() {" 
            "if (this.readyState == 4 && this.status == 200) {" 
              "var data = JSON.parse(this.responseText);" 
              "document.getElementById('infoImage').src = data.imagesrc;" 
              "document.getElementById('nameDisplay').innerText = data.name;" 
              "document.getElementById('universityDisplay').innerText = data.university;" 
              "document.getElementById('roomDisplay').innerText = data.room;"
              "document.getElementById('notifyGeneral').style.display = data.displayRed;"
              "document.getElementById('redTitle').innerText = data.redTitle;"
              "document.getElementById('redNotify').innerText = data.redContent;"
              "document.getElementById('notifyInfo').style.display = data.displayGreen;"
              "document.getElementById('greenTitle').innerText = data.greenTitle;"
              "document.getElementById('greenNotify').style.display = data.displayGreenNotify;"
              "document.getElementById('greenInfo').style.display = data.displayGreenInfo;"
              "document.getElementById('greenNotify').innerText = data.greenContent;"
              "document.getElementById('greenInfo').innerHTML = data.nameReceive + '<br>' + data.uniReceive + '<br>' + data.roomReceive + '<br>' + data.timestampReceive + '.jpg';"
            "}"
          "};" 
          "xhttp.open('GET', '/', true);" 
          "xhttp.setRequestHeader('X-Requested-With', 'XMLHttpRequest');" 
          "xhttp.send();" 
          "if (document.getElementById('notifyInfo').style.display === 'block') {"
            "setTimeout(function() {"
              "updateDisplayGreen();"
              "document.getElementById('notifyInfo').style.display = 'none';"
            "}, 5000);"
          "}"
          "if (document.getElementById('notifyGeneral').style.display === 'block') {"
            "setTimeout(function() {"
              "updateDisplayRed();"
              "document.getElementById('notifyGeneral').style.display = 'none';"
            "}, 3000);"
          "}"
        "}, 500);"

        "const resetBtn = document.getElementById('resetBtn');"
        "const optionContainer = document.getElementById('optionContainer');"
        "const agreeBtn = document.getElementById('agreeBtn');"
        "const cancelBtn = document.getElementById('cancelBtn');"

        "resetBtn.addEventListener('click', () => {"
          "optionContainer.style.display = 'block';"
        "});"

        "cancelBtn.addEventListener('click', () => {"
          "optionContainer.style.display = 'none';"
        "});"

        "agreeBtn.addEventListener('click', () => {"
          "optionContainer.style.display = 'none';"
          "var xhttp = new XMLHttpRequest();"
          "xhttp.open('GET', '/reset', true);"
          "xhttp.send();"
        "});"

      "</script>";

    updateHTML = head + css + body + jvscript;
  }

  for (int Index = 0; Index < updateHTML.length(); Index += 1024) {
    client.print(updateHTML.substring(Index, Index+1024));
  } 
}

void listenConnection() {
  client = server.available();

  if (client) { 
    String currentLine = "";
    bool isAjaxRequest = false;

    while (client.connected()) {
      if (client.available()) {
        char c = client.read();             
        currentLine += c;

        if (currentLine.endsWith("\r\n\r\n")) {
          if (currentLine.indexOf("X-Requested-With: XMLHttpRequest") != -1) {
            isAjaxRequest = true;
          }

          if (currentLine.startsWith("GET /hideGreen")) {
            displayGreen = "none";
            break;
          }

          if (currentLine.startsWith("GET /hideRed")) {
            displayRed = "none";
            break;
          }

          if (currentLine.startsWith("GET /reset")) {
            addNewUser = false;
            readInfo = true;

            displayGreen = "block";
            displayGreenNotify = "block";
            displayGreenInfo = "none";
            greenTitle = "Đã đặt lại dữ liệu";
            greenContent = "Trở về chế độ đọc";

            nameReceive = "";
            uniReceive = "";
            roomReceive = "";
            timestampReceive = "";
            
            Serial.println("Đã đặt lại thông tin");
            Serial.println("Đã trở về chế độ đọc");

            break;
          }

          updatepage(isAjaxRequest);
          break;
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

  if (buzzerActive && (millis() - buzzerStartTime >= buzzerTime)) {
    digitalWrite(BUZZER, LOW);
    buzzerActive = false;
  }

  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  if (readInfo) {
    name = readStringFromBlocks(4, 3);
    university = readStringFromBlocks(8, 3);
    room = readStringFromBlocks(12, 3);
    timestamp = readStringFromBlocks(16, 3);
    imagesrc = "https://firebasestorage.googleapis.com/v0/b/"+ FIREBASE_STORAGE_BUCKET + "/o/" + timestamp + ".jpg?alt=media";

    int countLogTemp = countLogReceive.toInt();
    writeToLog(countLogTemp);
    countLogTemp += 1;
    countLogSend = "newCountLog:" + String(countLogTemp);
    webSocket.sendTXT(clientID, countLogSend);

    if ((name!="")&&(university!="")&&(room!="")&&(timestamp!="")) {
      Serial.println("Đã đọc dữ liệu từ thẻ: ");
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
      }
      Serial.println();
      Serial.print("Họ và tên: ");
      Serial.println(name);
      Serial.print("Trường: ");
      Serial.println(university);
      Serial.print("Phòng: ");
      Serial.println(room);
      Serial.print("Tên ảnh: ");
      Serial.print(timestamp);
      Serial.println(".jpg");
      
      buzzerStartTime = millis();
      buzzerTime = 100;
      buzzerActive = true;
      digitalWrite(BUZZER, HIGH);
    } else {
        displayRed = "block";
        redTitle = "Từ chối truy cập";
        redContent = "Thông tin thẻ không hợp lệ";

        buzzerStartTime = millis();
        buzzerTime = 3000;
        buzzerActive = true;
        digitalWrite(BUZZER, HIGH);
      }
  }
  
  if (addNewUser) {
    writeStringToBlocks(4, nameReceive);
    writeStringToBlocks(8, uniReceive);
    writeStringToBlocks(12, roomReceive);
    writeStringToBlocks(16, timestampReceive);

    displayGreen = "block";
    displayGreenNotify = "block";
    displayGreenInfo = "none";
    greenTitle = "Đã thêm người mới";
    greenContent = "Thành công";

    int countDatabaseTemp = countDatabaseReceive.toInt();
    countDatabaseTemp += 1;
    writeToDatabase(countDatabaseTemp);
    countDatabaseSend = "newCountDatabase:" + String(countDatabaseTemp);
    webSocket.sendTXT(clientID, countDatabaseSend);

    Serial.println("Đã ghi dữ liệu vào thẻ: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();
    Serial.print("Họ và tên: ");
    Serial.println(nameReceive);
    Serial.print("Trường: ");
    Serial.println(uniReceive);
    Serial.print("Phòng: ");
    Serial.println(roomReceive);
    Serial.print("Tên ảnh: ");
    Serial.print(timestampReceive);
    Serial.println(".jpg");
    Serial.print("Đã cập nhật số thứ tự: ");
    Serial.println(countDatabaseSend);

    addNewUser = false;
    readInfo = true;
    
    buzzerStartTime = millis();
    buzzerTime = 100;
    buzzerActive = true;
    digitalWrite(BUZZER, HIGH);
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(100);
}


void writeStringToBlocks(int startBlock, String data) {
  int len = data.length();
  byte buffer[16];
  for (int i = 0; i < 3; i++) {
    int offset = i * 16;
    for (int j = 0; j < 16; j++) {
      if (offset + j < len) {
        buffer[j] = data[offset + j];
      } else {
          buffer[j] = 0;
        }
    }
    writeDataToBlock(startBlock + i, buffer);
  }
}

String readStringFromBlocks(int startBlock, int blockCount) {
  String result = "";
  byte buffer[18];
  for (int i = 0; i < blockCount; i++) {
    readDataFromBlock(startBlock + i, buffer);
    for (int j = 0; j < 16; j++) {
      if (buffer[j] != 0) {
        result += (char)buffer[j];
      }
    }
  }
  return result;
}

void writeDataToBlock(int blockNum, byte blockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    return;
  }
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK) {
    return;
  }
}

void readDataFromBlock(int blockNum, byte readBlockData[]) {
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    return;
  }
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    return;
  }
}

void databaseInit() {
  bool ready = GSheet.ready();
  if (ready) {
    FirebaseJson response;
    FirebaseJson databaseHeaders;

    databaseHeaders.set("values/[0]/[0]", "STT");
    databaseHeaders.set("values/[0]/[1]", "Họ và tên");
    databaseHeaders.set("values/[0]/[2]", "Trường");
    databaseHeaders.set("values/[0]/[3]", "Phòng");

    GSheet.values.update(&response, SPREADSHEET_ID, "Database!A1:D1", &databaseHeaders);
    response.toString(Serial, true);
  }
}

void writeToDatabase(int row) {
  bool ready = GSheet.ready();
  if (ready) {
    FirebaseJson response;
    FirebaseJson databaseData;

    databaseData.set("values/[0]/[0]", String(row-1));
    databaseData.set("values/[0]/[1]", nameReceive);
    databaseData.set("values/[0]/[2]", uniReceive);
    databaseData.set("values/[0]/[3]", roomReceive);

    String range = "Database!A" + String(row) + ":D" + String(row);
    GSheet.values.update(&response, SPREADSHEET_ID, range, &databaseData);
    
    response.toString(Serial, true);
  }
}

void logInit() {
  bool ready = GSheet.ready();
  if (ready) {
    FirebaseJson response;
    FirebaseJson logHeaders;

    logHeaders.set("values/[0]/[0]", "Ngày");
    logHeaders.set("values/[0]/[1]", "Giờ");
    logHeaders.set("values/[0]/[2]", "Họ và tên");
    logHeaders.set("values/[0]/[3]", "Trường");
    logHeaders.set("values/[0]/[4]", "Phòng");

    GSheet.values.update(&response, SPREADSHEET_ID, "Log!A1:E1", &logHeaders);
    
    response.toString(Serial, true);
  }
}

void writeToLog(int row) {
  bool ready = GSheet.ready();
  if (ready) {
    timeClient.update();

    time_t epochTime = timeClient.getEpochTime();
    String formattedTime = timeClient.getFormattedTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime); 
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon+1;
    String currentMonthName = months[currentMonth-1];
    int currentYear = ptm->tm_year+1900;
    String currentDate = currentMonthName + " " + String(monthDay) + ", " + String(currentYear);

    FirebaseJson response;
    FirebaseJson logData;

    logData.set("values/[0]/[0]", currentDate);
    logData.set("values/[0]/[1]", formattedTime);
    logData.set("values/[0]/[2]", name);
    logData.set("values/[0]/[3]", university);
    logData.set("values/[0]/[4]", room);

    String range = "Log!A" + String(row) + ":E" + String(row);
    GSheet.values.update(&response, SPREADSHEET_ID, range, &logData);

    response.toString(Serial, true);
  }
}

void tokenStatusCallback(TokenInfo info) {
  if (info.status == esp_signer_token_status_error) {
    Serial.printf("Thông tin token: loại = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    Serial.printf("Lỗi token: %s\n", GSheet.getTokenError(info).c_str());
  } else {
      Serial.printf("Thông tin token: loại = %s, trạng thái = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_TEXT:
      handleWebSocketMessage(NULL, payload, length);
      break;
    case WStype_DISCONNECTED:
      Serial.printf("[%u] ngắt kết nối\n", num);
      break;
    case WStype_CONNECTED: {
      clientID = num;
      IPAddress ip = webSocket.remoteIP(clientID);
      Serial.printf("[%u] kết nối từ %s\n", clientID, ip.toString().c_str());
      break;
    }
  }
}
