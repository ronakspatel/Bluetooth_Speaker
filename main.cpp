#include "AudioTools.h"
#include "BluetoothA2DPSink.h"
#include <arduinoFFT.h>
#include <TFT_eSPI.h>
#include <math.h>


// ===== Config =====
#define ANALOG_PIN 35
#define BACKLIGHT_PIN 4


const uint16_t SAMPLES = 64;
const double SAMPLING_FREQ = 44100;
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int MAX_BAR_HEIGHT = 80;
const float SMOOTHING_FACTOR = 0.90;


// ===== FFT Buffers and Object =====
float vReal[SAMPLES];
float vImag[SAMPLES];
ArduinoFFT<float> FFT(vReal, vImag, SAMPLES, SAMPLING_FREQ);


// ===== Bluetooth & I2S =====
I2SStream i2s;
BluetoothA2DPSink a2dp_sink(i2s);


// ===== Display =====
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);


// Retro Yellow/Magenta Color Palette
const uint16_t deep_navy = tft.color565(8, 12, 32);
const uint16_t retro_yellow = tft.color565(255, 255, 0);
const uint16_t retro_magenta = tft.color565(255, 0, 255);
const uint16_t dark_magenta = tft.color565(128, 0, 128);
const uint16_t bright_magenta = tft.color565(255, 50, 127);


// ===== Visualizer Settings =====
const uint8_t NUM_BARS = 64;
const uint8_t binEdges[NUM_BARS + 1] = {
 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
 61, 62, 63, 64, 65
};
float smoothedMagnitudes[NUM_BARS] = {0};


// ===== Enhanced Animation Variables =====
unsigned long animationTime = 0;
float pulsePhase = 0;
int glowIntensity = 0;
float wavePhase = 0;


// ===== Metadata State =====
String title = "";
String artist = "";
String album = "";
String lastTitle = "";
String lastArtist = "";
String lastAlbum = "";


// ===== Playback State =====
esp_avrc_playback_stat_t status;


// ===== Bluetooth Connection State =====
bool btConnected = false;
bool screenCleared = false;


// ===== Volume Indicator State =====
int lastVolume = -1;
unsigned long volumeChangeTime = 0;
const unsigned long VOLUME_DISPLAY_DURATION = 3000; // Show for 3 seconds


// ===== AVRCP Metadata Callback =====
void avrcpMetadataCallback(uint8_t id, const uint8_t *text) {
 String value = (const char *)text;


 if (id == 0x01) {
   title = value;
 }
 else if (id == 0x02) {
   artist = value;
 }
 else if (id == 0x04) {
   album = value;
 }
}


// ===== Playback Status Callback =====
void avrc_rn_playstatus_callback(esp_avrc_playback_stat_t playback) {
 status = playback;


 switch (playback) {
   case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_STOPPED: {
     break;
   }
   case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PLAYING: {
     break;
   }
   case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_PAUSED: {
     break;
   }
   case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_FWD_SEEK: {
     break;
   }
   case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_REV_SEEK: {
     break;
   }
   case esp_avrc_playback_stat_t::ESP_AVRC_PLAYBACK_ERROR: {
     break;
   }
   default: {
     break;
   }
 }
}


// ===== Bluetooth Connection Callback =====
void bt_connection_callback(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
 if (event == ESP_A2D_CONNECTION_STATE_EVT) {
   if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
     btConnected = true;
     screenCleared = false;
   }
   else {
     btConnected = false;
   }
 }
}


// ===== Retro Grid Background =====
void drawRetroBackground() {
 // Create dark gradient background
 for (int y = 0; y < SCREEN_HEIGHT; y++) {
   uint8_t intensity = map(y, 0, SCREEN_HEIGHT, 20, 5);
   uint16_t bg_color = tft.color565(intensity/4, intensity/8, intensity/2);
   sprite.drawFastHLine(0, y, SCREEN_WIDTH, bg_color);
 }
}


