#include "Adafruit_SSD1306.h"
#include "config.h"
#include "hardware/gpio.h"
#include "sensor.h"
#include "state.h"

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, -1);

static int FONT_COLS, FONT_ROWS;

void terminal_poweroff() { display.ssd1306_command(SSD1306_DISPLAYOFF); }

void terminal_poweron() { display.ssd1306_command(SSD1306_DISPLAYON); }

void _update_font_info() {
  int16_t dummy;
  uint16_t w, h;

  display.getTextBounds("a", 0, 0, &dummy, &dummy, &w, &h);

  FONT_COLS = w, FONT_ROWS = h;
}

void terminal_init() {
  Wire.setSDA(DISPLAY_SDA);
  Wire.setSCL(DISPLAY_SCL);
  Wire.begin();

  _update_font_info();

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    MSerial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(50);

  display.clearDisplay();
  display.display();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();
  state_set_display_active(true);
}

void terminal_toogle_display_power() {
  if (state_get_display_active()) {
    terminal_poweroff();
    state_set_display_active(false);
  } else {
    terminal_poweron();
    state_set_display_active(true);
  }
}

void draw_text_field(int16_t x, int16_t y, const char *text, bool highlight) {
  const int16_t row_y = y;
  const int16_t row_h = FONT_ROWS;

  if (highlight) {
    display.fillRect(0, row_y, DISPLAY_WIDTH, row_h, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.fillRect(0, row_y, DISPLAY_WIDTH, row_h, SSD1306_BLACK);
    display.setTextColor(SSD1306_WHITE);
  }

  display.setCursor(x, y);
  display.print(text);
}

void terminal_redraw_all() {
  if (!state_get_display_active())
    return;

  const int selected = state_get_selected();
  const moisture_sensor_t *sensors = state_get_sensors();
  display.fillRect(0, 0, DISPLAY_WIDTH, FONT_ROWS * OPT_MAX, SSD1306_BLACK);

  display.setCursor(0, 0);

  for (int i = 0; i < SENSORS; ++i) {
    if (selected == i) {

      display.fillRect(0, i * FONT_ROWS, DISPLAY_WIDTH, FONT_ROWS,
                       SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);

    } else {
      display.setTextColor(SSD1306_WHITE);
    }

    display.printf("S%d: %d T: %d\n", i + 1, sensors[i].adc_value,
                   sensors[i].threshold);
    display.setTextColor(SSD1306_WHITE);
  }

  draw_text_field(0, display.getCursorY(), "[POWER OFFW]",
                  selected == OPT_POWEROFF);

  display.display();
}
