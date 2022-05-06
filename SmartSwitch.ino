//完成温度阈值设定

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>               // 使用软串口，需要包含头文件
#include <Ticker.h>                       // 使用Ticker库，需要包含头文件
#include <ESP8266HTTPClient.h>            //微信推送
#include <DS1302.h>
SoftwareSerial mySerial(14,12);//RX=d5,TX=d6
DS1302 rtc(D3, D8, D7); //对应DS1302的RST,DAT,CLK
unsigned char USART_TX_BUF[8]={0x01,0x03,0x00,0x48,0x00,0x08,0xC4,0x1A};
Ticker timer1;                            // 创建一个定时器对象
Ticker timer2;                            // 创建一个定时器对象
//巴法云服务器地址默认即可
#define TCP_SERVER_ADDR "bemfa.com"
//服务器端口//TCP创客云端口8344//TCP设备云端口8340
#define TCP_SERVER_PORT "8344"

///****************需要修改的地方*****************///

////WIFI名称，区分大小写，不要写错
//#define DEFAULT_STASSID  "huawei"
//////WIFI密码
//#define DEFAULT_STAPSW "613613613"

////WIFI名称，区分大小写，不要写错
//#define DEFAULT_STASSID  "YN-ROBOT"
//////WIFI密码
//#define DEFAULT_STAPSW "405405405"


//WIFI名称，区分大小写，不要写错
#define DEFAULT_STASSID  "vivo"
////WIFI密码
#define DEFAULT_STAPSW "sbwhj111"

////用户私钥，可在控制台获取,修改为自己的UID
String UID = "f36a2bdced6a54d407bc29c3235a6c4e";
//主题名字，可在控制台新建
String TOPIC = "mytemp";
String TOPIC2  = "myswitch";  //用于插座控制的主题
String type = "2";                                           // 1表示是预警消息，2表示设备提醒消息
String device = "智能插座";                           // 设备名称
String msg1 = "温度超出阈值，请立刻检查，设备已断电";       //发送的消息
String msg2 = "电流超出阈值，请立刻检查，设备已断电";       //发送的消息
int delaytime = 0;                                          //为了防止被设备“骚扰”，可设置贤者时间，单位是秒，如果设置了该值，在该时间内不会发消息到微信，设置为0立即推送。
String ApiUrl = "http://api.bemfa.com/api/wechat/v1/";        //默认 api 网址
int count_bzw=0;
int count_switch=0;
const int SWITCH_Pin = D2;              //单片机LED引脚值，D2是NodeMcu引脚命名方式，其他esp8266型号将D2改为自己的引脚
const int kaiguan_Pin = D4;
const int BEEP_Pin = D1;     
///*********************************************///
//初始化电压电流电量等信息
String str1="";
String str2="";
String strv="";
String strc="";
String strp="";//功率
String strw="";//电能
String strpw="";//功率因数

String a1="";//电压的第1个参数
String b1="";//电压的第2个参数
String c1="";//电压的第3个参数
String d1="";//电压的第4个参数
String a="";//温度的第1个参数
String b="";//温度的第2个参数
String c="";//温度的第3个参数
String d="";//温度的第4个参数
String a2="";//电流的第1个参数
String b2="";//电流的第2个参数
String c2="";//电流的第3个参数
String d2="";//电流的第4个参数
String a3="";//功率的第1个参数
String b3="";//功率的第2个参数
String c3="";//功率的第3个参数
String d3="";//功率的第4个参数
String a4="";//电能的第1个参数
String b4="";//电能的第2个参数
String c4="";//电能的第3个参数
String d4="";//电能的第4个参数
String a5="";//功率因数的第1个参数
String b5="";//功率因数的第2个参数
String c5="";//功率因数的第3个参数
String d5="";//功率因数的第4个参数
long int cmd=0;//字符串转换为整形数据的过程变量
long int cmdv=0;//字符串转换为整形数据的过程变量
long int cmdc=0;//字符串转换为整形数据的过程变量
long int cmdp=0;//字符串转换为整形数据的过程变量
long int cmdw=0;//字符串转换为整形数据的过程变量
long int cmdpw=0;//字符串转换为整形数据的过程变量
float voltage;//电压变量
float current;//电流变量
float temp;//温度变量
float power;//功率变量
float power_consumption;//电量变量
float power_factor;//功率因数变量


