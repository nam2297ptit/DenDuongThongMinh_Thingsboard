#include <ESP8266WiFi.h> // thư vien dung cho esp8266
#include <PubSubClient.h> // thu vien mqtt
#include <ArduinoJson.h>  // thu vien Arduin Json v5
#include "DHT.h" // thư vien DHT sensors


// Thông tin về wifi
#define ssid "lophocvui.com 2.4GHz"
#define password "88889999"

// Thông tin về MQTT Broker
#define mqtt_server "demo.thingsboard.io"
#define mqtt_port  1883
#define mqtt_user "Gateway 1" // user mqtt 
#define mqtt_pwd "" //password mqtt

#define mqtt_topic_sensor "v1/devices/me/telemetry" //topic nhan du lieu cam bien mac dinh cua ThingBoard
#define mqtt_topic_led_response "v1/devices/me/attributes" //topic nhan du lieu bat tat den mac dinh cua ThingBoard
#define mqtt_topic_led_request "v1/devices/me/rpc/request/+" //topic nhan du lieu bat tat den mac dinh cua ThingBoard
#define DeviceID  "LHV-01" // id of station x

WiFiClient espClient;
PubSubClient client(espClient);

long now = millis();
long lastMeasure = 0;

/*---DHT11---*/
#define DHTPIN D3
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
/*---Quang Tro---*/
int quangtro = A0;
/*------LED------*/
#define LED1 D1
#define LED2 D2
#define BT1  D6
#define BT2  D5
/*----BUTTON-----*/
int buttonStatus1 = 0;
int buttonStatus2 = 0;
int oldButton = 0;
int ledStatus = 0;
/*----MOTION-----*/
int inputPin = D7;   //dùng chân D7 để kết nối tới cảm biến
bool pinStatus = LOW; // biến để so sánh với trạng thái trước
bool pirState = LOW; //LOW = không có chuyển động, HIGH = có chuyển động

