#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Setup SSD1306 screen
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin
#define CLK_DURING 800000UL // Speed (in Hz) for Wire transmissions in SSD1306 library calls
TwoWire twi = TwoWire(); // create our own TwoWire instance
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &twi, OLED_RESET, CLK_DURING); 

#define DS3231_ADDR  0x68      // DS3231 clock I2C address
#define SSD1306_ADDR 0x3C      // SSD1306 display I2C address

#define button1    9                       // Button B1 is connected to Arduino pin 9
#define button2    8                       // Button B2 is connected to Arduino pin 8
#define buzzer     3

char Time[]     = "  :  :  ";
char Calendar[] = "  /  /20  ";
char temperature[] = " 00.00";
char temperature_msb;
byte i, second, minute, hour, day, date, month, year, temperature_lsb;

// A time/day pair for an alarm
struct AlarmTime {
  char *time;
  byte days;
};

// Bitmask for days of the week
enum Days {
  SUNDAY = (1u << 0),
  MONDAY = (1u << 1),
  TUESDAY = (1u << 2),
  WEDNESDAY = (1u << 3),
  THURSDAY = (1u << 4),
  FRIDAY = (1u << 5),
  SATURDAY = (1u << 6)
};

// Store the times to alarm:  "HH:MM:SS", <day1 + day2 ...>
const byte alarmCount = 5;
const AlarmTime alarmTimes[alarmCount] = {
  {"07:55:00", MONDAY + TUESDAY + THURSDAY + FRIDAY},
  {"09:00:00", MONDAY + TUESDAY + THURSDAY + FRIDAY},
  {"12:25:00", MONDAY + TUESDAY + THURSDAY + FRIDAY},
  {"13:30:00", MONDAY + TUESDAY + THURSDAY + FRIDAY},
  {"14:00:00", WEDNESDAY}
};

void setup(void) {
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, HIGH);
  delay(1000);
  Serial.begin(115200);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR);  // initialize with the I2C addr
  // init done

  // Clear the display buffer.
  display.clearDisplay();
  display.display();

  display.setTextColor(WHITE, BLACK);
  display.drawRect(117, 55, 3, 3, WHITE);     // Put degree symbol ( ° )
  draw_text(0, 55, "TEMPERATURE =", 1);
  draw_text(122, 55, "C", 1);
}


// True when within the alarm period
// Currently alarms for 10 seconds
bool timeToAlarm() {
  for (int i = 0; i < alarmCount; i++ ) {
    if (Time[0] == alarmTimes[i].time[0] && Time[1] == alarmTimes[i].time[1] && 
        Time[3] == alarmTimes[i].time[3] && Time[4] == alarmTimes[i].time[4] && 
        Time[6] == alarmTimes[i].time[6] && ( (1u << day) & alarmTimes[i].days ) ) {
          return true;
    }
  }
  return false;
}


void displayDayText() {
  switch (day) {
    case 1:  draw_text(0, 0, " SUNDAY  ", 1); break;
    case 2:  draw_text(0, 0, " MONDAY  ", 1); break;
    case 3:  draw_text(0, 0, " TUESDAY ", 1); break;
    case 4:  draw_text(0, 0, "WEDNESDAY", 1); break;
    case 5:  draw_text(0, 0, "THURSDAY ", 1); break;
    case 6:  draw_text(0, 0, " FRIDAY  ", 1); break;
    default: draw_text(0, 0, "SATURDAY ", 1);
  }
}

byte bcdToDecimal(byte val) {
  return (second >> 4) * 10 + (second & 0x0F);
}

void DS3231_display() {
  // Convert BCD to decimal
  second = (second >> 4) * 10 + (second & 0x0F);
  minute = (minute >> 4) * 10 + (minute & 0x0F);
  hour   = (hour >> 4)   * 10 + (hour & 0x0F);
  date   = (date >> 4)   * 10 + (date & 0x0F);
  month  = (month >> 4)  * 10 + (month & 0x0F);
  year   = (year >> 4)   * 10 + (year & 0x0F);
  // End conversion

  Time[7]     = second % 10 + 48;
  Time[6]     = second / 10 + 48;
  Time[4]     = minute % 10 + 48;
  Time[3]     = minute / 10 + 48;
  Time[1]     = hour   % 10 + 48;
  Time[0]     = hour   / 10 + 48;
  Calendar[9] = year   % 10 + 48;
  Calendar[8] = year   / 10 + 48;
  Calendar[4] = month  % 10 + 48;
  Calendar[3] = month  / 10 + 48;
  Calendar[1] = date   % 10 + 48;
  Calendar[0] = date   / 10 + 48;
  if (temperature_msb < 0) {
    temperature_msb = abs(temperature_msb);
    temperature[0] = '-';
  }
  else
    temperature[0] = ' ';
  temperature_lsb >>= 6;
  temperature[2] = temperature_msb % 10  + 48;
  temperature[1] = temperature_msb / 10  + 48;
  if (temperature_lsb == 0 || temperature_lsb == 2) {
    temperature[5] = '0';
    if (temperature_lsb == 0) temperature[4] = '0';
    else                     temperature[4] = '5';
  }
  if (temperature_lsb == 1 || temperature_lsb == 3) {
    temperature[5] = '5';
    if (temperature_lsb == 1) temperature[4] = '2';
    else                     temperature[4] = '7';
  }

  draw_text(60,  0, Calendar, 1);                     // Display the date (format: dd/mm/yyyy)
  draw_text(10, 24, Time, 2);                         // Display the time
  draw_text(75, 55, temperature, 1);                  // Display the temperature

}