String DS_time;
int DS_timeh;
int DS_timem;
int now_temp;
int now_current;
int now_power;

//设置上传速率2s（1s<=upDataTime<=60s）
#define upDataTime 2*1000


//最大字节数
#define MAX_PACKETSIZE 512

//#define KEEPALIVEATIME 30*1000
////tcp客户端相关初始化，默认即可



//tcp客户端相关初始化，默认即可
WiFiClient TCPclient;//初始化wifi客户端
HTTPClient http;  //初始化http
String TcpClient_Buff = "";
unsigned int TcpClient_BuffIndex = 0;
unsigned long TcpClient_preTick = 0;
unsigned long preHeartTick = 0;//心跳
unsigned long preTCPStartTick = 0;//连接
bool preTCPConnected = false;

int tempwarning_bzw=0;//温度报警标志位
int powerwarning_bzw=0;//功率报警标志位
int set_temper = 90; //APP设置更新的温度阈值
int set_current = 90; //APP设置更新的温度阈值
int set_power = 5000;//APP设置更新的功率阈值
String set_opentime ="25:25";//不默认为空
String set_closetime ="25:25";
int set_opentimeh=25;
int set_opentimem=25;
int set_closetimeh=25;
int set_closetimem=25;
char time_buf[50];
//相关函数初始化
//连接WIFI
void doWiFiTick();
void startSTA();

//TCP初始化连接
void doTCPClientTick();
void startTCPClient();
void sendtoTCPServer(String p);

//switch控制函数，具体函数内容见下方
void turnOnSwitch();
void turnOffSwitch();

//led状态
String my_led_status = "off";

//beep控制函数，具体函数内容见下方
void turnOnBeep();
void turnOffBeep();

//推送消息函数
void doHttpStick();
/*
  *发送数据到TCP服务器
 */
void sendtoTCPServer(String p){
  
  if (!TCPclient.connected()) 
  {
    Serial.println("Client is not readly");
    return;
  }
  TCPclient.print(p);
  Serial.println("[Send to TCPServer]:String");
  Serial.println(p);
}


/*
  *初始化和服务器建立连接
*/
void startTCPClient(){
  if(TCPclient.connect(TCP_SERVER_ADDR, atoi(TCP_SERVER_PORT))){
    Serial.print("\nConnected to server:");
    Serial.printf("%s:%d\r\n",TCP_SERVER_ADDR,atoi(TCP_SERVER_PORT));
    String tcpTemp="";  //初始化字符串
    tcpTemp = "cmd=1&uid="+UID+"&topic="+TOPIC2+"\r\n"; //构建订阅myswitch指令

    sendtoTCPServer(tcpTemp); //发送订阅指令 
    preTCPConnected = true;
    preHeartTick = millis();
    TCPclient.setNoDelay(true);
  }
  else{
    Serial.print("Failed connected to server:");
    Serial.println(TCP_SERVER_ADDR);
    TCPclient.stop();
    preTCPConnected = false;
  }
  preTCPStartTick = millis();
}


