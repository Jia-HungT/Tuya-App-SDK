#include <TuyaWifi.h>
#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

#define BUTTON_PIN   2

#define PIXEL_PIN    6  // Digital IO pin connected to the NeoPixels.

#define PIXEL_COUNT 30  // Number of NeoPixels

Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
TuyaWifi my_device;

boolean oldState = HIGH;
int     mode     = 0;    // Currently-active animation mode, 0-9
//备注:
#define DPID_SWITCH_LED 20
//模式(可下发可上报)
//备注:
#define DPID_WORK_MODE 21
//音乐灯(只下发)
//备注:类型：字符串；
//Value: 011112222333344445555；
//0：   变化方式，0表示直接输出，1表示渐变；
//1111：H（色度：0-360，0X0000-0X0168）；
//2222：S (饱和：0-1000, 0X0000-0X03E8）；
//3333：V (明度：0-1000，0X0000-0X03E8）；
//4444：白光亮度（0-1000）；
//5555：色温值（0-1000）
#define DPID_MUSIC_DATA 27

//炫彩情景(可下发可上报)
//备注:专门用于幻彩灯带场景
//Value：ABCDEFGHIJJKLLM...
//A：版本号；
//B：情景模式编号；
//C：变化方式（0-静态、1-渐变、2跳变、3呼吸、4-闪烁、10-流水、11-彩虹）
//D：单元切换间隔时间（0-100）;
//E：单元变化时间（0-100）；
//FGH：设置项；
//I：亮度（亮度V：0~100）；
//JJ：颜色1（色度H：0-360）；
//K：饱和度1 (饱和度S：0-100)；
//LL：颜色2（色度H：0-360）；
//M：饱和度2（饱和度S：0~100）；
//注：有多少个颜色单元就有多少组，最多支持20组；
//每个字母代表一个字节
#define DPID_DREAMLIGHT_SCENE_MODE 51

//点数/长度设置(可下发可上报)
//备注:幻彩灯带裁剪之后重新设置长度
#define DPID_LIGHTPIXEL_NUMBER_SET 53


#define DPIO_SCENE_QUIET_MODE 101
#define DPIO_SCENE_ACTIVE_MODE 103
#define DPIO_SCENE_DAZZLE_MODE 105

/* Stores all DPs and their types. PS: array[][0]:dpid, array[][1]:dp type. 
 *                                     dp type(TuyaDefs.h) : DP_TYPE_RAW, DP_TYPE_BOOL, DP_TYPE_VALUE, DP_TYPE_STRING, DP_TYPE_ENUM, DP_TYPE_BITMAP
*/
unsigned char dp_array[][2] = {
    {DPID_SWITCH_LED, DP_TYPE_BOOL},
    {DPID_LIGHTPIXEL_NUMBER_SET, DP_TYPE_VALUE},
    {DPID_WORK_MODE, DP_TYPE_ENUM},
    {DPID_MUSIC_DATA, DP_TYPE_STRING},
    {DPID_DREAMLIGHT_SCENE_MODE, DP_TYPE_RAW},
    {DPIO_SCENE_QUIET_MODE, DP_TYPE_ENUM},
    {DPIO_SCENE_ACTIVE_MODE, DP_TYPE_ENUM},
    {DPIO_SCENE_DAZZLE_MODE, DP_TYPE_ENUM},
};



unsigned char pid[] = {"s4ripf692wac6uie"};
unsigned char mcu_ver[] = {"1.0.0"};

/* last time */
unsigned long last_time = 0;

/* Current device DP values */
unsigned char led_state = 0;
unsigned char bool_switch_state = 0;
unsigned char work_mode_enum_state = 0;
unsigned char music_data_string_value[21]={"0"};
unsigned char DREAMLIGHT_SCENE_MODE[40] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
unsigned char scene_mode[3]={0,0,0};
unsigned char led_mode = 0;
long LIGHTPIXEL_NUMBER_value = 30;

/* Connect network button pin */
int key_pin = 7;