void soundAlarm() {
  if (timeToAlarm())  {
    digitalWrite(buzzer, LOW);
  }
  else {
    digitalWrite(buzzer, HIGH);
  }
}

void blink_parameter() {
  byte j = 0;
  while (j < 10 && digitalRead(button1) && digitalRead(button2)) {
    j++;
    delay(25);
  }
}

byte edit(byte x_pos, byte y_pos, byte parameter) {
  char text[3];
  sprintf(text, "%02u", parameter);
  while (!digitalRead(button1));                     // Wait until button B1 released
  while (true) {
    while (!digitalRead(button2)) {                  // If button B2 is pressed
      parameter++;
      if (i == 0 && parameter > 31)                  // If date > 31 ==> date = 1
        parameter = 1;
      if (i == 1 && parameter > 12)                  // If month > 12 ==> month = 1
        parameter = 1;
      if (i == 2 && parameter > 99)                  // If year > 99 ==> year = 0
        parameter = 0;
      if (i == 3 && parameter > 23)                  // If hours > 23 ==> hours = 0
        parameter = 0;
      if (i == 4 && parameter > 59)                  // If minutes > 59 ==> minutes = 0
        parameter = 0;
      sprintf(text, "%02u", parameter);
      draw_text(x_pos, y_pos, text, 1);
      delay(200);                                    // Wait 200ms
    }
    draw_text(x_pos, y_pos, "  ", 1);
    blink_parameter();
    draw_text(x_pos, y_pos, text, 1);
    blink_parameter();
    if (!digitalRead(button1)) {                     // If button B1 is pressed
      i++;                                           // Increament 'i' for the next parameter
      return parameter;                              // Return parameter value and exit
    }
  }
}

void draw_text(byte x_pos, byte y_pos, char *text, byte text_size) {
  display.setCursor(x_pos, y_pos);
  display.setTextSize(text_size);
  display.print(text);
  display.display();
}

void readFromClock() {
  Wire.beginTransmission(DS3231_ADDR);                 // Start I2C protocol with DS3231 address
  Wire.write(0);                                // Send register address
  Wire.endTransmission(false);                  // I2C restart
  Wire.requestFrom(DS3231_ADDR, 7);                    // Request 7 bytes from DS3231 and release I2C bus at end of reading
  second = Wire.read();                         // Read seconds from register 0
  minute = Wire.read();                         // Read minuts from register 1
  hour   = Wire.read();                         // Read hour from register 2
  day    = Wire.read();                         // Read day from register 3
  date   = Wire.read();                         // Read date from register 4
  month  = Wire.read();                         // Read month from register 5
  year   = Wire.read();                         // Read year from register 6
  Wire.beginTransmission(DS3231_ADDR);                 // Start I2C protocol with DS3231 address
  Wire.write(0x11);                             // Send register address
  Wire.endTransmission(false);                  // I2C restart
  Wire.requestFrom(DS3231_ADDR, 2);                    // Request 2 bytes from DS3231 and release I2C bus at end of reading
  temperature_msb = Wire.read();                // Read temperature MSB
  temperature_lsb = Wire.read();                // Read temperature LSB
}

void updateClockWithTimeAndDate() {
  // Write data to DS3231 RTC
  Wire.beginTransmission(DS3231_ADDR);               // Start I2C protocol with DS3231 address
  Wire.write(0);                              // Send register address
  Wire.write(0);                              // Reset sesonds and start oscillator
  Wire.write(minute);                         // Write minute
  Wire.write(hour);                           // Write hour
  Wire.write(day);                            // Write day
  Wire.write(date);                           // Write date
  Wire.write(month);                          // Write month
  Wire.write(year);                           // Write year
  Wire.endTransmission();                     // Stop transmission and release the I2C bus
}

void loop() {

  if (!digitalRead(button1)) {                       // If button B1 is pressed
    i = 0;
    while (!digitalRead(button1));                   // Wait for button B1 release
    while (true) {
      while (!digitalRead(button2)) {                // While button B2 pressed
        day++;                                       // Increment day
        if (day > 7) day = 1;
        displayDayText();                               // Call display_day function
        delay(200);                                  // Wait 200 ms
      }
      draw_text(0, 0, "         ", 1);
      blink_parameter();                             // Call blink_parameter function
      displayDayText();                                 // Call display_day function
      blink_parameter();                             // Call blink_parameter function
      if (!digitalRead(button1))                     // If button B1 is pressed
        break;
    }
    //set position of text when editing on button press
    date   = edit(60, 0, date);                      // Edit date
    month  = edit(80, 0, month);                    // Edit month
    year   = edit(110, 0, year);                   // Edit year
    hour   = edit(14, 9, hour);                     // Edit hours
    minute = edit(50, 9, minute);                   // Edit minutes

    // Convert decimal to BCD
    minute = ((minute / 10) << 4) + (minute % 10);
    hour = ((hour / 10)  << 4) + (hour % 10);
    date = ((date / 10) <<  4) + (date % 10);
    month = ((month / 10)  << 4) + (month % 10);
    year = ((year / 10)  << 4) + (year % 10);
    // End conversion

    updateClockWithTimeAndDate();
    delay(200);                                 // Wait 200ms
  }

  readFromClock();                                 // read date and time
  displayDayText();
  DS3231_display();                             // Display time & calendar
  soundAlarm();
  delay(50);                                    // Wait 50ms
}
