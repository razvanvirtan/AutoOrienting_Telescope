#include "Wire.h"
#include <MPU6050_light.h>
#include <SoftwareSerial.h>

#define BUFSIZ  50

SoftwareSerial hc06(12,13);
MPU6050 mpu(Wire);

/* defining motor control connections */
const double oneStep = 0.112;
const int stepPin2 = 5, stepPin1 = 9;
const int dirPin2 = 6, dirPin1 = 10;
const int msPin2 = 7, msPin1 = 11;
int nStepsHorizontal = 0, nStepsVertical = 0;

/* defining location and time data */
double latitude, longitude;
int year, month, day, hour, minute;
double juliandate;
float thisday;
double timenow;

/* defining celestial objects data */
struct CelestialCoord {
  double right_ascension, declination;
};

struct HorizonCoord {
  double azimuth, altitude;
};

struct CelestialCoord targetCelestial, polarisCelestial;
struct HorizonCoord targetHorizon, polarisHorizon;
double gstresult;
double lstresult;

/* defining bluetooth communication buffer */
char buff[BUFSIZ];

void setup() {
  /* set motor pins */
  pinMode(stepPin1, OUTPUT);
  pinMode(dirPin1, OUTPUT);
  pinMode(msPin1, OUTPUT);
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  pinMode(msPin2, OUTPUT);

  digitalWrite(stepPin1, LOW);
  digitalWrite(dirPin1, LOW);
  digitalWrite(msPin1, LOW);
  digitalWrite(stepPin2, LOW);
  digitalWrite(dirPin2, LOW);
  digitalWrite(msPin2, LOW);

  /* initialize bluetooth interface */
  Serial.begin(9600);
  hc06.begin(9600);
  Serial.println("Bluetooth Activated");
  
  /* set polaris celestial coordinates (this is a reference point, so it is hardcoded) */
  polarisCelestial.right_ascension = 37.95454;
  polarisCelestial.declination = 89.26411;
}

void loop() {
  /* callibrate gyro sensor */
  Wire.begin();
  byte status = mpu.begin(1, 0);
  while (status != 0) { status = mpu.begin(1, 0); }
  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets();
  Serial.println("Gyro Calibrated.\n");

  /* read user data */
  readFromUser();

  /* compute local sideral time (LST) */
  timenow = (double) hour + (double)minute / 60.0;
  juliandate = julian();
  gstresult = utctogst();
  lstresult = gsttolst();

  /* compute horizon coordinates for target body and reference point */
  azalt(polarisCelestial, &polarisHorizon);
  azalt(targetCelestial, &targetHorizon);

  Serial.println("POLARIS:");
  Serial.println(polarisHorizon.azimuth);
  Serial.println(polarisHorizon.altitude);
  Serial.println("TARGET:");
  Serial.println(targetHorizon.azimuth);
  Serial.println(targetHorizon.altitude);

  /* find number of steps and direction for azimuth (horizontal) rotation */
  if (targetHorizon.azimuth > polarisHorizon.azimuth) {
    if (targetHorizon.azimuth > 180) {
      nStepsHorizontal = (polarisHorizon.azimuth + 360 - targetHorizon.azimuth) / oneStep;
      digitalWrite(dirPin1, HIGH);
    }
    else if (targetHorizon.azimuth < 180) {
      nStepsHorizontal = (targetHorizon.azimuth - polarisHorizon.azimuth) / oneStep;
      digitalWrite(dirPin1, LOW);
    }
    Serial.println("Azimut >");
  } else if (targetHorizon.azimuth < polarisHorizon.azimuth) {
    if (targetHorizon.azimuth > 180) {
      nStepsHorizontal = (polarisHorizon.azimuth - targetHorizon.azimuth) / oneStep;
      digitalWrite(dirPin1, HIGH);
    }
    else if (targetHorizon.azimuth < 180) {
       nStepsHorizontal = (targetHorizon.azimuth + 360 - polarisHorizon.azimuth) / oneStep;
       digitalWrite(dirPin1, LOW);
    }
    Serial.println("Azimut <");
  }

  /* find number of steps and direction for altitude (vertical) rotation */
  if (targetHorizon.altitude > polarisHorizon.altitude) {
    nStepsVertical = (targetHorizon.altitude - polarisHorizon.altitude) / oneStep;
    digitalWrite(dirPin2, LOW);
    Serial.println("Altitudine >");
  } else if (targetHorizon.altitude < polarisHorizon.altitude) {
    nStepsVertical = (polarisHorizon.altitude - targetHorizon.altitude) / oneStep;
    digitalWrite(dirPin2, HIGH);
    Serial.println("Altitudine <");
  }

  /* move vertical */
  int x = 0;
  double verticalRotation;
  int newnStepsVertical = nStepsVertical;
  digitalWrite(msPin2, HIGH);
  while (x < newnStepsVertical) {
    digitalWrite(stepPin2, HIGH);
    delay(50);
    mpu.update();
    digitalWrite(stepPin2, LOW);
    delay(50);
    mpu.update();
    x++;
    /* real time correction, using gyro input */
    if (x % 50 == 0) {
      x = 0;
      verticalRotation = abs(mpu.getAngleX());
      Serial.println(verticalRotation);
      newnStepsVertical = nStepsVertical - verticalRotation / oneStep;
    }
  }
  delay(100);

  /* move horizontal */
  x = 0;
  double horizontalRotation;
  int newnStepsHorizontal = nStepsHorizontal;
  digitalWrite(msPin1, HIGH);
  while (x < newnStepsHorizontal) {
    digitalWrite(stepPin1, HIGH);
    delay(50);
    mpu.update();
    digitalWrite(stepPin1, LOW);
    delay(50);
    mpu.update();
    x++;
    /* real time correction, using gyro input */
    if (x % 50 == 0) {
      x = 0;
      horizontalRotation = abs(mpu.getAngleZ());
      Serial.println(horizontalRotation);
      newnStepsHorizontal = nStepsHorizontal - horizontalRotation / oneStep;
    }
  }
  delay(1000); // Wait a second

  Serial.println("DONE");
}

