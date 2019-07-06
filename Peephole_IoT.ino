#include "ESPino32CAM.h"
#include "ESPino32CAM_LineNotify.h"
const char* ssid     = ""; 
const char* password = "";
String token = "";
ESPino32CAM cam;
LineNotify line;
static mtmn_config_t mtmn_config = {0};


void setup() {
  Serial.begin(2000000);
  Serial.println("\r\nESPino32CAM");
  if (cam.init() != ESP_OK)
  {
    cam.printDebug(F("Init Fail"));
    while (1);
  }
  sensor_t *s = cam.sensor();
  s->set_framesize(s, FRAMESIZE_QVGA);
  s->set_vflip(s,1);
  mtmn_config.min_face = 80;
  mtmn_config.pyramid = 0.7;
  mtmn_config.p_threshold.score = 0.6;
  mtmn_config.p_threshold.nms = 0.7;
  mtmn_config.r_threshold.score = 0.7;
  mtmn_config.r_threshold.nms = 0.7;
  mtmn_config.r_threshold.candidate_number = 4;
  mtmn_config.o_threshold.score = 0.7;
  mtmn_config.o_threshold.nms = 0.4;
  mtmn_config.o_threshold.candidate_number = 2;
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\r\nConnected");
  line.authenKey(token);
  line.setUTC(7);
}

void loop()
{
  camera_fb_t *fb = cam.capture();
  String msg;
  if (!fb)
  {
    cam.printDebug("Camera capture failed");
    return;
  }
  dl_matrix3du_t *rgb888;
  if (cam.jpg2rgb(fb,&rgb888))
  {
    cam.clearMemory(fb);
    box_array_t *net_boxes = cam.faceDetect(rgb888, &mtmn_config);
    if (net_boxes)
    {
      cam.printDebug("");
      cam.printfDebug("Found %d face",net_boxes->len);
      int x, y, w, h, i;
      for (i = 0; i < net_boxes->len; i++)
      {
        x = (int)net_boxes->box[i].box_p[0];
        y = (int)net_boxes->box[i].box_p[1];
        w = (int)net_boxes->box[i].box_p[2] - x + 1;
        h = (int)net_boxes->box[i].box_p[3] - y + 1;
        cam.printfDebug("Face:%d\r\nOrigin: (x:%d,y:%d)\r\nWide: %d\r\nHigh: %d",i,x,y,w,h);
      }
      cam.drawFaceBoxes(rgb888, net_boxes, true);

    lineResp ret;
    msg = "มีใครบางคนอยู่หน้าห้อง!";
    Serial.println("Send Message,Sticker and Image: "+msg);
    Serial.println("Capture");
    ret = line.send(msg,1,17,fb->buf, fb->len);
    if(ret.status)
    {
      Serial.printf("API Called %d/%d per hour\r\n",ret.remaining,ret.limit);
      Serial.printf("Send Image %d/%d per hour\r\n",ret.imageremaining,ret.imagelimit);
      Serial.printf("The time when the limit is reset %d(UTC epoch seconds) , ",ret.reset);
      Serial.println(ret.reset_time);
    }
    else
      Serial.println(ret.response);
   Serial.println();
      
      cam.clearMemory(net_boxes);
    }

  }
  uint8_t *jpgout;
  size_t  jpglen;
  if (cam.rgb2jpg(rgb888,&jpgout, &jpglen))
    Serial.write(jpgout, jpglen);
  cam.clearMemory(jpgout);
  cam.clearMemory(rgb888);
}
