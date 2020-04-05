#include <M5StickC.h>
#include <WiFi.h>
#include "time.h"
#include "watchinvaders.h"

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

// Prototype
void enterIntro();
void enterWatch();



// DISPLAY
void clearBuffer(){
  memset((void *)sBuffer, 0, sizeof(sBuffer));
}

void setZeroBuffer(int x, int y, int w, int h){
  byte *p;
  byte c;
  for(int yy = 0; yy < h; yy++){
    for(int xx = 0; xx < w; xx++){
      p = &sBuffer[((y + yy)*WIDTH + x + xx)/8];
      c = (1 << (x+xx)%8);
      if(*p & c){
        *p ^= c;
      }
    }
  }
}

void drawBuffer(int x, int y, int w, int h, byte *in){
  byte *p, *q;
  byte c;
  for(int yy = 0; yy < h; yy++){
    for(int xx = 0; xx < w; xx++){
      p = in + xx + yy/8 * w;
      c = ( *p & (1<<(yy%8)) ) ? 1 : 0;
      q = &sBuffer[((y + yy)*WIDTH + x + xx)/8];
      *q |= (c << (x+xx)%8);
    }
  }
}

void dispBuffer(){
  M5.Lcd.drawXBitmap(4, 4, sBuffer, WIDTH, HEIGHT, TFT_WHITE, BLACK);
}


// GAME
int chkRange(int in, int min, int max){
  if( in < min ){return 0;}
  if( in > max ){return 0;}
  return -1;
}

void drawVaders(){
  T_VADER *v;
  for(int i = 0; i < sVaders.live; i++){
    v = &sVaders.mVader[i];
    if(v->sts != -1){
      drawBuffer(v->x, v->y, VADERW, VADERH, (byte *)&chrVader[v->sts*VADERW]);
    }
  }
}

void setVaders(){
  T_VADER *v;
  for(int i = 0; i < VADERSNUM; i++){
    v = &sVaders.mVader[i];
    v->sts = (i%2) ? 4:2;
    v->x = (i%VADERCOL)*(VADERW + VADERMARGIN);
    v->y = ((VADERRAW-1) - i/VADERCOL)*(VADERH + VADERMARGIN) + 8;
  }
  sVaders.live = VADERSNUM;
  sVaders.dir = 1;
  sVaders.current = 0;
  sVaders.turn = 0;
  sVaders.down = 0;
}

void destroyVader(){
  static int count = 0;
  T_VADER *v, *w;
  if(++count < 3){return;}
  count = 0;
  for(int i = 0; i < sVaders.live; i++){
    v = &sVaders.mVader[i];
    if(v->sts < 9){continue;}
    if(++v->sts < 12){continue;}
    int d = i;
    while(d < sVaders.live - 1){
      v = &sVaders.mVader[d];
      w = &sVaders.mVader[d+1];
      v->sts = w->sts;
      v->x = w->x;
      v->y = w->y;
      d++;
    }
    if(--sVaders.live < 1){
      // GAME WIN
      sMain.mode = MODE_WATCH;
    }
    if(sVaders.current >= sVaders.live){
      sVaders.current = 0;
    }
  }
}

void moveVaders(){
  T_VADER *v;
  v = &sVaders.mVader[sVaders.current];  
  if(v->sts >= 0){
    v->x += sVaders.dir;
    if(v->x <= 0 || v->x >= (WIDTH-VADERW)){sVaders.turn = 1;}
    if(sVaders.down && v->y < HEIGHT-SHIPH-VADERH){v->y += 4;}
  	if(v->sts < 9){
  		v->sts ^= 1;
  		if(v->y >= HEIGHT-SHIPH-VADERH){
        if(chkRange(v->x, sShip.x, sShip.x + SHIPW - 1) || chkRange(v->x + VADERW, sShip.x, sShip.x + SHIPW - 1)){
  			  //deadShip();
          sMain.mode = MODE_LOSE;
  			  return;
        }
  		}
  	}
  }
  if(++sVaders.current < sVaders.live){return;}
  sVaders.down = 0;
  if(sVaders.turn){
    sVaders.dir = -sVaders.dir;
    sVaders.turn = 0;
    sVaders.down = 1;
  }
  sVaders.current = 0;
}


void drawShip(){
  drawBuffer(sShip.x, HEIGHT-SHIPH, SHIPW, SHIPH, (byte *)chrShip);
}

void moveShip(){
  float pitch, roll, yaw;
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  float dx = 12 * sin(roll/180);
  if(sShip.realx + dx < 0){
      sShip.realx = 0;
  }
  else if(sShip.realx + dx > (WIDTH-SHIPW)){
      sShip.realx = WIDTH-SHIPW;
  }
  else{
      sShip.realx += dx; 
  }
  sShip.x = sShip.realx;
}

