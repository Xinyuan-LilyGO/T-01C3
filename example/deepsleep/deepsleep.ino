/*
T-01C3 Simple Deep Sleep

This code is under Public Domain License.

*/
#define TIMER_WAKEUP  
//#define GPIO2_WAKEUP  


#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time T-01C3 will go to sleep (in seconds) */

#define GREEN_LED 3

RTC_DATA_ATTR int bootCount = 0;

/*
Method to print the reason by which T-01C3
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_GPIO : Serial.println("Wakeup caused by external signal using GPIO"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup(){
  Serial.begin(115200);
  delay(1000); //Take some time to open up the Serial Monitor
  pinMode(GREEN_LED,OUTPUT);
  digitalWrite(GREEN_LED,HIGH);
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for T-01C3
  print_wakeup_reason();

 delay(3000);

#if defined(TIMER_WAKEUP)
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +" Seconds");

#elif defined(GPIO2_WAKEUP)
  esp_deep_sleep_enable_gpio_wakeup(1<<2,ESP_GPIO_WAKEUP_GPIO_HIGH);//GPIO2 WAKEUP
   Serial.println("Set GPIO2 wake up");
#endif

  Serial.println("Going to sleep now");
  Serial.flush(); 
  esp_deep_sleep_start();
  Serial.println("This will never be printed");
}

void loop(){
  //This is not going to be called
}