// ===== Twinkling Stars =====
void drawTwinklingStars() {
 // Add twinkling stars over the background
 for (int i = 0; i < 25; i++) {
   int starX = (i * 17 + 23) % SCREEN_WIDTH;
   int starY = (i * 31 + 47) % SCREEN_HEIGHT;
  
   // Make stars twinkle based on time and position
   float twinkle = sin(animationTime * 0.003 + i * 0.7) * 0.5 + 0.5;
   uint8_t brightness = twinkle * 255;
  
   if (brightness > 120) {
     uint16_t starColor = tft.color565(brightness, brightness, brightness);
     sprite.drawPixel(starX, starY, starColor);
    
     // Bigger stars get a cross pattern
     if (brightness > 200) {
       sprite.drawPixel(starX + 1, starY, starColor);
       sprite.drawPixel(starX - 1, starY, starColor);
       sprite.drawPixel(starX, starY + 1, starColor);
       sprite.drawPixel(starX, starY - 1, starColor);
     }
   }
 }
}


// ===== Enhanced Text with Magenta Glow =====
void drawGlowText(String text, int x, int y, uint16_t color, uint16_t glowColor) {
 // Draw glow effect (multiple offset shadows for stronger glow)
 sprite.setTextColor(glowColor);
 sprite.drawString(text, x-1, y-1);
 sprite.drawString(text, x+1, y-1);
 sprite.drawString(text, x-1, y+1);
 sprite.drawString(text, x+1, y+1);
 sprite.drawString(text, x-2, y);
 sprite.drawString(text, x+2, y);
 sprite.drawString(text, x, y-2);
 sprite.drawString(text, x, y+2);
  // Draw main text
 sprite.setTextColor(color);
 sprite.drawString(text, x, y);
}


// ===== Volume Indicator =====
void drawVolumeIndicator() {
 int currentVolume = a2dp_sink.get_volume();
  // Check if volume has changed
 if (currentVolume != lastVolume) {
   lastVolume = currentVolume;
   volumeChangeTime = millis();
 }
  // Only show volume indicator for a few seconds after change
 if (millis() - volumeChangeTime < VOLUME_DISPLAY_DURATION) {
   // Centered volume display
   int volumePercent = map(currentVolume, 0, 127, 0, 100);
   String volumeText = String(volumePercent) + "%";
  
   // Calculate total width needed
   int textWidth = sprite.textWidth(volumeText);
   int barWidth = 60; // Fixed bar width
   int spacing = 4; // Space between text and bar
   int totalWidth = textWidth + spacing + barWidth;
  
   // Center the entire volume display
   int startX = (SCREEN_WIDTH - totalWidth) / 2;
   int textX = startX;
   int textY = 62;
   int barX = startX + textWidth + spacing;
   int barY = 63;
   int barHeight = 6;
  
   // Draw volume percentage text
   sprite.setTextColor(retro_yellow);
   sprite.drawString(volumeText, textX, textY);
  
   // Draw volume bar background
   sprite.drawRect(barX, barY, barWidth, barHeight, dark_magenta);
  
   // Calculate volume fill (volume range is typically 0-127)
   int volumeLevel = map(currentVolume, 0, 127, 0, barWidth - 2);
   volumeLevel = constrain(volumeLevel, 0, barWidth - 2);
  
   // Draw volume fill with color based on level
   if (volumeLevel > 0) {
     // Color transitions from bright_magenta (low) to retro_yellow (high)
     float volumeRatio = (float)currentVolume / 127.0;
     uint16_t volumeColor;
    
     if (volumeRatio < 0.1) {
       volumeColor = bright_magenta;
     } else {
       float transition = (volumeRatio - 0.1) / 0.9;
       transition = constrain(transition, 0.0, 1.0);
      
       uint8_t red = 255;
       uint8_t green = 50 + (transition * 205); // 50 to 255
       uint8_t blue = 127 * (1.0 - transition); // 127 to 0
      
       volumeColor = tft.color565(red, green, blue);
     }
    
     sprite.fillRect(barX + 1, barY + 1, volumeLevel, barHeight - 2, volumeColor);
   }
 }
}


