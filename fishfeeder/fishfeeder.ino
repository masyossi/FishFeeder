#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include <Servo.h>
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include "time.h"


// Replace with your network credentials
//const char* ssid = "Redmi Note 8 Pro";
//const char* password = "00000000";
const char* ssid = "GodBless";
const char* password = "keepsmile";

// Initialize Telegram BOT
#define BOTtoken "1349854451:AAGXfih7EcOOadJ_BFASHZZNSHYWEWk4X7k"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "675585822"

#ifdef ESP8266
  X509List cert(TELEGRAM_CERTIFICATE_ROOT);
#endif

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

bool ledState = LOW;

static const int servoPin = 13;
static const int pinLed = 5;

Servo servo1;
int pos = 0;
int second = 0;
int times = 180;
unsigned long previousMillis = 0;
unsigned long interval = 30000;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 25200;
const int   daylightOffset_sec = 0;
String dayOfWeek[7] = {"Minggu","Senin","Selasa","Rabu","Kamis","Jumat","Sabtu"};
String month[12] = {"Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"};
String datetime = "";
int count = 0;

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  int current_hour = timeinfo.tm_hour;
  int current_min = timeinfo.tm_min;
  int current_sec = timeinfo.tm_min;
  int current_day = timeinfo.tm_wday;
  int current_month = timeinfo.tm_mon;
  int current_days = timeinfo.tm_mday;
  int current_year = 1900 + (timeinfo.tm_year);

  datetime = dayOfWeek[current_day] + ", "+ current_days + " " + month[current_month] + " " +current_year + " " + current_hour + ":" + current_min + ":" + current_sec;
  Serial.println(datetime);
//  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  
}

// Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Selamat datang, " + from_name + ".\n";
      welcome += "Gunakan perintah dibawah ini untuk mendapatkan informasi.\n\n";
      welcome += "/makan -> perintah untuk kasih makan ikan \n";
      welcome += "/info -> perintah untuk mengetahui berapa kali ikan makan \n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/makan") {
      bot.sendMessage(chat_id, "kasih makan selesai", "");
      kasihmakan();
    }

    if (text == "/info") {
      bot.sendMessage(chat_id, "Sejak tgl "+ datetime + " sudah dikasih makan sebanyak "+ (count) +" kali", "");
    }
  }
}

void setup() {
    Serial.begin(115200);
    pinMode(pinLed, OUTPUT);
    digitalWrite(pinLed, LOW);
    servo1.attach(servoPin);

    // Connect to Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    #ifdef ESP32
      client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
    #endif
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }
    
    digitalWrite(pinLed, HIGH); //matikan led jika wifi sudah terhubung
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    // Print ESP32 Local IP Address
    Serial.println(WiFi.localIP());

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    printLocalTime();

    xTaskCreatePinnedToCore(
    timerTask
    ,  "Timer Task"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL
    ,  ARDUINO_RUNNING_CORE);
    Serial.println("Task 1 Created");

    xTaskCreatePinnedToCore(
      telegramTask
    ,  "Telegram Task"   // A name just for humans
    ,  5120  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
    Serial.println("Task 2 Created");
//    vTaskDelete(NULL);
}

void timerTask(void *pvParameters){
    for(;;){
      vTaskDelay(1000);
      second++;
      Serial.print("s = ");
      Serial.print(second);
      Serial.println();
      if((second*1000) == (times * 60000)){
        Serial.println("\nmkn");      
        second = 0;
        kasihmakan();
      }
    }
}

void telegramTask(void *pvParameters){
  int n = 0;
  for(;;){
      vTaskDelay(1000);
      if (millis() > lastTimeBotRan + botRequestDelay)  {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while(numNewMessages) {
        Serial.println("got response");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      lastTimeBotRan = millis();
    }
  }
}

static void led_task_func(void *argp) {
    UniversalTelegramBot *ledp = (UniversalTelegramBot*)argp;
    unsigned stack_hwm = 0, temp;

    delay(1000);
    for(;;){
        temp = uxTaskGetStackHighWaterMark(nullptr);
        if(!stack_hwm || temp < stack_hwm){
            stack_hwm = temp;
            printf("Task for gpio %d has stack hwm %u\n", 1, stack_hwm);
        }
    }
}

void loop() {
  //telegramTask();
//  delay(1000);
//    second++;
//    printf("\ns = %d" , second);
//    if((second*1000) == (times * 60000)){
//      printf("\nmkn");      
//      second = 0;
//      kasihmakan();
//    }

//    if (millis() > lastTimeBotRan + botRequestDelay)  {
//      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//      printf("\nNumNewMsg = %d", numNewMessages);
//      while(numNewMessages) {
//        Serial.println("got response");
//        handleNewMessages(numNewMessages);
//        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
//      }
//      lastTimeBotRan = millis();
//    }
}

void kasihmakan(){
  digitalWrite(pinLed, LOW);
  servo1.write(100);
  delay(5000);
  servo1.write(0);
  digitalWrite(pinLed, HIGH);
  count++;
}
