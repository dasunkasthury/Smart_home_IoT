// Example of drawing a graphical "switch" and using
// the touch screen to change it's state.

// This sketch does not use the libraries button drawing
// and handling functions.

// Based on Adafruit_GFX library onoffbutton example.

// Touch handling for XPT2046 based screens is handled by
// the TFT_eSPI library.

// Calibration data is stored in SPIFFS so we need to include it
#include "FS.h"
#include <Arduino.h>

#include <SPI.h>

#include <TFT_eSPI.h> // Hardware-specific library

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Adafruit_NeoPixel.h>

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

// This is the file name used to store the touch coordinate
// calibration data. Change the name to start a new calibration.
#define CALIBRATION_FILE "/TouchCalData3"

// Set REPEAT_CAL to true instead of false to run calibration
// again, otherwise it will only be done once.
// Repeat calibration if you change the screen rotation.
#define REPEAT_CAL false

bool SwitchOn = false;

// Comment out to stop drawing black spots
#define BLACK_SPOT

// Switch position and size
#define FRAME_X 50
#define FRAME_Y 130
#define FRAME_W 220
#define FRAME_H 100

// Red zone size
#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W / 2)
#define REDBUTTON_H FRAME_H

// Green zone size
#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W / 2)
#define GREENBUTTON_H FRAME_H

#define WIFI_SSID "DK_MAX"
#define WIFI_PASSWORD "$@mpatha_637-208511"
#define API_KEY "AIzaSyBWxCAsEJW5umn9ypxTVVKghVQxgv1zifQ";
#define DATABASE_URL "https://esp32-home-932a5-default-rtdb.asia-southeast1.firebasedatabase.app/";
#define USER_EMAIL "dasunkasthury@gmail.com";
#define USER_PASSWORD "zaq1XSW@";
// Define the pin where the data line is connected
#define LED_PIN 16 // Change this to the pin you are using
// Define the number of LEDs in the strip
#define NUM_LEDS 5 // Change this to the number of LEDs you have
// Create an instance of the Adafruit_NeoPixel class
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPreMillis = 0;

bool showMode = true;
bool oldStateValue = false;
String oldMood = "";

void touch_calibrate();
void redBtn();
void greenBtn();
void setMood(String mood);
// Function prototype for colorWipe
void colorWipe(uint32_t color, int wait);

//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void setup(void)
{
  Serial.begin(921600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(1);

  // call screen calibration
  touch_calibrate();

  // clear screen
  tft.fillScreen(TFT_BLUE);

  tft.fillRect(100, 50, 20, 30, TFT_MAGENTA);

  // tft.setTextSize(3);
  // tft.println("This is Your Mood");
  // setMood("Noo");

  // Draw button (this example does not use library Button class)
  redBtn();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  /* Assign the api key (required) */
  config.api_key = API_KEY;
  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;
  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);
  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);
  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);
  Firebase.setDoubleDigits(5);
  config.timeout.serverResponse = 10 * 1000;
  sendDataPreMillis = 0;
  // Initialize the LED strip
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
}
//------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------
void loop()
{
  uint16_t x, y;

  // See if there's any touch data for us
  if (tft.getTouch(&x, &y))
  {
// Draw a block spot to show where touch was calculated to be
#ifdef BLACK_SPOT
    tft.fillCircle(x, y, 2, TFT_BLACK);
#endif

    if (SwitchOn)
    {
      if ((x > REDBUTTON_X) && (x < (REDBUTTON_X + REDBUTTON_W)))
      {
        if ((y > REDBUTTON_Y) && (y <= (REDBUTTON_Y + REDBUTTON_H)))
        {
          Serial.println("Red btn hit");
          redBtn();
        }
      }
    }
    else // Record is off (SwitchOn == false)
    {
      if ((x > GREENBUTTON_X) && (x < (GREENBUTTON_X + GREENBUTTON_W)))
      {
        if ((y > GREENBUTTON_Y) && (y <= (GREENBUTTON_Y + GREENBUTTON_H)))
        {
          Serial.println("Green btn hit");
          greenBtn();
        }
      }
    }

    Serial.println(SwitchOn);
  }

  if (Firebase.ready() && (millis() - sendDataPreMillis > 1000 || sendDataPreMillis == 0))
  {
    sendDataPreMillis = millis();

    String mood = "INIT";
    int red_value = Firebase.RTDB.getInt(&fbdo, "/led/Red", &red_value) ? red_value : 0;
    int blue_value = Firebase.RTDB.getInt(&fbdo, "/led/Blue", &blue_value) ? blue_value : 0;
    int green_value = Firebase.RTDB.getInt(&fbdo, "/led/Green", &green_value) ? green_value : 0;
    int ledBright = Firebase.RTDB.getInt(&fbdo, "/led/bright", &ledBright) ? ledBright : 100;
    mood = Firebase.RTDB.getString(&fbdo, "/led/mood", &mood) ? mood : "";
    Firebase.RTDB.getBool(&fbdo, "/led/state", &oldStateValue);

    setMood(mood);

    // if (showMode)
    // {
    //   Serial.println("mood ---------------------------------------");
    //   Serial.println(mood);
    //   Serial.println("mood  END---------------------------------------");
    //   setMood(mood);
    //   showMode = false;
    // }

    if (oldStateValue != SwitchOn)
    {
      bool isSuccess = Firebase.RTDB.setBool(&fbdo, "/led/state", SwitchOn ? 1 : 0);
      oldStateValue = isSuccess ? SwitchOn : oldStateValue;
    }

    int ledState;
    if (true)
    {
      digitalWrite(LED_BUILTIN, ledState);
      // Serial.println(ledState);
      // Serial.print("LED ON ");
      // Serial.println();
      strip.setBrightness(ledBright);
      colorWipe(strip.Color(red_value, green_value, blue_value), 50);
    }
    else
    {
      Serial.println(fbdo.errorReason().c_str());
    }
  }
}
//------------------------------------------------------------------------------------------

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin())
  {
    Serial.println("Formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE))
  {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f)
      {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL)
  {
    // calibration data valid
    tft.setTouch(calData);
  }
  else
  {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL)
    {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f)
    {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

void drawFrame()
{
  tft.drawRect(FRAME_X, FRAME_Y, FRAME_W, FRAME_H, TFT_BLACK);
}

// Draw a red button
void redBtn()
{
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_RED);
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("YES", GREENBUTTON_X + (GREENBUTTON_W / 2), GREENBUTTON_Y + (GREENBUTTON_H / 2));
  SwitchOn = false;
}

// Draw a green button
void greenBtn()
{
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_DARKGREY);
  drawFrame();
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("NO", REDBUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
  SwitchOn = true;
}

void setMood(String mood)
{
  if (oldMood != mood)
  {
    String myString = mood + "?";
    tft.fillRect(0, 0, 320, 100, TFT_BLACK); // 320 * 240
    tft.setCursor(0, 0);
    tft.setTextSize(3);
    tft.println("Is Your Mood");
    tft.setCursor(100, 40);
    tft.setTextSize(5);
    tft.print(myString);
    oldMood = mood;
    redBtn();
  }
}

void colorWipe(uint32_t color, int wait)
{
  for (int i = 0; i < strip.numPixels(); i++)
  {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}