void launchShoot(){
  if(sShip.shootx >= 0){return;}
  float accX,accY,accZ;
  M5.IMU.getAccelData(&accX,&accY,&accZ);
  //M5.Lcd.setCursor(0, 45);
  //M5.Lcd.printf("%.2f   %.2f   %.2f      ",accX * 1000,accY * 1000, accZ * 1000);
  if(accX*1000 >= 1200 || digitalRead(M5_BUTTON_HOME) == LOW){
    //M5.Lcd.fillScreen(RED);
    //delay(500);
    sShip.shootx = sShip.x + (SHIPW/2);
    sShip.shooty = HEIGHT - SHIPH - 4;
  }
}


void chkShoot(){
  if(sShip.shootx < 0){return;}
  T_VADER *v;
  for( int i = 0; i < VADERSNUM; i++ ){
    v = &sVaders.mVader[i];
    if(v->sts < 0 || v->sts > 8){continue;}
    if(chkRange(sShip.shootx, v->x, v->x + VADERW) && chkRange(sShip.shooty, v->y, v->y + VADERH)){
        v->sts = 9;
        sShip.shootx = -1;
    }
  }
}

const byte chrShoot [] PROGMEM =  {0x0f,};
void drawShoot(){
  //M5.Lcd.setCursor(0, 45);
  //M5.Lcd.printf("shootx=%d",sShip.shootx);
  if(sShip.shootx < 0){return;}
  sShip.shooty -= 1;
  if(sShip.shooty < 0){sShip.shootx = -1;}
  drawBuffer(sShip.shootx, sShip.shooty, 1, 4, (byte *)chrShoot);
}

void initShip(){
  sShip.x = (WIDTH - SHIPW)/2;
  sShip.realx = (float)sShip.x;
  sShip.shootx = -1;
}

// Move mode
void enterIntro(){
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCentreString("WATCH INVADERS",80,15,2);
  M5.Lcd.drawCentreString("PUSH[A] START TO BEAT!",80,50,1);
  M5.Lcd.drawCentreString("PUSH[B] ADJUST BY NTP ",80,60,1);
  //M5.Lcd.setCursor(30,50);
  //M5.Lcd.println("PUSH[A] START TO BEAT!");
  //M5.Lcd.setCursor(30,60);
  //M5.Lcd.println("PUSH[B] NTP ADJUST");
  //clearBuffer();
  //M5.Lcd.drawXBitmap(16, 14, intro, 128, 55, TFT_WHITE, BLACK);
  sMain.entrtime = millis();
  sMain.release = 0;
  sMain.mode = MODE_INTRO;
}

void enterGame(){
  M5.Lcd.fillScreen(TFT_BLACK);
  setVaders();
  initShip();

  while(1){
    if(digitalRead(M5_BUTTON_HOME) ==HIGH && digitalRead(M5_BUTTON_RST) == HIGH){break;}
    delay(10);
  }
  sMain.mode = MODE_GAME;
}

// TIME ADJUST
void getNTP(){
  const char* ssid       = SSIDNAME;
  const char* password   = WIFIPASS;
  int count = 0;
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0,4);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println(" Adjust by NTP");
  M5.Lcd.println(" Wifi Conecting...");

#ifdef SMARTCONFIG
  WiFi.begin();
#else
  WiFi.begin(ssid, password);
#endif

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if(++count > 60){
        M5.Lcd.println("No Wifi");
        delay(1000);
        enterIntro();
        return;    
      }
  }
  M5.Lcd.println(" CONNECTED!");
  
  //init and get the time
  configTime(TIMEZONE * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) { 
    // Set RTC time
    RTC_TimeTypeDef TimeStruct;
    TimeStruct.Hours   = timeInfo.tm_hour;
    TimeStruct.Minutes = timeInfo.tm_min;
    TimeStruct.Seconds = timeInfo.tm_sec;
    M5.Rtc.SetTime(&TimeStruct);
 
    RTC_DateTypeDef DateStruct;
    DateStruct.WeekDay = timeInfo.tm_wday;
    DateStruct.Month = timeInfo.tm_mon + 1;
    DateStruct.Date = timeInfo.tm_mday;
    DateStruct.Year = timeInfo.tm_year + 1900;
    M5.Rtc.SetData(&DateStruct);
  }

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(2000);
  enterIntro();
}

