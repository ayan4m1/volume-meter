#include <Arduino.h>
#include <SPIFFS.h>

#include <Audio.hpp>
#include <gfx.hpp>
#include <st7789.hpp>

#include "NotoSans_Bold.hpp"

#define VOLUME_DECAY_ALPHA 0.3

#define LCD_WIDTH 128
#define LCD_HEIGHT 128
#define LCD_ROTATION 2
#define LCD_HOST SPI1_HOST

#define PIN_NUM_CS 5
#define PIN_NUM_MOSI 2
#define PIN_NUM_MISO -1
#define PIN_NUM_CLK 3
#define PIN_NUM_DC 6
#define PIN_NUM_RST 1
#define PIN_NUM_BCKL 10

using namespace arduino;
using namespace gfx;

using bus_type =
    tft_spi_ex<LCD_HOST, PIN_NUM_CS, PIN_NUM_MOSI, PIN_NUM_MISO, PIN_NUM_CLK,
               SPI_MODE0, LCD_WIDTH * LCD_HEIGHT * 2 + 8>;
using lcd_type = st7789<LCD_WIDTH, LCD_HEIGHT, PIN_NUM_DC, PIN_NUM_RST, -1,
                        bus_type, LCD_ROTATION, false, 400, 200>;
using lcd_color = color<typename lcd_type::pixel_type>;

lcd_type lcd;

constexpr static const size16 bmp_size(LCD_WIDTH, LCD_HEIGHT);

using bmp_type = bitmap<decltype(lcd)::pixel_type>;
using bmp_color = color<typename bmp_type::pixel_type>;

uint8_t bmp_buf[bmp_type::sizeof_buffer(bmp_size)];
bmp_type bmp(bmp_size, bmp_buf);

Audio audio = Audio();

double decibelAccum = 0;

const uint8_t red_hue = 0;
const uint8_t green_hue = 128 - 32;

void setup() {
  Serial.begin(115200);

  // override backlight control (active LOW)
  pinMode(PIN_NUM_BCKL, OUTPUT);
  digitalWrite(PIN_NUM_BCKL, LOW);

  // set up display
  lcd.initialize();

  // start microphone task loop
  audio.begin();
}

void loop() {
  // check if we have audio data
  double db = audio.getDecibels();

  if (db == 0) {
    return;
  }

  decibelAccum =
      (VOLUME_DECAY_ALPHA * db) + (1.0 - VOLUME_DECAY_ALPHA) * decibelAccum;

  auto color_hsv = hsv_pixel<24>();

  color_hsv.channel<channel_name::H>(green_hue + (red_hue - green_hue) *
                                                     (db / MIC_OVERLOAD_DB));
  color_hsv.channel<channel_name::S>(255);
  color_hsv.channel<channel_name::V>(255);

  auto color = rgb_pixel<16>();

  convert<hsv_pixel<24>, rgb_pixel<16>>(color_hsv, &color);

  char buffer[12];

  sprintf(buffer, "%.0f", decibelAccum);

  open_text_info oti;
  oti.font = &NotoSans_Bold;
  oti.scale = NotoSans_Bold.scale(64);
  oti.text = buffer;

  auto text_size = NotoSans_Bold.measure_text(
      (ssize16)lcd.bounds().dimensions(), spoint16(0, 0), oti.text, oti.scale);

  auto text_pos = text_size.bounds().center((srect16)bmp.bounds());

  draw::filled_rectangle(bmp, bmp.bounds(), color);
  draw::text(bmp, text_pos, oti, lcd_color::black, color);
  draw::bitmap_async(lcd, lcd.bounds(), bmp, bmp.bounds());
}