/*----SetUp-----*/
void setup(){
  Serial.begin(9600);
  pinMode(LED1,OUTPUT);
  pinMode(LED2,OUTPUT);
  
  pinMode(BT1,INPUT_PULLUP);
  pinMode(BT2,INPUT_PULLUP);
   
  digitalWrite(LED1,LOW);
  digitalWrite(LED2,LOW);
  
  pinMode(inputPin, INPUT);
  
  setup_wifi();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback); 
  dht.begin();
}
/*----Loop-----*/
void loop(){
  // Kết nối wifi 
  if (!client.connected()){
    reconnect();
  }
  client.loop();
  
  // Khi nhấn nút sẽ bật đèn, đồng thời publish giá trị trạng thái của đèn lên server ThingsBoard
  
  StaticJsonBuffer<200> jsonBuffer;//memory allocated on the stack and has a fixed size
  JsonObject& data = jsonBuffer.createObject();//Allocates an empty JsonObject
  char mesage[256];// mang ky tu
  
  buttonStatus1 = digitalRead(BT1);
  if(buttonStatus1 != oldButton){
    oldButton = buttonStatus1;
    delay(50);
    if(buttonStatus1 == 1){ 
      ledStatus = !ledStatus;  //đảo trạng thái led
      if(ledStatus == 1){ 
        Ctl_Led(LED1,1);
        data["R1"] = true;
        data.printTo(mesage, sizeof(mesage));
        client.publish(mqtt_topic_led_response,mesage);
      }
      else{ 
        Ctl_Led(LED1,0);
        data["R1"] = false;
        data.printTo(mesage, sizeof(mesage));
        client.publish(mqtt_topic_led_response,mesage);
      }
    }
  }

  buttonStatus2 = digitalRead(BT2);
  if(buttonStatus2 != oldButton){
    oldButton = buttonStatus2;
    delay(50);
    if(buttonStatus2 == 1){ 
      ledStatus = !ledStatus;  //đảo trạng thái led
      if(ledStatus == 1){ 
        Ctl_Led(LED2,1);
        data["R2"] = true;
        data.printTo(mesage, sizeof(mesage));
        client.publish(mqtt_topic_led_response,mesage);
      }
      else{ 
        Ctl_Led(LED2,0);
        data["R2"] = false;
        data.printTo(mesage, sizeof(mesage));
        client.publish(mqtt_topic_led_response,mesage);
      }
    }
  }

  // Emotions
  pinStatus = digitalRead(inputPin);
  if (pinStatus == HIGH) {
    if (pirState == LOW) {
      Serial.println("Phát hiện chuyển động");
      digitalWrite(LED2, LOW);
      pirState = HIGH;
    }
  }
  else
  {
    if (pirState == HIGH) {
      Serial.println("Không có chuyển động");
      digitalWrite(LED2, HIGH);
      pirState = LOW;
    }
  }

  now = millis();
  // cu 2s gui du lieu cảm biến 1 lần
  if (now - lastMeasure > 2000){
    lastMeasure = now;
    Update_data();
   }
}
/**
  * @brief:  fucntion callback of mqtt 
  * @param  topic: topic mqtt client subcrice to
  *         mesage: message
  *         length: len of message
  * @retval None
**/
void callback(String topic, byte* message, unsigned int length) {
    // Hien thi len Serial topic va message nhan duoc
    Serial.print("Message arrived on topic: ");
    String Topic = topic;
    Serial.print(Topic);
    Serial.print(". Message: ");
    String messageTemp;
    Serial.println(messageTemp);
    // gop message nhan duoc tu kieu byte sang string
    for(int i = 0; i < length; i++){
        messageTemp += (char)message[i];
    }
    Serial.println(messageTemp);

    // convert message tu kieu String sang Objetct
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject  &data = jsonBuffer.parseObject(messageTemp);
    // Kiem tra qua trinh convert co loi hay khong
    if (!data.success()){
    Serial.println("parseObject() failed");
    return;
    }

    // Du lieu nhan duoc khi nhan nut tren giao dien ThingsBoard co dang {“method”:”x”,”params”:y} nen can so sanh method va params
    String methodName = String((const char*)data["method"]); //convert lai string de so sanh method
/****************Den 1************************/
    if(methodName.equals("R1")){
      if(data["params"]==true){
          Ctl_Led(LED1,1);
          }
      else{
          Ctl_Led(LED1,0);
          }
    }
/****************Den 2************************/
    if(methodName.equals("R2")){
      if(data["params"]==true){
          Ctl_Led(LED2,1);
          }
      else{
          Ctl_Led(LED2,0);
          }
    }
/*********************************************/
}
/**
  * @brief:  Control LEDx 
  * @param:  led is led to cotrol 
  *              where is LED1,LED2
  * @param:  Status:
  *           1: LED ON
  *           0: LED OFF
  * @retval None
**/
void Ctl_Led(int led,int Status){
  digitalWrite(led,Status == 1 ? HIGH : LOW);
}
/**
  * @brief:  Setup wifi for ESP and print IP address.
  * @param: None
  * @retval None
**/
void setup_wifi(){
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
/**
  * @brief:  Connect to MQTT broker with ID, user and pass
  *          and pushlish init value of LEDs
  * @param: None
  * @retval None
**/
void reconnect(){
  // Chờ tới khi kết nối
  while (!client.connected()){
    Serial.print("Attempting MQTT connection...");// Thực hiện kết nối với mqtt user và pass
    if (client.connect(DeviceID,mqtt_user, mqtt_pwd)) {
      Serial.println("connected");// Khi kết nối sẽ publish thông báo
      delay(50);
      // subcrice đến topic của mỗi đèn để nhận tín hiệu điều khiển
      client.subscribe(mqtt_topic_led_request);
    }
    else{
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);// Đợi 5s
    }
  }
}
/**
  * @brief:  Read data from sensors. Temperature,Humidity and light
  * after convert to JSON and publish.
  * @param: None
  * @retval None
**/
void Update_data(){
    StaticJsonBuffer<200> jsonBuffer;//memory allocated on the stack and has a fixed size
    JsonObject& data = jsonBuffer.createObject();//Allocates an empty JsonObject
    char mesage[256];// mang ky tu
    // cảm biến ánh sáng
    int l = analogRead(quangtro);
    if(l > 1000){
      Ctl_Led(LED1,1); 
      data["R1"] = true;
      data.printTo(mesage, sizeof(mesage));
      client.publish(mqtt_topic_led_response,mesage);
      Serial.println("Đèn sáng");
     }
     else if(l < 300 ){
      Ctl_Led(LED1,0); 
      data["R1"] = false;
      data.printTo(mesage, sizeof(mesage));
      client.publish(mqtt_topic_led_response,mesage);
      Serial.println("Đèn tắt");
     }
    
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    
    if (isnan(l)){ l = 0;}
    if (isnan(h)){ h = 0; } 
    if (isnan(t)){ t = 0; }
    // gan du lieu vao chuoi Json
    data["ID"] = DeviceID;
    data["Temperature"] = t ;
    data["Humidity"] = h;
    data["Light"] = l;
    
    data.printTo(mesage, sizeof(mesage));// lưu thanh chuoi JSON
    // gui du lieu len ThingsBoard
    Serial.print("Format data: ");
    Serial.println(mesage);
    client.publish(mqtt_topic_sensor,mesage);
}