void setup() {
  // Serial.begin(9600);
  Serial.begin(9600);


  pinMode(BUTTON_PIN, INPUT_PULLUP);
  strip.begin(); // Initialize NeoPixel strip object (REQUIRED)
  strip.show();  // Initialize all pixels to 'off'
  
  //Initialize led port, turn off led.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(key_pin, INPUT_PULLUP);
  //Enter the PID and MCU software version
  my_device.init(pid, mcu_ver);
  //incoming all DPs and their types array, DP numbers
  my_device.set_dp_cmd_total(dp_array, 8);
  //register DP download processing callback function
  my_device.dp_process_func_register(dp_process);
  //register upload all DP callback function
  my_device.dp_update_all_func_register(dp_update_all);

  last_time = millis();
  
}
void loop() {
  my_device.uart_service();

  //Enter the connection network mode when Pin7 is pressed.
  if (digitalRead(key_pin) == LOW) {
    delay(80);
    if (digitalRead(key_pin) == LOW) {
      my_device.mcu_set_wifi_mode(SMART_CONFIG);
    }
  }
  /* LED blinks when network is being connected */
  if ((my_device.mcu_get_wifi_work_state() != WIFI_LOW_POWER) && (my_device.mcu_get_wifi_work_state() != WIFI_CONN_CLOUD) && (my_device.mcu_get_wifi_work_state() != WIFI_SATE_UNKNOW)) {
    if (millis()- last_time >= 500) {

      last_time = millis();

      if (led_state == LOW) {
        led_state = HIGH;
      } else {
        led_state = LOW;
      }
      digitalWrite(LED_BUILTIN, led_state);
    }
  }

  // Get current button state.
  boolean newState = digitalRead(BUTTON_PIN);

  /*colorWipe(strip.Color(255,   0,   0), 50);    // Red
  colorWipe(strip.Color(  0, 255,   0), 50);    // Green
  colorWipe(strip.Color(  0,   0, 255), 50);    // Blue
  theaterChase(strip.Color(127, 127, 127), 50); // White
  theaterChase(strip.Color(127,   0,   0), 50); // Red
  theaterChase(strip.Color(  0,   0, 127), 50); // Blue
  rainbow(10);
  theaterChaseRainbow(50);
  colorWipe(strip.Color(  0,   0,   0), 50);    // Black/off*/
  
  // Check if state changed from high to low (button press).
  if((newState == LOW) && (oldState == HIGH)) {
    // Short delay to debounce button.
    delay(20);
    // Check if button is still low after debounce.
    newState = digitalRead(BUTTON_PIN);
    if(newState == LOW) {      // Yes, still low
      if(++mode > 8) mode = 0; // Advance to next mode, wrap around after #8
      switch(mode) {           // Start the new animation...
        case 0:
          colorWipe(strip.Color(  0,   0,   0), 10);    // Black/off
          break;
        case 1:
          colorWipe(strip.Color(255,   0,   0), 10);    // Red
          break;
        case 2:
          colorWipe(strip.Color(  0, 255,   0), 10);    // Green
          break;
        case 3:
          colorWipe(strip.Color(  0,   0, 255), 10);    // Blue
          break;
        case 4:
          theaterChase(strip.Color(127, 127, 127), 10); // White
          break;
        case 5:
          theaterChase(strip.Color(127,   0,   0), 10); // Red
          break;
        case 6:
          theaterChase(strip.Color(  0,   0, 127), 10); // Blue
          break;
        case 7:
          rainbow(10);
          break;
        case 8:
          theaterChaseRainbow(50);
          break;
      }
    }
  }
  if(work_mode_enum_state==2)
    scene_mode_data_process();
    else led_mode = 0;
  // Set the last-read button state to the old state.
  oldState = newState;
  delay(10);

}

/**
 * @description: DP download callback function.
 * @param {unsigned char} dpid
 * @param {const unsigned char} value
 * @param {unsigned short} length
 * @return {unsigned char}
 */