/* translate celestial coordinates to horizon coordinates */
void azalt(struct CelestialCoord celestial, struct HorizonCoord *horizon){
  double ra = celestial.right_ascension / 15;
  double h = 15.0 * (lstresult - ra);
  h = (h / 360) * (2 * PI);
  double dec = ((celestial.declination / 360) * (2 * PI));
  double sindec = sin(dec);
  double sinlat = sin(latitude);
  double cosdec = cos(dec);
  double coslat = cos(latitude);
  double jeremy = cos(h);
  double sinalt = (sindec * sinlat) + (cosdec * coslat * jeremy);
  double alt = asin(sinalt);
  double cosalt = cos(alt);
  alt = (alt / (2 * PI)) * 360;
  double cosaz = (sindec - (sinlat * sinalt)) / (coslat * cosalt);
  double az = ((acos(cosaz)) * 4068) / 71;
  double sinhh = sin(h);
 
  if ((sinhh * -1) > 0) {
    az = az;
  }
  else
  {
    az = 360.0 - az;
  }

  if (alt < 0) {
    Serial.println("object is below the horizon!");
    return;
  }

  horizon->altitude = alt;
  horizon->azimuth = az;
}

/* calculate julian time */
float julian() { //For more info see 'Practical Astronomy With Your Calculator'.
  thisday = ((day - 1.0) + (timenow / 24.0));
  if (month == 1 || month == 2) {
    month = month + 12;
    year = year - 1;
  }
  int a = floor ((double)year / 100);
  int b = 2 - a + floor (a / 4);
  long c = (365.25 * (double) year);
  float d = floor (30.6001 * ((double) month + 1));
  float jd = b + c + d + (double)thisday + 1720994.5;
  return jd; //'jd' being the Julian Date.
}

/* translate UTC to GST */
float utctogst() {
  double s = juliandate - 2451545.0;
  double t = s / 36525.0;
  double step1 = (2400.051336 * t);
  double step2 = (t * t);
  double step3 = (0.000025862 * step2);
  double to = (6.697374558 + step1 + step3);
  double n1 = floor (to / 24);
  to = to - (n1 * 24);
  double h1 = (timenow * 1.002737909);
  double n2 = floor ((to + h1) / 24.0);
  double gst = (to + h1) - (n2 * 24.0);
  
  return gst;
}

/* translate GST to LST */
float gsttolst() {
  double diff = abs(longitude);
  diff = (diff / 15);
  double lst;
  if ((longitude * -1) > 0)
  {
    gstresult = gstresult - diff;
  }
  else
  {
    gstresult = gstresult + diff;
  }
  if (gstresult > 24)
  {
    lst = gstresult - 24;
  }
  else if ((gstresult * -1) > 0) {
    lst = gstresult + 24;
  }
  else
  {
    lst = gstresult;
  }
  return lst;
}

/* read data from client, using bluetooth */
void readFromUser() {
  readBluetoothData();
  targetCelestial.right_ascension = round(atof(buff) * 100) / 100.0;
  readBluetoothData();
  targetCelestial.declination = round(atof(buff) * 100000) / 100000.0;
  readBluetoothData();
  latitude = atof(buff);
  readBluetoothData();
  longitude = atof(buff);
  readBluetoothData();
  year = atoi(buff);
  readBluetoothData();
  month = atoi(buff);
  readBluetoothData();
  day = atoi(buff);
  readBluetoothData();
  hour = atoi(buff);
  readBluetoothData();
  minute = atoi(buff);

  Serial.println(year);
  Serial.println(month);
  Serial.println(day);
  Serial.println(hour);
  Serial.println(minute);
  Serial.println(targetCelestial.right_ascension, 7);
  Serial.println(targetCelestial.declination, 7);
  Serial.println(latitude, 7);
  Serial.println(longitude, 7);
}

/* read characters from bluetooth, untill a `\n` is encountered */
void readBluetoothData() {
  char c = 0;
  int i = 0;

  while (c != '\n') {
    while (hc06.available() == 0) {}
    c = hc06.read();
    buff[i] = c;
    i++;
  }
  buff[i] = '\0';
}