void enterWatch(){
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCentreString("U Get the Time!",80,15,2);
  M5.Lcd.drawCentreString("PUSH[B] BACK TO TOP ",80,60,1);
  sMain.entrtime = millis();
  sMain.release = 0;
  sMain.mode = MODE_WATCH;
}

void deadShip(){
  for(int i = 9; i < 12; i++){
    setZeroBuffer(sShip.x, HEIGHT-SHIPH, SHIPW, SHIPH);
    drawBuffer(sShip.x, HEIGHT-SHIPH, SHIPW, SHIPH, (byte *)&chrVader[i*VADERW]);
    dispBuffer();
    delay(300);
  }
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCentreString("U MISS THE TIME...",80,30,2);
  //M5.Lcd.setCursor(20,45);
  //M5.Lcd.println("PRESS:");
  //M5.Lcd.setCursor(30,60);
  M5.Lcd.drawCentreString("PUSH[B] BACK TO TOP ",80,60,1);
  sMain.entrtime = millis();
  sMain.release = 0; 
}


void chkTimeOut(){
  ulong time = millis();
  if(time - sMain.entrtime > 1000*60 || time < sMain.entrtime){
    switch(sMain.mode){
      case MODE_INTRO:
        M5.Axp.ScreenBreath(0);
        esp_light_sleep_start();
        sMain.release = 0;
        M5.Axp.ScreenBreath(8);
        sMain.entrtime = millis();
        break;    // Sleep
      case MODE_WATCH:enterIntro();break;
      case MODE_LOSE:enterIntro();break;
    }
  }
}

void modeIntro(){
  chkTimeOut();
  if(!sMain.release){
  	if(digitalRead(M5_BUTTON_HOME) ==HIGH && digitalRead(M5_BUTTON_RST) == HIGH){sMain.release = 1;}
  	return;
  }
  if(digitalRead(M5_BUTTON_HOME) == LOW){
    enterGame();
  }//enterGame();}
  if(digitalRead(M5_BUTTON_RST) == LOW){getNTP();}
}

void modeGame(){
  moveShip();
  launchShoot();
  chkShoot();
  moveVaders();
  destroyVader();

  clearBuffer();
  drawVaders();
  drawShip();
  drawShoot();
  dispBuffer();

  /* Mode Chnage? */
  switch(sMain.mode){
    case MODE_WATCH:enterWatch();break;
    case MODE_LOSE:deadShip();break;
  }
}

void modeWatch(){
  chkTimeOut();
  if(!sMain.release){
  	if(digitalRead(M5_BUTTON_HOME) ==HIGH && digitalRead(M5_BUTTON_RST) == HIGH){sMain.release = 1;}
  	return;
  }
  M5.Rtc.GetTime(&RTC_TimeStruct);
  M5.Lcd.setCursor(35,35);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("%02d:%02d:%02d\n",RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds);
  if(digitalRead(M5_BUTTON_RST) == LOW){enterIntro();}
}

void modeLose(){
  chkTimeOut();
  if(!sMain.release){
  	if(digitalRead(M5_BUTTON_HOME) ==HIGH && digitalRead(M5_BUTTON_RST) == HIGH){sMain.release = 1;}
  	return;
  }
  if(digitalRead(M5_BUTTON_RST) == LOW){enterIntro();}
}

//
//  Arduino function
//
void setup() {
  // put your setup code here, to run once:
  M5.begin();
  M5.Axp.ScreenBreath(8);
  M5.IMU.Init();
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  
  M5.Lcd.setTextSize(1);
  //M5.Lcd.setCursor(40, 0, 2);
  //M5.Lcd.println("RTC TEST");
/***
  RTC_TimeTypeDef TimeStruct;
  TimeStruct.Hours   = 18;
  TimeStruct.Minutes = 56;
  TimeStruct.Seconds = 10;
  M5.Rtc.SetTime(&TimeStruct);
  RTC_DateTypeDef DateStruct;
  DateStruct.WeekDay = 3;
  DateStruct.Month = 3;
  DateStruct.Date = 22;
  DateStruct.Year = 2019;
  M5.Rtc.SetData(&DateStruct);
***/
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);

  gpio_wakeup_enable(GPIO_NUM_37, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable(GPIO_NUM_39, GPIO_INTR_LOW_LEVEL);

  // GPIOをウェイクアップソースとして有効にする
  esp_sleep_enable_gpio_wakeup();

  enterIntro();
}


void loop() {
  switch(sMain.mode){
  case MODE_INTRO:modeIntro();break;
  case MODE_GAME:modeGame();break;
  case MODE_WATCH:modeWatch();break;
  case MODE_LOSE:modeLose();break;
  }
  delay(10);
}