unsigned char dp_process(unsigned char dpid, const unsigned char value[], unsigned short length)
{
    switch (dpid) {
        case DPID_SWITCH_LED:
            
            bool_switch_state = my_device.mcu_get_dp_download_data(dpid, value, length);
            my_device.mcu_dp_update(DPID_SWITCH_LED, bool_switch_state, 1);
            if (bool_switch_state) {
              //Turn on
              colorfill (strip.Color(  127, 127,  127)); //上一次状态
            } else {
              //Turn off
              colorfill (strip.Color(  0, 0,  0));
            }
            
        break;
            
        case DPID_LIGHTPIXEL_NUMBER_SET:
            LIGHTPIXEL_NUMBER_value = my_device.mcu_get_dp_download_data(DPID_LIGHTPIXEL_NUMBER_SET, value, length);        
            my_device.mcu_dp_update(DPID_LIGHTPIXEL_NUMBER_SET, LIGHTPIXEL_NUMBER_value, 1);
        break;

        case DPID_WORK_MODE:
        if(bool_switch_state)
          {
            work_mode_enum_state = my_device.mcu_get_dp_download_data(dpid, value, length);
            my_device.mcu_dp_update(DPID_WORK_MODE, work_mode_enum_state, 1);
            switch(work_mode_enum_state)
            {
              case 0:  
                colorWipe(strip.Color(  127,   127,   127), 5);
                colorWipe(strip.Color(  0,   0,   0), 5);
              break;
              case 1: 
                 colorWipe(strip.Color(255,   0,   0), 25);
                 colorWipe(strip.Color(0,   255,   0), 25);
                 colorWipe(strip.Color(0,   0,   255), 25);
                 colorWipe(strip.Color(127,   127,   127), 10);
                 theaterChase(strip.Color(183, 134, 236), 25);
                 theaterChase(strip.Color(79, 83, 238), 25); 
                 theaterChase(strip.Color(211, 103, 206), 25);
                 theaterChase(strip.Color(108, 201, 92), 25); 
                 theaterChase(strip.Color(82, 211, 211), 25);
                 theaterChase(strip.Color(166, 160, 218), 25); 
                 theaterChase(strip.Color(58, 63, 235), 25);
                 theaterChase(strip.Color(243,   135,   238), 25);
                 theaterChase(strip.Color(79,   197,   80), 25);
                 theaterChase(strip.Color(41,   225,   235), 25);
                 rainbow(10);
                 theaterChaseRainbow(50);
                 colorWipe(strip.Color(  0,   0,   0), 50);    // Black/off
              break;
              case 2:
                  colorWipe(strip.Color(  0,   127,   0), 5);//Green
                  colorWipe(strip.Color(  0,   0,   0), 5);    // Black/off
              break;
              case 3: 
                colorWipe(strip.Color(  0,   0,   127), 5);//Blue
                colorWipe(strip.Color(  0,   0,   0), 5);    // Black/off
              break;
            }
            
          }
        break;

        case DPID_MUSIC_DATA:
        if(bool_switch_state)
          {
            for (unsigned int i=0; i<length; i++) {
                music_data_string_value[i] = value[i];
            }
            my_device.mcu_dp_update(DPID_MUSIC_DATA, music_data_string_value, length);
          }
        break;

        case DPID_DREAMLIGHT_SCENE_MODE:
            my_device.mcu_dp_update(DPID_DREAMLIGHT_SCENE_MODE, DREAMLIGHT_SCENE_MODE, length);
        break;
        case DPIO_SCENE_QUIET_MODE:
        if(bool_switch_state)
          {
            scene_mode[0] = my_device.mcu_get_dp_download_data(dpid, value, length);
            switch(scene_mode[0])
            {
              case 0:led_mode = 'A';break;
              case 1:led_mode = 'E';break;
              case 2:led_mode = 'I';break;
              case 3:led_mode = 'M';break;
            }
            my_device.mcu_dp_update(DPIO_SCENE_QUIET_MODE, scene_mode[0], 1);
          }
        break;

        case DPIO_SCENE_ACTIVE_MODE:
        if(bool_switch_state)
          {
            scene_mode[0] = my_device.mcu_get_dp_download_data(dpid, value, length);
            switch(scene_mode[1])
            {
              case 0:led_mode = 'Q';break;
              case 1:led_mode = 'U';break;
              case 2:led_mode = 'Y';break;
              case 3:led_mode = 'c';break;
            }
            my_device.mcu_dp_update(DPIO_SCENE_ACTIVE_MODE, scene_mode[1], 1);
          }
        break;

        case DPIO_SCENE_DAZZLE_MODE:
        if(bool_switch_state)
          {
            scene_mode[2] = my_device.mcu_get_dp_download_data(dpid, value, length);
            switch(scene_mode[1])
            {
              case 0:led_mode = 'g';break;
              case 1:led_mode = 'k';break;
            }
            my_device.mcu_dp_update(DPIO_SCENE_DAZZLE_MODE, scene_mode[2], 1);
          }
        break;
    }
    
    return SUCCESS;
}
void dp_update_all(void)
{
    my_device.mcu_dp_update(DPID_SWITCH_LED, bool_switch_state, 1);
    my_device.mcu_dp_update(DPID_LIGHTPIXEL_NUMBER_SET, LIGHTPIXEL_NUMBER_value, 1);
    my_device.mcu_dp_update(DPID_WORK_MODE, work_mode_enum_state, 1);
    my_device.mcu_dp_update(DPID_MUSIC_DATA, music_data_string_value, (sizeof(music_data_string_value) / sizeof(music_data_string_value[0])));
    my_device.mcu_dp_update(DPID_DREAMLIGHT_SCENE_MODE, DREAMLIGHT_SCENE_MODE, (sizeof(DREAMLIGHT_SCENE_MODE) / sizeof(DREAMLIGHT_SCENE_MODE[0])));
    my_device.mcu_dp_update(DPIO_SCENE_QUIET_MODE, scene_mode[0], 1);
    my_device.mcu_dp_update(DPIO_SCENE_ACTIVE_MODE, scene_mode[1], 1);
    my_device.mcu_dp_update(DPIO_SCENE_DAZZLE_MODE, scene_mode[2], 1);
    
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

void rainbow(int wait) {
  // Hue of first pixel runs 3 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 3*65536. Adding 256 to firstPixelHue each time
  // means we'll make 3*65536/256 = 768 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 3*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}

//拓展
void colorfill(uint32_t color) {
 strip.fill(color,1,LIGHTPIXEL_NUMBER_value);
    strip.show();                          //  Update strip to match   
}

/*colorWipe(strip.Color(255,   0,   0), 50);    // Red
  colorWipe(strip.Color(  0, 255,   0), 50);    // Green
  colorWipe(strip.Color(  0,   0, 255), 50);    // Blue
  theaterChase(strip.Color(127, 127, 127), 50); // White
  theaterChase(strip.Color(127,   0,   0), 50); // Red
  theaterChase(strip.Color(  0,   0, 127), 50); // Blue
  rainbow(10);
  theaterChaseRainbow(50);
  colorWipe(strip.Color(  0,   0,   0), 50);    // Black/off*/
void scene_mode_data_process(void)
{
  switch(led_mode)
  {
    case 'A':colorWipe(strip.Color(255,   0,   0), 50);    // Red
    break;
    case 'E':colorWipe(strip.Color(0,   0,   255), 50);    // Blue
    break;
    case 'I':colorWipe(strip.Color(255,   128,   0), 50);   
    break;
    case 'M':colorWipe(strip.Color(192,   192,   192), 50);   
    break;
    case 'Q':theaterChase(strip.Color(183, 134, 236), 25); 
             theaterChase(strip.Color(79, 83, 238), 25); 
             theaterChase(strip.Color(211, 103, 206), 25);
    break;
    case 'U':theaterChase(strip.Color(108, 201, 92), 25); 
             theaterChase(strip.Color(82, 211, 211), 25);
    break;
    case 'Y':theaterChase(strip.Color(166, 160, 218), 25); 
             theaterChase(strip.Color(58, 63, 235), 25);
    break;
    case 'c':theaterChase(strip.Color(243,   135,   238), 25);
             theaterChase(strip.Color(79,   197,   80), 25);
             theaterChase(strip.Color(41,   225,   235), 25);
    break;
    case 'g':rainbow(10); 
    break;
    case 'k':theaterChaseRainbow(50);
    break;
    default:break;
  }
  led_mode = 0;
}
