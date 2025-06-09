//小船实验五,更合理的ip灯显示方式 2025-05-27
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

const char* ssid     = "WifiSsid";
const char* password = "WifiPassword";   

uint8_t bright = 12;
CRGB leds[LEDS_NUM];

WebServer server(80);


//操控页
void handelIndex()
{
  String HTML=R"(
<html>
<head>
<meta charset='UTF-8' name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, minimum-scale=1, user-scalable=no'>
<title>sailor</title>
<style>
body, html {overflow: hidden;}
  .fixed-top {position: fixed;top: 0;left: 0;right: 0;width: 100%;height: 30%;background-color: #f0f0f0;}
  .fixed-bottom {position: fixed;bottom: 0;left: 0;right: 0;width: 100%;height: 30%;background-color: #f0f0f0;}
  .btn-mid{position: absolute;top: 42%;left: 40%;text-align: center;line-height: 100px;font-size: 20px;transform: rotate(90deg);}
</style>
</head>
<body>
<div id='touchAreaL' class='fixed-top'></div>
<button class='btn-mid' onclick='send_stop()'>Stop My Ship</button>
<div id='touchAreaR' class='fixed-bottom'></div> 
<script type='app/worker' id='wrs'>
this.onmessage = function(res){
const servo_url='/servo?';
let servo_l = '';
let servo_r = ''; 
if (!(res.data.sdlx===null || res.data.sdlx===undefined)){
if(typeof(res.data.sdlx)==='number'){
if (res.data.sdlx>=-1000 && res.data.sdlx<=1000){
servo_l='l='+res.data.sdlx;}}}
if (!(res.data.sdrx===null || res.data.sdrx===undefined)){
if(typeof(res.data.sdrx)==='number'){
if (res.data.sdrx>=-1000 && res.data.sdrx<=1000){
if (servo_l===''){
servo_r='r='+res.data.sdrx;
}else{servo_r='&r='+res.data.sdrx;}}}}
fetch(res.data.p+'//'+res.data.h+servo_url+servo_l+servo_r);  
}
</script>
<script>
const touchAreaL = document.getElementById('touchAreaL');

var lx=0,rx=0;
var send_lx=0,send_rx=0;

touchAreaL.addEventListener('touchstart', function(event) {
event.preventDefault(); 
if (event.touches.length>=1){
if (event.touches[0].clientY<200){
lx=event.touches[0].clientX;}}
if (event.touches.length>=2){
if (event.touches[1].clientY<200){
lx=event.touches[1].clientX;}}});

touchAreaL.addEventListener('touchmove', function(event) {
event.preventDefault(); 
if (event.touches.length>=1){
if (event.touches[0].clientY<200){
lx=event.touches[0].clientX;}}
if (event.touches.length>=2){
if (event.touches[1].clientY<200){
lx=event.touches[1].clientX;}}});

touchAreaL.addEventListener('touchend', function(event) {
event.preventDefault();
});

const touchAreaR = document.getElementById('touchAreaR');

touchAreaR.addEventListener('touchstart', function(event) {
event.preventDefault();
if (event.touches.length>=1){
if (event.touches[0].clientY>500){
rx=event.touches[0].clientX;}}
if (event.touches.length>=2){
if (event.touches[1].clientY>500){
rx=event.touches[1].clientX;}}});

touchAreaR.addEventListener('touchmove', function(event) {
event.preventDefault();
if (event.touches.length>=1){
if (event.touches[0].clientY>500){
rx=event.touches[0].clientX;}}
if (event.touches.length>=2){
if (event.touches[1].clientY>500){
rx=event.touches[1].clientX;
}}});

touchAreaR.addEventListener('touchend', function(event) {
event.preventDefault();
});  

const wkblob = new Blob([document.querySelector('#wrs').textContent]);
const wkurl = window.URL.createObjectURL(wkblob);
const worker = new Worker(wkurl);

function send_value(){
let send_data={sdlx:0,sdrx:0,p:window.location.protocol,h:window.location.hostname};
let send_flg=false;
if (send_lx!=lx){
send_lx=lx;
send_data.sdlx=(send_lx-185)*5;
send_flg=true;
}else{delete send_data.sdlx;}
if (send_rx!=rx){
send_rx=rx;
send_data.sdrx=(185-send_rx)*5;
send_flg=true;
}else{delete send_data.sdrx;}
if (send_flg){worker.postMessage(send_data);}
}

window.onload = function() {setInterval(send_value, 100);};

function send_stop(){
lx=0;
rx=0;
send_lx=0;
send_rx=0;
fetch('./stop');}  
</script>
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

//用灯来显示ip,显示顺序按自然数字来192.168.72.121,31~24灯显示192,以此类推
void show_ip(){
    //rgb的数值Hex14是最低的激发值,小于该值不亮灯
    for(int j=0;j<4;j++){
      for(int i=0;i<8;i++){
        int led_index=(3-j)*8+i;
        if (WiFi.localIP()[j] & (1<<i)){
          leds[led_index]=0x001400;
        }else{
          leds[led_index]=0x150015;
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
    //断网后全条闪灯
    for (int m=0;m<LEDS_NUM;m++){
      leds[m]=0x202020;
    }
    FastLED.show();
    delay(400);
    FastLED.clear();
    FastLED.show();
    delay(100);
  }      
}