// ===== Retro Wave Border Effect =====
void drawWaveBorder() {
 // Use exact same magenta formula as metadata glow: tft.color565(glowIntensity, 0, glowIntensity/2)
  for (int i = 0; i < SCREEN_WIDTH; i++) {
   float wave1 = (sin(wavePhase + i * 0.2) + 1) * 127; // 0-254 range like glowIntensity
   float wave2 = (cos(wavePhase * 1.3 + i * 0.15) + 1) * 127; // 0-254 range
  
   // Use exact same formula as titleGlow: tft.color565(glowIntensity, 0, glowIntensity/2)
   uint16_t topColor = tft.color565(wave1, 0, wave1/2);
   uint16_t bottomColor = tft.color565(wave2, 0, wave2/2);
  
   sprite.drawPixel(i, 0, topColor);
   sprite.drawPixel(i, 1, topColor);
   sprite.drawPixel(i, SCREEN_HEIGHT-2, bottomColor);
   sprite.drawPixel(i, SCREEN_HEIGHT-1, bottomColor);
 }
  // Side waves using exact same glow formula
 for (int i = 0; i < SCREEN_HEIGHT; i++) {
   float wave1 = (sin(wavePhase * 0.8 + i * 0.25) + 1) * 127; // 0-254 range
   float wave2 = (cos(wavePhase * 1.1 + i * 0.2) + 1) * 127; // 0-254 range
  
   // Use exact same formula as titleGlow: tft.color565(glowIntensity, 0, glowIntensity/2)
   uint16_t leftColor = tft.color565(wave1, 0, wave1/2);
   uint16_t rightColor = tft.color565(wave2, 0, wave2/2);
  
   sprite.drawPixel(0, i, leftColor);
   sprite.drawPixel(1, i, leftColor);
   sprite.drawPixel(SCREEN_WIDTH-2, i, rightColor);
   sprite.drawPixel(SCREEN_WIDTH-1, i, rightColor);
 }
}


// ===== Enhanced Metadata Display =====
void displayMetadata() {
 static int scrollX_title = 0;
 static int scrollX_artist = 0;
 static int scrollX_album = 0;


 static unsigned long lastScrollTime = 0;
 static unsigned long lastChangeTime = 0;
 static bool scrolling = false;


 const int scrollDelay = 45;
 const int scrollStartDelay = 2000;


 // Update animation variables
 animationTime = millis();
 pulsePhase += 0.15;
 wavePhase += 0.08;
 glowIntensity = (sin(pulsePhase) + 1) * 127;


 if ((title != lastTitle) || (artist != lastArtist) || (album != lastAlbum)) {
   lastTitle = title;
   lastArtist = artist;
   lastAlbum = album;


   scrollX_title = 1;
   scrollX_artist = 1;
   scrollX_album = 1;


   lastChangeTime = millis();
   scrolling = false;
 }


 if ((!scrolling) && (millis() - lastChangeTime > scrollStartDelay)) {
   scrolling = true;
   lastScrollTime = millis();
 }


 if (scrolling && (millis() - lastScrollTime > scrollDelay)) {
   scrollX_title--;
   scrollX_artist--;
   scrollX_album--;
   lastScrollTime = millis();
 }


 // Background with twinkling stars
 drawRetroBackground();
 drawTwinklingStars();


 sprite.setTextDatum(TL_DATUM);
 sprite.setTextFont(1);
 sprite.setTextSize(1);


 // Enhanced title with pulsing magenta glow
 uint16_t titleGlow = tft.color565(glowIntensity, 0, glowIntensity/2);
 int width = sprite.textWidth(title);
 if (width > (SCREEN_WIDTH - 4)) {
   drawGlowText(title, scrollX_title, 8, retro_yellow, titleGlow);
   if (scrollX_title < (-width - 4)) {
     scrollX_title = SCREEN_WIDTH - 4;
   }
 } else {
   drawGlowText(title, 2 + ((SCREEN_WIDTH - 4 - width) / 2), 8, retro_yellow, titleGlow);
 }


 // Enhanced artist with pulsing magenta glow
 uint16_t artistGlow = tft.color565(glowIntensity, 0, glowIntensity/2);
 width = sprite.textWidth(artist);
 if (width > (SCREEN_WIDTH - 4)) {
   drawGlowText(artist, scrollX_artist, 26, retro_yellow, artistGlow);
   if (scrollX_artist < (-width - 4)) {
     scrollX_artist = SCREEN_WIDTH - 4;
   }
 } else {
   drawGlowText(artist, 2 + ((SCREEN_WIDTH - 4 - width) / 2), 26, retro_yellow, artistGlow);
 }


 // Enhanced album with pulsing magenta glow
 uint16_t albumGlow = tft.color565(glowIntensity, 0, glowIntensity/2);
 width = sprite.textWidth(album);
 if (width > (SCREEN_WIDTH - 4)) {
   drawGlowText(album, scrollX_album, 44, retro_yellow, albumGlow);
   if (scrollX_album < (-width - 4)) {
     scrollX_album = SCREEN_WIDTH - 4;
   }
 } else {
   drawGlowText(album, 2 + ((SCREEN_WIDTH - 4 - width) / 2), 44, retro_yellow, albumGlow);
 }


 // Draw volume indicator (only shows when volume changes)
 drawVolumeIndicator();
}


