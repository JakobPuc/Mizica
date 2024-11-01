#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/* 
    1 ... več teksta
    0 ... manj teksta
*/
#define VERBOSE 1

/* Pini za I2S */
#define I2S_DOUT 4
#define I2S_BCLK 0
#define I2S_LRC 2

/* Pini za SD kartico */
// !opomba: VCC more bit nujno povezan na 5 V, drgač dela čudn
#define SD_CS 5
#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCK 18

/* I2C zaslon */
#define COLS 16
#define ROWS 2
LiquidCrystal_I2C lcd(0x27, COLS, ROWS);

Audio audio;

/* WiFi */
#define WIFI_TIMEOUT 5
int retries = WIFI_TIMEOUT;
char ssid[] = "moje_omrezje";
char password[] = "moje_geslo";
bool on_sd = false;

static volatile bool wifi_connected = false;

void wifi_on_connect()
{
    Serial.printf("INFO: Connected to \'%s\' on %d. attempt\n", ssid, WIFI_TIMEOUT - retries + 1);
    retries = WIFI_TIMEOUT;
    
    if (!on_sd)
        audio.connecttohost("http://mp3.rtvslo.si/val202");
}

void wifi_on_disconnect()
{
    if (retries > 0) {
        Serial.printf("INFO: Disconnected from WiFi network, trying to reconnect (attempt %d out of %d)\n", WIFI_TIMEOUT - retries + 1, WIFI_TIMEOUT);
        retries--;
        delay(1000);

        WiFi.begin(ssid, password);
    }
}

void wifi_connected_loop()
{
    if (!on_sd)
        audio.loop();
}

void wifi_disconnected_loop()
{

}

void wifi_event(WiFiEvent_t event)
{
    switch(event) {
        case SYSTEM_EVENT_AP_START:
            //can set ap hostname here
            WiFi.softAPsetHostname("Miza");
            //enable ap ipv6 here
            WiFi.softAPenableIpV6();
            break;
        case SYSTEM_EVENT_STA_START:
            //set sta hostname here
            WiFi.setHostname("Miza");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            //enable sta ipv6 here
            WiFi.enableIpV6();
            break;
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            //both interfaces get the same event
            Serial.print("STA IPv6: ");
            Serial.println(WiFi.localIPv6());
            Serial.print("AP IPv6: ");
            Serial.println(WiFi.softAPIPv6());
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            wifi_on_connect();
            wifi_connected = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            wifi_connected = false;
            wifi_on_disconnect();
            break;
        default:
            break;
    }
}

void setup()
{
    /* Serijska povezava */
    Serial.begin(115200);

    /* LCD */
    Serial.printf("INFO: Initialising LCD display...\n");
    lcd.init();
    lcd.backlight();
    
    /* SPI */
    // opomba: ubistvu nepotrebno
    Serial.printf("INFO: Initialising SPI for SD card reader...\n");
    pinMode(SD_CS, OUTPUT);      
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    /* I2S */
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(21); // default 0...21

    delay(100);

    /* SD kartica */
    // zadeva se mal čudn obnaša pr soft rebootu (nevem a je zarad moje kinezer SD kartice ali pa ?????)
    // prtož se mi z 'A hard error occurred in the low level disk I/O layer'
    // to se da rešt tko da al kartico še enkrat daš not pa rebootaš al pa esp izštekaš pa vštekaš
    Serial.printf("INFO: Mounting SD card...\n");

    if (SD.begin(SD_CS)) {
        Serial.printf("INFO: Filesystem size: %lld MiB\n", SD_MMC.totalBytes() >> 20);
        
        audio.connecttoFS(SD, "/tuctuc.wav");
        on_sd = true;
    } else {
        Serial.printf("ERROR: Failed to mount SD card, falling back to network stream instead\n");
    }

    /* Wi-Fi */
    Serial.printf("INFO: Attempting to connect to \'%s\'...\n", ssid);
    WiFi.onEvent(wifi_event);
    WiFi.disconnect(false, true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
}

void loop()
{
    if (on_sd)
        audio.loop();

    if (wifi_connected)
        wifi_connected_loop();
    else
        wifi_disconnected_loop();
}

#if VERBOSE == 1
void audio_info(const char *info)
{
    Serial.print("INFO: ");
    Serial.println(info);
}

void audio_id3data(const char *info)
{
    Serial.print("INFO: id3data: ");
    Serial.println(info);
}

void audio_eof_mp3(const char *info)
{
    Serial.print("INFO: eof_mp3: ");
    Serial.println(info);
}

void audio_showstation(const char *info)
{
    Serial.print("INFO: Station: ");
    Serial.println(info);
}

void audio_showstreamtitle(const char *info)
{
    Serial.print("INFO: Streamtitle: ");
    Serial.println(info);
}

void audio_bitrate(const char *info)
{
    Serial.print("INFO: Bitrate: ");
    Serial.println(info);
}

void audio_commercial(const char *info)
{ // duration in sec
    Serial.print("INFO: Commercial: ");
    Serial.println(info);
}

void audio_icyurl(const char *info)
{ // homepage
    Serial.print("INFO: icyurl: ");
    Serial.println(info);
}

void audio_lasthost(const char *info)
{ // stream URL played
    Serial.print("INFO: Lasthost: ");
    Serial.println(info);
}

void audio_eof_speech(const char *info)
{
    Serial.print("INFO: eof_speech: ");
    Serial.println(info);
}
#endif