/*
  *检查数据，发送数据
*/
void doTCPClientTick(){
 //检查是否断开，断开后重连
   if(WiFi.status() != WL_CONNECTED) return;

  if (!TCPclient.connected()) {//断开重连

  if(preTCPConnected == true){

    preTCPConnected = false;
    preTCPStartTick = millis();
    Serial.println();
    Serial.println("TCP Client disconnected.");
    TCPclient.stop();
  }
  else if(millis() - preTCPStartTick > 1*1000)//重新连接
    startTCPClient();
  }
  else
  {
    if (TCPclient.available()) {//收数据
      char c =TCPclient.read();
      TcpClient_Buff +=c;
      TcpClient_BuffIndex++;
      TcpClient_preTick = millis();
      
      if(TcpClient_BuffIndex>=MAX_PACKETSIZE - 1){
        TcpClient_BuffIndex = MAX_PACKETSIZE-2;
        TcpClient_preTick = TcpClient_preTick - 200;
      }
      preHeartTick = millis();
    }
    if(millis() - preHeartTick >= upDataTime){//上传数据
      preHeartTick = millis();
      
      /*****************获取到的传感器数值*****************/
      //为了演示，定义了多种类型的数据，可根据自己传感器自行选择
      float data1 = voltage;
      float data2 = current;
      String data3 = "";
      float data4 = temp;
      float data5 = power;
      float data6 = power_consumption;
      float data7 = power_factor;
      String data8 = DS_time;
      int data9 = count_switch;
      int data10 = set_temper;
      int data11 = set_power;
      
       now_temp = temp; //实时温度
       now_current = current; //实时电流
       now_power = power; //实时功率
      if(digitalRead(SWITCH_Pin)==HIGH)
        {
         data3 = "on";
        }
      if(digitalRead(SWITCH_Pin)==LOW)
        {
         data3 = "off";
        }
     
      /*********************数据上传*******************/
      String upstr = "";
      upstr = "cmd=2&uid="+UID+"&topic="+TOPIC+"&msg=#"+data1+"#"+data2+"#"+data3+"#"+data4+"#"+data5+"#"+data6+"#"+data7+"#"+data8+"#"+data9+"#"+data10+"#"+data11+"#\r\n";
      sendtoTCPServer(upstr);
      upstr = "";
    }
  }
  if((TcpClient_Buff.length() >= 1) && (millis() - TcpClient_preTick>=200))
   {//data ready
    TCPclient.flush();
    Serial.println(TcpClient_Buff); //打印接收到的消息
    String getTopic = "";
    String getMsg = "";
    String getTemp = "";
    int tempIndex = 0; //赋一个temp位置初值
    
   
          //注意TcpClient_Buff只是个字符串，在上面开头做了初始化 String TcpClient_Buff = "";
          //此时会收到推送的指令，指令大概为 cmd=2&uid=xxx&topic=light002&msg=off  cmd=2&uid=xxx&topic=light002&msg=settemp#60
           if((TcpClient_Buff.indexOf("&msg=on") > 0)) {
              turnOnSwitch();
              //////字符串匹配，检测发了的字符串TcpClient_Buff里面是否包含&msg=off，如果有，则关闭开关
           }else if((TcpClient_Buff.indexOf("&msg=off") > 0)) {
            turnOffSwitch();
           } 
          int topicIndex = TcpClient_Buff.indexOf("&topic=")+7; //c语言字符串查找，查找&topic=位置，并移动7位，不懂的可百度c语言字符串查找
          int msgIndex = TcpClient_Buff.indexOf("&msg=");//c语言字符串查找，查找&msg=位置
          getTopic = TcpClient_Buff.substring(topicIndex,msgIndex);//c语言字符串截取，截取到topic,不懂的可百度c语言字符串截取
          getMsg = TcpClient_Buff.substring(msgIndex+5);//c语言字符串截取，截取到消息

          if(tempIndex = getMsg.indexOf("?")>3){//c语言字符串查找，查找?位置 如果带?则表明是设定温度
          getTemp = getMsg.substring(tempIndex+7);//c语言字符串截取，截取到温度信息//有bug，没有用到tempIndex=0
          Serial.println(getTemp);   //打印截取到的消息值
          set_temper=getTemp.toInt();
          Serial.printf("get_temper:%d\r\n",set_temper);   //打印截取到的消息值
          }

        

          if(tempIndex = getMsg.indexOf("!")>3){//c语言字符串查找，查找?位置 如果带?则表明是设定电流
          getTemp = getMsg.substring(tempIndex+10);//c语言字符串截取，截取到温度信息//有bug，没有用到tempIndex=0
          Serial.println(getTemp);   //打印截取到的消息值
          set_current=getTemp.toInt();
          Serial.printf("get_current:%d\r\n",set_current);   //打印截取到的消息值
          }
          
          if(tempIndex = getMsg.indexOf("*")>3){//c语言字符串查找，查找*位置 如果带*则表明是设定功率
          getTemp = getMsg.substring(tempIndex+8);//c语言字符串截取，截取到温度信息//有bug，没有用到tempIndex=0
          Serial.println(getTemp);   //打印截取到的消息值
          set_power=getTemp.toInt();
          Serial.printf("get_power:%d\r\n",set_power);   //打印截取到的消息值
          }

          if(tempIndex = getMsg.indexOf("$")>3){//c语言字符串查找，查找?位置 如果带$则表明是设定打开时间
          getTemp = getMsg.substring(tempIndex+11);//c语言字符串截取，截取到温度信息//有bug，没有用到tempIndex=0
          Serial.println(getTemp);   //打印截取到的消息值
          set_opentime=getTemp;
          int openIndex=getTemp.indexOf(":");
          String getopenh=getTemp.substring(0,openIndex);//可能为0
          set_opentimeh=getopenh.toInt();
          String getopenm=getTemp.substring(openIndex+1);
          set_opentimem=getopenm.toInt();
          Serial.printf("get_open:%d %d\r\n",set_opentimeh,set_opentimem);   //打印截取到的消息值
          }

          if(tempIndex = getMsg.indexOf("%")>3){//c语言字符串查找，查找%位置 如果带$则表明是设定关闭时间
          getTemp = getMsg.substring(tempIndex+12);//c语言字符串截取，截取到温度信息//有bug，没有用到tempIndex=0
          Serial.println(getTemp);   //打印截取到的消息值
          set_closetime=getTemp;
          int closeIndex=getTemp.indexOf(":");
          String getcloseh=getTemp.substring(0,closeIndex);//可能为0
          set_closetimeh=getcloseh.toInt();
          String getclosem=getTemp.substring(closeIndex+1);
          set_closetimem=getclosem.toInt();
          Serial.printf("get_close:%d %d\r\n",set_closetimeh,set_closetimem);   //打印截取到的消息值
          }
            
   TcpClient_Buff="";
   TcpClient_BuffIndex = 0;

  }
}