// ===== Circular Plasma Visualizer =====
void displayVisualizer() {
 if ((status == ESP_AVRC_PLAYBACK_PAUSED) || (status == ESP_AVRC_PLAYBACK_STOPPED)) {
   return;
 }


 int barWidth = 1;


 const float BASE_THRESHOLD = 750.0;
 const float THRESHOLD_DECAY = 0.15;


 for (int i = 0; i < NUM_BARS; i++) {
   float sum = 0;


   for (int j = binEdges[i]; j < binEdges[i + 1]; j++) {
     sum += vReal[j];
   }


   float avg = sum / (binEdges[i + 1] - binEdges[i]) * 10 * pow(25, -1.0 * a2dp_sink.get_volume() / 115.0);
   float noiseThreshold = BASE_THRESHOLD / (1.0 + THRESHOLD_DECAY * i);


   if (avg < noiseThreshold) {
     avg = 0;
   }


   float freqBoost = pow(1.0 + 500.0 * i / (float)(NUM_BARS - 1), 3.5);
   float scaled = constrain(50 * freqBoost * avg, 0, MAX_BAR_HEIGHT);


   smoothedMagnitudes[i] = SMOOTHING_FACTOR * smoothedMagnitudes[i] + (1.0f - SMOOTHING_FACTOR) * scaled;
   int displayMag = (int)smoothedMagnitudes[i];


   uint16_t color = sprite.color565(255, map(displayMag, 0, MAX_BAR_HEIGHT, 0, 200), map(displayMag, 0, MAX_BAR_HEIGHT, 180, 0));
   int y = SCREEN_HEIGHT - displayMag;


   sprite.fillRect(i * barWidth * 2, y, barWidth, displayMag, color);
 }
}


// ===== Enhanced Display Interface =====
void displayInterface() {
 displayMetadata();
 displayVisualizer();
 drawWaveBorder();
 sprite.pushSprite(0, 0);
}


// ===== Metadata Received Check =====
bool metadataReceived() {
 if ((title == "") && (artist == "") && (album == "")) {
   return false;
 }
 return true;
}


// ===== Setup =====
void setup() {
 pinMode(BACKLIGHT_PIN, OUTPUT);
 digitalWrite(BACKLIGHT_PIN, LOW);


 tft.init();
 tft.setRotation(0);
 tft.fillScreen(deep_navy);


 sprite.setColorDepth(8);
 sprite.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
 sprite.fillSprite(deep_navy);


 auto cfg = i2s.defaultConfig(TX_MODE);
 cfg.bits_per_sample = 32;
 cfg.sample_rate = 44100;
 cfg.channels = 2;
 cfg.pin_bck = 27;
 cfg.pin_ws = 26;
 cfg.pin_data = 25;


 i2s.begin(cfg);


 a2dp_sink.set_avrc_metadata_attribute_mask(
   ESP_AVRC_MD_ATTR_TITLE |
   ESP_AVRC_MD_ATTR_ARTIST |
   ESP_AVRC_MD_ATTR_ALBUM
 );


 a2dp_sink.set_avrc_metadata_callback(avrcpMetadataCallback);
 a2dp_sink.set_avrc_rn_playstatus_callback(avrc_rn_playstatus_callback);


 a2dp_sink.start("🔊Mini Me🔊");


 analogReadResolution(12);
 pinMode(ANALOG_PIN, INPUT);
}


// ===== Loop =====
void loop() {
 unsigned long startMicros = micros();


 for (int i = 0; i < SAMPLES; i++) {
   vReal[i] = analogRead(ANALOG_PIN);
   vImag[i] = 0;


   while (micros() - startMicros < (i + 1) * (1000000UL / SAMPLING_FREQ)) {
     yield();
   }
 }


 FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
 FFT.compute(FFT_FORWARD);
 FFT.complexToMagnitude();


 if (a2dp_sink.is_connected() && metadataReceived()) {
   digitalWrite(BACKLIGHT_PIN, HIGH);
   screenCleared = false;
   displayInterface();
 } else {
   digitalWrite(BACKLIGHT_PIN, LOW);


   if (!screenCleared) {
     tft.fillScreen(deep_navy);
     screenCleared = true;
   }
 }
}


