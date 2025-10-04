
#include <TinyGPSPlus.h>
#include <mpx_msb.h>
using namespace MPX;

MpxSimple mpx;
TinyGPSPlus gps;

volatile float g_speed=0,g_course=0,g_alt=0,g_dist=0,g_vario=0;
double lastLat=0,lastLng=0; bool haveLast=false;
uint32_t lastAltMs=0; double lastAlt=0;

static double rad(double d){ return d*3.14159265358979323846/180.0; }
static double hav(double lat1,double lon1,double lat2,double lon2){
  const double R=6371000.0;
  double dLat=rad(lat2-lat1), dLon=rad(lon2-lon1);
  double a=sin(dLat/2)*sin(dLat/2)+cos(rad(lat1))*cos(rad(lat2))*sin(dLon/2)*sin(dLon/2);
  return R*2*atan2(sqrt(a), sqrt(1.0-a));
}
float provSpeed(){ return g_speed; }
float provCourse(){ return g_course; }
float provAlt(){ return g_alt; }
float provDist(){ return g_dist; }
float provVario(){ return g_vario; }

void setup(){
  Serial.begin(115200); while(!Serial && millis()<2000){}
  mpx.begin(Serial3,38400);
  MPX::MpxMsb &raw = mpx.raw();
  raw.addGenericChannel(4,  MPX_SPEED,  &provSpeed,  10.0f);
  raw.addGenericChannel(7,  MPX_DIR,    &provCourse, 10.0f);
  raw.addGenericChannel(8,  MPX_HEIGHT, &provAlt,     1.0f);
  raw.addGenericChannel(13, MPX_DIST,   &provDist,   10.0f);
  raw.addGenericChannel(3,  MPX_VSPEED, &provVario,  10.0f);

  mpx.setEchoMasking(true);
  Serial2.begin(9600);  // u-blox in NMEA
}

void loop(){
  while(Serial2.available()) gps.encode(Serial2.read());

  if (gps.location.isUpdated()){
    double lat=gps.location.lat(), lng=gps.location.lng();
    if (haveLast){ double d = hav(lastLat,lastLng,lat,lng); g_dist += d/1000.0f; }
    lastLat=lat; lastLng=lng; haveLast=true;
  }
  if (gps.speed.isUpdated())  g_speed = gps.speed.kmph();
  if (gps.course.isUpdated()) g_course= gps.course.deg();
  if (gps.altitude.isUpdated()){
    g_alt = gps.altitude.meters();
    uint32_t now=millis(); if (lastAltMs){ double dt=(now-lastAltMs)/1000.0; if(dt>0) g_vario=(g_alt-lastAlt)/dt; }
    lastAltMs=now; lastAlt=g_alt;
  }
  mpx.poll();
}