void startSTA(){
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(DEFAULT_STASSID, DEFAULT_STAPSW);

}



/**************************************************************************
                                 WIFI
***************************************************************************/
/*
  WiFiTick
  检查是否需要初始化WiFi
  检查WiFi是否连接上，若连接成功启动TCP Client
  控制指示灯
*/
void doWiFiTick(){
  static bool startSTAFlag = false;
  static bool taskStarted = false;
  static uint32_t lastWiFiCheckTick = 0;

  if (!startSTAFlag) {
    startSTAFlag = true;
    startSTA();
    Serial.printf("Heap size:%d\r\n", ESP.getFreeHeap());
  }

  //未连接1s重连
  if ( WiFi.status() != WL_CONNECTED ) {
    if (millis() - lastWiFiCheckTick > 1000) {
      lastWiFiCheckTick = millis();
    }
  }
  //连接成功建立
  else {
    if (taskStarted == false) {
      taskStarted = true;
      Serial.print("\r\nGet IP Address: ");
      Serial.println(WiFi.localIP());
      startTCPClient();
    }
  }
}

//打开开关
void turnOnSwitch(){
  Serial.println("Turn ON Switch");
  digitalWrite(SWITCH_Pin,HIGH);
}
//关闭开关
void turnOffSwitch(){
  Serial.println("Turn OFF Switch");
    digitalWrite(SWITCH_Pin,LOW);
}

//打开蜂鸣器
void turnOnBeep(){
  Serial.println("Turn ON Beep");
    digitalWrite(BEEP_Pin,HIGH);
}

//关闭蜂鸣器
void turnOffBeep(){
  Serial.println("Turn OFF Beep");
  digitalWrite(BEEP_Pin,LOW);
}

//******微信消息推送函数********//
void doHttpStick_temp(){  //微信消息推送函数
  String postData;
  //Post Data
  postData = "uid="+UID+"&type=" + type +"&time="+delaytime+"&device="+device+"&msg="+msg1;
  http.begin(TCPclient,ApiUrl);              //Specify request destination
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
  int httpCode = http.POST(postData);   //Send the request
  String payload = http.getString();    //Get the response payload
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  http.end();  //Close connection
  Serial.println("send success");  
  }

 void doHttpStick_power(){  //微信消息推送函数
  String postData;
  //Post Data
  postData = "uid="+UID+"&type=" + type +"&time="+delaytime+"&device="+device+"&msg="+msg2;
  http.begin(TCPclient,ApiUrl);              //Specify request destination
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");    //Specify content-type header
  int httpCode = http.POST(postData);   //Send the request
  String payload = http.getString();    //Get the response payload
  Serial.println(httpCode);   //Print HTTP return code
  Serial.println(payload);    //Print request response payload
  http.end();  //Close connection
  Serial.println("send success");  
  }
