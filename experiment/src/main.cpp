#include <Arduino.h>

#define G433_SPEED 1000   //О6ЯЗАТЕЛЬНО!!! перед подключением 6и6лиотек

#include <Gyver433.h>
#include <GyverHub.h>

///////////////////======================================
#define led 2               //индикатор входящего пакета по радио
uint8_t device;             //какой датчик прислал данные
uint16 incomin;             //Входящие данные по радио
int temperatureC;           //вычисленная температура
uint16 count_t = 601;
int16_t countdown = 5;         //таймер ожидания от радио температуры
uint32_t time_sist;         //время системы
uint32_t time_radio;        //время прихода радио
uint32_t ltime;
uint32_t  t_on;
uint32_t  t_off;
uint8_t sw_stat;
///================================//

GyverHub hub("MyDev", "Сво6одный", "f0ad");  // имя сети, имя устройства, иконка
WiFiClient espClient;
Gyver433_RX<0, 5> rx;       //6ля!!!0- ЭТО D3 пин esp!!! 5 6айт- 6уфер


///   WI-FI  ///////////
const char* ssid = "RT-WiFi-6359";
const char* password = "EJirUCda4A";

//   MQTT  /////////////
const char* mqtt_server = "m4.wqtt.ru";
const int mqtt_port = 9478;
const char* mqtt_user = "u_5A3C2X";
const char* mqtt_password = "HilZPRjD";

////////////////////////////////////////
void ICACHE_RAM_ATTR isr();  //зарание оъявляем, не то вечный ре6ут

void onunix(uint32_t stamp) {                //получаем дату время. ра6отает!!!
    time_sist = (stamp + 32400) % 86400;    //получаем только время и корректируем часовой пояс
}

void isr() {                      // тикер радио вызывается в прерывании
  rx.tickISR();
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void radio(){                     // Принимаем радио!!!
             
    digitalWrite(led, LOW);     //зажигаем индикатор
    ltime = millis();
    device = rx.buffer[0];
    
    switch (device){
      case 0:
      Serial.println("okokok");
      break;

      case 1:                                         // 1- переносной первый датчик
      time_radio = time_sist;
      countdown = 900;                                //счетчик на 15 мин
      count_t++;
      Serial.print("termo: ");
      Serial.println(rx.buffer[1] << 8 | rx.buffer[2]);
      
      incomin = (rx.buffer[1] << 8 | rx.buffer[2]);
      
      temperatureC = (incomin * (3.2 / 1024) - 0.5) * 100 ; //исходя из 10 мВ на градус со смещением 500 мВ
      Serial.print(temperatureC); Serial.println("  C");

      hub.sendUpdate("dispTemp");
      hub.sendUpdate("dispCount");
      hub.sendUpdate(F("t_in"));
      break;

      case 2:
      hub.sendUpdate(F("led"),rx.buffer[1]);
      Serial.print("кнопка: ");
      Serial.println(rx.buffer[1]);
      break;

      default:
      Serial.println("error");
    }

}

void build(gh::Builder& b) {      // билдер  ///////////////
  b.LED_(F("led"));               //сра6отает от радио усстройства 2
  if (b.beginRow()) {
    b.Display_("dispTemp", temperatureC).label("мо6ильный датчик").color(gh::Colors::Aqua); 
    b.Display_("dispCount", count_t).label("счетчик ").color(gh::Colors::Blue);
    b.Time_(F("t_in"), &time_radio).label(F("время")).color(gh::Colors::Yellow);
    b.endRow();  // завершить
  }
  if(b.beginRow()){
    b.Time_(F("time"), &time_sist).label(F("время")).color(gh::Colors::Mint);
    b.Time_(F("t_on"), &t_on).label(F("включить")).color(gh::Colors::Red).click();
    b.Time_(F("t_off"), &t_off).label(F("выкл")).color(gh::Colors::Green);
    b.endRow();
  }
  b.Switch_(F("Swit"), &sw_stat).label(F("включатель"));
}

void setup() {
    Serial.begin(74880);
    Serial.println("ПОЕХАЛИ 222!!!");
    
    pinMode(led, OUTPUT);
    setup_wifi();

    hub.mqtt.config(mqtt_server, mqtt_port, mqtt_user, mqtt_password);

    attachInterrupt(0, isr, CHANGE);  // взводим прерывания по CHANGE
    hub.setVersion("Srvrn1/experiment@1.1");
    hub.onUnix(onunix);
    hub.onBuild(build);               // подключаем билдер
    hub.begin();                      // запускаем систему
    
}

void loop() {

  hub.tick();                                               // тикаем тут
  
  if (rx.gotData()) {                                       //если пришло радио
    radio();
  }

  if(millis() - ltime > 100) digitalWrite(led, HIGH);       //тушим лампочку радио

  static GH::Timer tmr(1000);
  if(tmr){
    countdown--;
    if(countdown < 0){                                     //если долго нет сигнала от датчика №1
      countdown = 900;                                     //15 min
      temperatureC = 888;
      hub.sendStatus(F("dispTemp"));                       //выводим 888 - код оши6ки
    }
    time_sist++;
    if(time_sist >= 86400) time_sist = 0;                   //время в Unix формате с6расываем в 00 часов
    hub.sendUpdate(F("time"));

    if(time_sist == t_on){                                 //6удильник включаем Switch
      sw_stat = 1;
      hub.sendUpdate(F("Swit"));
    }
    if(time_sist == t_off){
      sw_stat = 0;
      hub.sendUpdate(F("Swit"));
    }
  }

}