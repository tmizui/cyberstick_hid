/*
 * Arduino Leonardo Joystick (公式のものはバグあり、以下を利用)
 * https://github.com/Cryocrat/LeonardoJoystick
 * IDE Version 1.6.3_r5
DSUB9P  --  arduino (uno) / (micro pro)
  1     --    d4          /  d4
  2     --    d5          /  d5
  3     --    d6          /  d6
  4     --    d7          /  d7
  5     --   Vcc(+5V)     /  Vcc(+5V)
  6     --    d2          /  d2 (not use)
  7     --    d3          /  d3 (interruput 0)
  8     --    d8          /  d8 (strobe)
  9     --   GND          /  GND
*/
JoyState_t joySt;
#define LOOP_INTERVAL 16  // 入力周期(msec)
#define RCVSIZE  12
#define AXIS_X  0  // 操縦桿左右 (左00H～右FFH)
#define AXIS_Y  1  // 操縦桿前後 (奥00H～手前FFH)
#define AXIS_Z  2  // スロットル (奥00H～手前FFH)
#define AXIS_W  3  // 予備、フットペダル用
#define BTN_A   4  // ボタン類１
#define BTN_END  5  // 最大バッファ
#define CENTER_PIVOT 127    // 中心値
#define BTN_D   0x10;   // スロットル D ボタン
#define BTN_E1  0x08;   // スロットル E1 ボタン
#define BTN_E2  0x04;   // スロットル E2 ボタン
#define DBUS0   4
#define DBUS1   5
#define DBUS2   6
#define DBUS3   7
#define STROBE  8
#define BUSY    3
byte data[BTN_END] ;
unsigned long loopTimer ;
volatile byte rcvdata[RCVSIZE] ;
volatile byte rcvcnt ;

void setup()
{
// Serial.begin(115200) ;
  pinMode(DBUS0,INPUT_PULLUP) ;
  pinMode(DBUS1,INPUT_PULLUP) ;
  pinMode(DBUS2,INPUT_PULLUP) ;
  pinMode(DBUS3,INPUT_PULLUP) ;
  pinMode(BUSY,INPUT_PULLUP) ; // for interruput 1
  pinMode(STROBE,OUTPUT) ;
  digitalWrite(STROBE,HIGH);
  
	joySt.xAxis = CENTER_PIVOT;
	joySt.yAxis = CENTER_PIVOT;
	joySt.zAxis = CENTER_PIVOT;
	joySt.xRotAxis = CENTER_PIVOT;
	joySt.yRotAxis = CENTER_PIVOT;
	joySt.zRotAxis = CENTER_PIVOT;
	joySt.throttle = CENTER_PIVOT;
	joySt.rudder = CENTER_PIVOT;
	joySt.hatSw1 = 255;
	joySt.hatSw2 = 255;
	joySt.buttons = 0;

  // CyberStick の仕様(データの取り込みタイミングをアタッチ)
  attachInterrupt(/*割り込みチャンネル*/ 1, /*割り込み時の処理関数*/ riseACK, /*割り込み条件*/ FALLING) ;
}

int ck;

void loop()
{
  loopTimer = millis();

  if (ck==0) {
    PORTB &= 0b11111110;
    ck=1;
  } else {
    PORTB |= 0b00000001;
    ck=0;
  }
  // CyberStick からデータ読み込み
  getCyberStickStatus() ;
  // -----------------------

  // アナログ入力 ----------
  joySt.xAxis = data[AXIS_X];
  joySt.yAxis = data[AXIS_Y];
  joySt.throttle = data[AXIS_Z];
	// joySt.zAxis = analogRead(0) >> 2;
  joySt.zAxis = CENTER_PIVOT;
	joySt.xRotAxis = CENTER_PIVOT;
	joySt.yRotAxis = CENTER_PIVOT;
  joySt.zRotAxis = CENTER_PIVOT;
  //joySt.xRotAxis = analogRead(1) >> 2;
  //joySt.yRotAxis = analogRead(2) >> 2;
  //joySt.zRotAxis = analogRead(3) >> 2;
	joySt.rudder = CENTER_PIVOT;
  // -----------------------

  // ボタン入力 ------------
  joySt.buttons = data[BTN_A];
  // -----------------------

  // ハットスイッチ --------
	// joySt.buttons <<= 1;
	// if (joySt.buttons == 0)
	// 	joySt.buttons = 1;
	// joySt.hatSw1++;
	// joySt.hatSw2--;
	// if (joySt.hatSw1 > 8)
	//	joySt.hatSw1 = 0;
	//if (joySt.hatSw2 > 8)
	//	joySt.hatSw2 = 8;
  // ----------------------

  // Serial.println(data[BTN_A]);
	delay(LOOP_INTERVAL);

	// HID デバイスへの出力に反映させる
	Joystick.setState(&joySt);
  
}

// CYBERSTICKのハードウェアデータ取り込み
void getCyberStickStatus(){
  rcvcnt = 0 ;
  // PORTB &= 0b11111110 ;  // d8(REQ) LOW 、CyberStick がデータを吐き出す (for UNO)
   PORTB &= 0b11101111 ;     // d8(REQ) LOW 、CyberStick がデータを吐き出す (for micro pro = PB4)
  //digitalWrite(STROBE,LOW);
  while (rcvcnt<RCVSIZE) {
    if (loopTimer < millis() + LOOP_INTERVAL) {
      digitalWrite(STROBE,HIGH);
      break;  // タイムアウト
    }
    if (rcvcnt==1) {
    //  PORTB |= 0b00000001 ;  // d8(REQ) HIGH (for UNO)
       PORTB |= 0b00010000 ;  // d8(REQ) HIGH (for micro pro)
    //    digitalWrite(STROBE,HIGH);
    }
  }
  // ボタンデータを1バイトに集約
  for (byte i=0 ; i<BTN_END ; i++) data[i] = CENTER_PIVOT ;
  data[BTN_A] = rcvdata[0] & 0xf0 | ((rcvdata[1]>>4) & 0x0f) ;
  data[BTN_A] = 0xff - data[BTN_A] ;  // 逆倫理
  // サイクリックレバー X軸
  data[AXIS_X] = rcvdata[3] & 0xf0 | ((rcvdata[7]>>4) & 0x0f) ;  // X軸(サイバースティック)
  // サイクリックレバー Y軸
  data[AXIS_Y] = rcvdata[2] & 0xf0 | ((rcvdata[6]>>4) & 0x0f) ;  // Y軸(サイバースティック)
  // スロットル
  data[AXIS_Z] = rcvdata[4] & 0xf0 | ((rcvdata[8]>>4) & 0x0f) ;  // スロットル絶対出力(サイバースティック)
  // data[AXIS_Z] = 0xff - data[AXIS_Z] ;  // リバース
}
// サイバースティックのクロックデータの取得
// 
void riseACK() {
//  rcvdata[rcvcnt++] = PIND ;  // uno のポート単位読み込み (PD d4～d7)
    byte retByte = 0;           // micro pro のポート単位読み込み
    retByte |= (PIND & 0b00010000) ;
    retByte |= (PINC & 0b01000000) >> 1;
    retByte |= (PIND & 0b10000000) >> 1;
    retByte |= (PINE & 0b01000000) << 1;
    rcvdata[rcvcnt++] = retByte;
}