//=======================================================================
void initRTCTime(void)//初始化RTC时钟
{
  rtc.writeProtect(false); //关闭写保护
  rtc.halt(false); //清除时钟停止标志
  Time t(2022, 4, 22, 17, 25, 30, Time::kFriday); //新建时间对象 最后参数位星期数据，周日为1，周一为2以此类推
  rtc.time(t);//向DS1302设置时间数据
}

void printTime()//打印时间数据
{
  Time tim = rtc.time(); //从DS1302获取时间数据
  
  char buf[50];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           tim.yr, tim.mon, tim.date,
           tim.hr, tim.min, tim.sec);

  Serial.println(buf);
}

// 初始化，相当于main 函数
void setup() {
  Serial.begin(115200);
  mySerial.begin(4800);
  timer1.attach(2, timer1_cb);
  timer2.attach(1, timer2_cb);
  pinMode(SWITCH_Pin,OUTPUT);
  pinMode(BEEP_Pin,OUTPUT);  
  pinMode(kaiguan_Pin,INPUT);
  digitalWrite(SWITCH_Pin,LOW); //开关断开为low
  digitalWrite(BEEP_Pin,LOW); //蜂鸣器断开为低
  Serial.println("beginning.....");
  //initRTCTime();
}

//循环
void loop() {
  doWiFiTick();
  doTCPClientTick();
}

void timer1_cb() {
  int num = 0;
  mySerial.write(USART_TX_BUF,8);
  delay(500); 
  for(int i=0; i<=40; i++)
  {
    str2 = String(mySerial.read(),HEX);//将数据以16进制的形式存储进字符串str2
    str1 += str2;
    str1 += " ";
    if(i==3)  //电压
    {
      a1 = str2; 
    }
    if(i==4)
    {
      b1 = str2; 
    }
    if(i==5)
    {
      c1 = str2; 
    }
    if(i==6)
    {
      d1 = str2; 
    }

    if(i==7)//电流
    {
      a2 = str2; 
    }
    if(i==8)
    {
      b2 = str2; 
    }
    if(i==9)
    {
      c2 = str2; 
    }
    if(i==10)
    {
      d2 = str2; 
    }

    if(i==11)//功率
    {
      a3 = str2; 
    }
    if(i==12)
    {
      b3 = str2; 
    }
    if(i==13)
    {
      c3 = str2; 
    }
    if(i==14)
    {
      d3 = str2; 
    }

    if(i==15)//电能
    {
      a4 = str2; 
    }
    if(i==16)
    {
      b4 = str2; 
    }
    if(i==17)
    {
      c4 = str2; 
    }
    if(i==18)
    {
      d4 = str2; 
    }

    if(i==19)//功率因数
    {
      a5 = str2; 
    }
    if(i==20)
    {
      b5 = str2; 
    }
    if(i==21)
    {
      c5 = str2; 
    }
    if(i==22)
    {
      d5 = str2; 
    }

    
    if(i==27)
    {
      a = str2; 
    }
    if(i==28)
    {
      b = str2; 
    }
    if(i==29)
    {
      c = str2; 
    }
    if(i==30)
    {
      d = str2; 
    }
  }
  if(str1.charAt(0)=='1')//由于开始几次会得到乱码，首字符为1的时候才进行数据处理
  {
    
//    Serial.println(str1);
     str2=a+b+c+d;
     strv=a1+b1+c1+d1;
     strc=a2+b2+c2+d2;
     strp=a3+b3+c3+d3;
     strw=a4+b4+c4+d4;
     strpw=a5+b5+c5+d5;
     char charBuf[str2.length() + 1];//String转char*，char*转long，long转10进制数///temp
     char charBufv[strv.length() + 1];//String转char*，char*转long，long转10进制数
     char charBufc[strc.length() + 1];//String转char*，char*转long，long转10进制数
     char charBufp[strp.length() + 1];//String转char*，char*转long，long转10进制数
     char charBufw[strw.length() + 1];//String转char*，char*转long，long转10进制数
     char charBufpw[strpw.length() + 1];//String转char*，char*转long，long转10进制数
     str2.toCharArray(charBuf, str2.length() + 1) ;
     strv.toCharArray(charBufv, strv.length() + 1) ;
     strc.toCharArray(charBufc, strc.length() + 1) ;
     strp.toCharArray(charBufp, strp.length() + 1) ;
     strw.toCharArray(charBufw, strw.length() + 1) ;
     strpw.toCharArray(charBufpw, strpw.length() + 1) ;
     cmd = strtoul(charBuf, NULL, 16);
     cmdv = strtoul(charBufv, NULL, 16);
     cmdc = strtoul(charBufc, NULL, 16);
     cmdp = strtoul(charBufp, NULL, 16);
     cmdw = strtoul(charBufw, NULL, 16);
     cmdpw = strtoul(charBufpw, NULL, 16);
     temp = cmd *0.01;
     //if (temp <2) temp=temp*15;
     voltage = cmdv *0.0001;
     current = cmdc *0.0001;
     power = cmdp *0.0001;
     power_consumption = cmdw *0.0001;
     power_factor = cmdpw *0.001;
     now_temp = temp; //实时温度
     now_current = current; //实时温度
     now_power = power; //实时温度
     Serial.print("当前温度为：");Serial.print(temp);Serial.println("℃");
     Serial.print("当前电压为：");Serial.print(voltage);Serial.println("V");
//     Serial.print("当前电流为：");Serial.print(current);Serial.println("A");
//     Serial.print("当前功率为：");Serial.print(power);Serial.println("W");
//     Serial.print("当前电能为：");Serial.print(power_consumption);Serial.println("KWh");
//     Serial.print("当前功率因数为：");Serial.print(power_factor);
  }
  
  str1 = "";//字符串清空
}

