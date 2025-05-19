//arduino来说这个算是小船的草图吧
#include "include.h"
#include "SerialServo.h"
#include <WiFi.h>
#include <WebServer.h>
#include "FastLED.h"

#define LEDS_NUM 64
#define DATA_PIN 6
#define LED_TYPE WS2812
#define COLOR_ORDER GRB

#define RX_PIN D7
#define TX_PIN D6
#define BAUD 115200

const char* ssid     = "wifissid";
const char* password = "wifipassword";   

uint8_t bright = 12;
CRGB leds[LEDS_NUM];

WebServer server(80);


//操控页
void handelIndex()
{
  String HTML=R"(
<html><head><title>My little ship</title></head><body>
<script>
function send_servo(i){
  const servo_url='./servo?';
  if (i===1){
    fetch(servo_url+'l=600');
  }
  if (i===2){
    fetch(servo_url+'l=-600');
  } 
  if (i===3){
    fetch(servo_url+'r=-600');
  }
  if (i===4){
    fetch(servo_url+'r=600');
  } 
}
function send_stop(){
  fetch('./stop');
}
</script>
<button onclick='send_servo(1)'>Left Forward</button><button onclick='send_servo(2)'>Left Back</button><button onclick='send_servo(3)'>Right Forward</button><button onclick='send_servo(4)'>Right Back</button><button onclick='send_stop()'>Stop</button>
</body></html>)"; 
  server.send(200,"text/html",HTML);
}

//开灯
void handelLedOn()
{
  //广播开灯指令
  LobotSerialServoLedSet(Serial1, ID_ALL,0);
  delay(100); 
  //http响应
  server.send(200,"text/html","LED:on");
}

//关灯
void handelLedOff()
{
  //广播关灯指令
  LobotSerialServoLedSet(Serial1, ID_ALL,1);
  delay(100);
  //http响应
  server.send(200,"text/html","LED:off");
}

//停转
void handelStop()
{
  //广播全部伺服电机停止
  LobotSerialServoSetMode(Serial1, ID_ALL,1, 0);
  //http返回
  server.send(200,"text/html","stop");
}

//运转指令
void handelServo()
{
  //接受两个参数,分别代码两个电机的运转状态
  String rs=server.arg("r");
  String ls=server.arg("l");
  //分别给对应的伺服电机发指令
  if (rs!=""){
    int16_t ri=rs.toInt(); 
    LobotSerialServoSetMode(Serial1, 1,1, ri);
    delay(50);
  }
  if (ls!=""){
    int16_t li=ls.toInt();
    LobotSerialServoSetMode(Serial1, 2,1, li);
  }
  //http响应
  server.send(200,"text/html","Servo r="+rs+",l="+ls);
}

//用灯来显示ip
void show_ip(){
    //通过灯显示出来(用数组保存ip,然后把下面改成双循环会更优雅?)然后这个Hex14是最低的激发值,小于该值不亮灯
    for(int j=0;j<4;j++){
      for(int i=0;i<8;i++){
        //WiFi.localIP()返回的是IPAddress类型的变量,它的实际结构其实是长度为4的byte数组
        if (WiFi.localIP()[j] & (1<<i)){
          //这个算法让灯看起来比较合理:序号小的对应低位的二进制数
          leds[j*8+7-i]=0x001400;
        }else{
          leds[j*8+7-i]=0x150015;
        }           
      }    
    }
    //显示出来     
    FastLED.show();   
}

void setup() {
  //初始化串口,舵机用的
  Serial1.begin(BAUD,SERIAL_8N1,RX_PIN,TX_PIN);
  delay(1000);
  //初始化rgb led
  delay(1000);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, LEDS_NUM);
  FastLED.setBrightness(bright);  
  delay(500);
  FastLED.clear();//全部关灯
  FastLED.show();
  delay(1000);
  //链接无线网络
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  delay(500);
  show_ip();
  delay(500);
  //注册链接与回调函数
  server.on("/", handelIndex);
  server.on("/ledon", handelLedOn);
  server.on("/ledoff", handelLedOff);
  server.on("/stop", handelStop);
  server.on("/servo", handelServo);//调用样例 /servo?r=550&l=-550
  //web服务开始
  server.begin(); 
 
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    server.handleClient();
  }else{
    //断网后不断闪灯
    for (int m=0;m<LEDS_NUM;m++){
      if (m<10){
        //这个Hex14是最低的激发值,小于该值不亮灯
        leds[m]=0x141414;
      }else if (m<20){
        leds[m]=0x242424;
      }else if(m<30){
        leds[m]=0x343434;
      }else if(m<40){
        leds[m]=0x444444;
      }else {
        //这下面的不发光
        leds[m]=0x131313;
      }
    }
    FastLED.show();
    delay(400);
    FastLED.clear();
    FastLED.show();
    delay(100);
  }      
}
