  

    #define BLYNK_TEMPLATE_ID "TMPL6pz-YQtge"
    #define BLYNK_TEMPLATE_NAME "daikin"
    #define BLYNK_AUTH_TOKEN "ua-7Go5k_QnTyuBb3ReCx1nku_HuZ0Mv"

    //================ LIBRARY =================

    #include <Arduino.h>
    #include <ESP8266WiFi.h>
    #include <BlynkSimpleEsp8266.h>
    #include <IRremoteESP8266.h>
    #include <IRsend.h>
    #include <IRrecv.h>
    #include <IRutils.h>
    #include <IRac.h>
    #include <ir_Daikin.h>
    #include <DHT.h>

    //================ WIFI ====================

    char ssid[] = "iPhone";
    char pass[] = "12345678";

    //================ PIN =====================

    // IR
    const uint16_t kIrLed   = 4;   // D2 GPIO4
    const uint16_t kRecvPin = 14;  // D5 GPIO14

    // DHT22
    #define DHTPIN 5      
    #define DHTTYPE DHT22

    //================ DHT =====================

    DHT dht(DHTPIN, DHTTYPE);

    //================ IR ======================

    IRDaikinESP ac(kIrLed);
    IRDaikin128 ac_rx(0);

    const uint16_t kCaptureBufferSize = 1024;
    const uint8_t kTimeout = 50;

    IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);

    decode_results results;

    //================ BLYNK ===================

    WidgetLCD lcd(V7);

    BlynkTimer timer;

    //================ VARIABLE ================

    bool powerState = false;

    int temp = 24;

    int modeIndex = 0;
    int fanIndex = 0;

    bool swingState = false;

    // DHT22
    float roomTemp = 0;
    float humidity = 0;

    //==========================================
    // UPDATE LCD
    //==========================================

    void updateLCD() {

      lcd.clear();

      lcd.print(0, 0, "DAIKIN AC");

      lcd.print(0, 1, "TEMP:");
      lcd.print(5, 1, temp);

      lcd.print(10, 1, powerState ? "ON " : "OFF");
    }

    //==========================================
    // READ DHT22
    //==========================================

    void readDHT22() {

      roomTemp = dht.readTemperature();

      humidity = dht.readHumidity();

      if (isnan(roomTemp) || isnan(humidity)) {

        Serial.println("DHT22 ERROR");
        return;
      }

      // SEND TO BLYNK
      Blynk.virtualWrite(V9, roomTemp);

      Blynk.virtualWrite(V10, humidity);

      // SERIAL MONITOR
      Serial.println("===== DHT22 =====");

      Serial.print("ROOM TEMP : ");
      Serial.print(roomTemp);
      Serial.println(" C");

      Serial.print("HUMIDITY  : ");
      Serial.print(humidity);
      Serial.println(" %");

      Serial.println("=================");
    }

    //==========================================
    // SEND IR TO AC
    //==========================================

    void sendAC() {

      ac.send();

      Serial.println("================================");
      Serial.println("IR SENT TO AC");
      Serial.println("================================");

      Serial.print("POWER : ");
      Serial.println(powerState ? "ON" : "OFF");

      Serial.print("TEMP  : ");
      Serial.println(temp);

      Serial.print("MODE  : ");

      switch (modeIndex) {

        case 0:
          Serial.println("COOL");
          break;

        case 1:
          Serial.println("FAN");
          break;
      }

      Serial.print("FAN   : ");

      switch (fanIndex) {

        case 0:
          Serial.println("AUTO");
          break;

        case 1:
          Serial.println("LOW");
          break;

        case 2:
          Serial.println("MED");
          break;

        case 3:
          Serial.println("HIGH");
          break;
      }

      Serial.print("SWING : ");
      Serial.println(swingState ? "ON" : "OFF");

      Serial.println("================================");

      updateLCD();
    }

    //==========================================
    // SETUP
    //==========================================

    void setup() {

      Serial.begin(115200);

      Serial.println();
      Serial.println("STARTING SYSTEM...");

      //========== DHT22 ==========
      dht.begin();

      //========== IR TX ==========
      ac.begin();

      //========== IR RX ==========
      irrecv.enableIRIn();

      //========== DEFAULT AC =====

      ac.off();

      ac.setMode(kDaikinCool);

      ac.setTemp(temp);

      ac.setFan(kDaikinFanAuto);

      ac.setSwingVertical(false);

      //========== WIFI + BLYNK ===

      Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

      //========== LCD ============

      updateLCD();

      Blynk.virtualWrite(V8, temp);

      //========== TIMER ==========
      //timer.setInterval(3000L, readDHT22);

      Serial.println("SYSTEM READY");
    }

    //==========================================
    // LOOP
    //==========================================

    void loop() {

      Blynk.run();

      timer.run();

      //========================================
      // IR RECEIVER LEARNING
      //========================================

      if (irrecv.decode(&results)) {

        Serial.println();
        Serial.println("######## IR SIGNAL RECEIVED ########");

        // Human readable
        Serial.println(resultToHumanReadableBasic(&results));

        // RAW CODE
        Serial.println(resultToSourceCode(&results));

        Serial.println("####################################");
        Serial.println();

        irrecv.resume();
      }
    }

    //==========================================
    // POWER BUTTON
    // V0 = SWITCH
    //==========================================

    BLYNK_WRITE(V0) {

      powerState = param.asInt();

      if (powerState) {

        ac.on();

        Serial.println("AC ON");

      } else {

        ac.off();

        Serial.println("AC OFF");
      }

      sendAC();
    }

    //==========================================
    // MODE BUTTON
    // V1 = BUTTON
    //==========================================

      BLYNK_WRITE(V2) {

        if (param.asInt() == 1) {

          modeIndex++;

          if (modeIndex > 2)
            modeIndex = 0;

          switch (modeIndex) {

            case 0:

              ac.setMode(kDaikinCool);

              Serial.println("MODE COOL");

              break;

            case 1:

              ac.setMode(kDaikinFan);

              Serial.println("MODE FAN");

              break;
          }

          sendAC();
        }
      }

      //==========================================
      // FAN BUTTON
      // V2 = BUTTON
      //==========================================

      BLYNK_WRITE(V3) {

        if (param.asInt() == 1) {

          fanIndex++;

          if (fanIndex > 3)
            fanIndex = 0;

          switch (fanIndex) {

            case 0:

              ac.setFan(kDaikinFanAuto);

              break;

            case 1:

              ac.setFan(1);

              break;

            case 2:

              ac.setFan(3);

              break;

            case 3:

              ac.setFan(5);

              break;
          }

          sendAC();
        }
      }

      //==========================================
      // SWING BUTTON
      // V3 = BUTTON
      //==========================================

      BLYNK_WRITE(V11) {

        if (param.asInt() == 1) {

          swingState = !swingState;

          ac.setSwingVertical(swingState);
          Blynk.virtualWrite(V11, swingState ? "ON" : "OFF");

          sendAC();
        }
      }

      //==========================================
      // TEMP UP
      // V4 = BUTTON
      //==========================================

      BLYNK_WRITE(V5) {

        if (param.asInt() == 1) {

          temp++;

          if (temp > 30)
            temp = 30;

          ac.setTemp(temp);

          Blynk.virtualWrite(V8, temp);

          sendAC();
        }
      }

      //==========================================
      // TEMP DOWN
      // V5 = BUTTON
      //==========================================

      BLYNK_WRITE(V6) {

        if (param.asInt() == 1) {

          temp--;

          if (temp < 16)
            temp = 16;

          ac.setTemp(temp);

          Blynk.virtualWrite(V8, temp);

          sendAC();
        }
      }



      