void timer2_cb() 
{
   Time tim = rtc.time(); //从DS1302获取时间数据
//   snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
//           tim.yr, tim.mon, tim.date,
//           tim.hr, tim.min, tim.sec);
   DS_time=String(tim.hr)+":"+String(tim.min)+":"+String(tim.sec);
   DS_timeh=tim.hr;
   DS_timem=tim.min;
   Serial.print("当前时间为：");Serial.print(DS_time);
   Serial.print("设定打开时间为：");Serial.print(set_opentime);
   Serial.print("设定关闭时间为：");Serial.print(set_closetime);
    if(DS_timeh==set_opentimeh&&DS_timem==set_opentimem)
    {
       Serial.print("设定打开时间到");
       turnOnSwitch();
       set_opentimeh=25;
       set_opentimem=25;
       
    }

     if(DS_timeh==set_closetimeh&&DS_timem==set_closetimem)
    {
       Serial.print("设定关闭时间到");
       turnOffSwitch();
       set_closetimeh=25;
       set_closetimem=25;
    }
  if(now_temp>=set_temper||now_power>=set_power)
  {
    turnOnBeep();
    turnOffSwitch();
    
    if(now_temp>=set_temper)
    {
      Serial.print("warning: temperature high");
      if(tempwarning_bzw==0)
         { doHttpStick_temp();
           tempwarning_bzw==1;
         }          
     }
     if(now_power>=set_power){
        Serial.print("warning: power high");          
        if(powerwarning_bzw==0)
         { doHttpStick_power();
           powerwarning_bzw==1;
         }
       
        } 
    }else
         {
            turnOffBeep();     
            if(tempwarning_bzw==1&&now_temp<set_temper) tempwarning_bzw=0;
            if(powerwarning_bzw==1&&now_power<set_power) powerwarning_bzw=0;         
          }
     
  //检查开关次数
  if(digitalRead(SWITCH_Pin)==HIGH)
        {
         count_bzw=1;
        }
  if(digitalRead(SWITCH_Pin)==LOW&&count_bzw==1)
        {
         count_bzw=0;
         count_switch++;
        }
   
}
