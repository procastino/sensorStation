#include <DHT11.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

#define BMP085_ADDRESS 0x77  // I2C address of BMP085

#define DHTLIB_OK 0
#define DHTLIB_ERROR_CHECKSUM -1
#define DHTLIB_ERROR_TIMEOUT -2

const unsigned char OSS = 0;  // Oversampling Setting

// Calibration values
int ac1;
int ac2; 
int ac3; 
unsigned int ac4;
unsigned int ac5;
unsigned int ac6;
int b1; 
int b2;
int mb;
int mc;
int md;

// b5 is calculated in bmp085GetTemperature(...), this variable is also used in bmp085GetPressure(...)
// so ...Temperature(...) must be called before ...Pressure(...).
long b5; 

short temperature;
long pressure;

// Use these for altitude conversions
const float p0 = 101325;     // Pressure at sea level (Pa)
float altitude;

int LDR = A3;
int luminosity = 0;

//int HPIN = A2;
float humidity = 0;

int NPIN = A1;
int noise = 0;

#define DHT11PIN A2
DHT11 dht11(DHT11PIN);


LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int speed;

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.clear();
  Wire.begin();
  bmp085Calibration();
  pinMode(LDR, INPUT);
  pinMode(DHT11PIN, INPUT);
  pinMode(NPIN, INPUT);
  speed = 5000;
}

void loop()
{
  showTemperature();
  delay(speed); 
  showPressure();
  delay(speed); 
  showAltitude();
  delay(speed); 
  showLuminosity();
  delay(speed); 
  showHumidity();
  delay(speed); 
  showNoise();
  delay(speed); 
}

void showTemperature(){
  temperature = bmp085GetTemperature(bmp085ReadUT())/10;
  Serial.print("Temperature: ");
  Serial.print(temperature, DEC);
  Serial.println(" C");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperature (C):");
  lcd.setCursor(0, 1);
  lcd.print(temperature);
}

void showPressure(){
    pressure = bmp085GetPressure(bmp085ReadUP());
  Serial.print("Pressure: ");
  Serial.print(pressure/100, DEC);
  Serial.println(" hPa");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pressure (hPa):");
  lcd.setCursor(0, 1);
  lcd.print(pressure/100);
}

void showAltitude(){
    altitude = (float)44330 * (1 - pow(((float) pressure/p0), 0.190295));
  Serial.print("Altitude: ");
  Serial.print(altitude, 2);
  Serial.println(" m");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Altitude (m):");
  lcd.setCursor(0, 1);
  lcd.print(altitude);
}

void showLuminosity(){
  luminosity=map(analogRead(LDR),0,1023,100,0);
  Serial.print("Luminosity: ");
  Serial.print(luminosity);
  Serial.println(" %");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Luminosity (%):");
  lcd.setCursor(0, 1);
  lcd.print(luminosity);
}

void showHumidity(){
  float temp;
  int chk = dht11.read(humidity,temp);
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity (%):");
  lcd.setCursor(0, 1);
  lcd.print(humidity);
}

void showNoise(){
   noise=map(analogRead(NPIN),0,1023,0,100);
  Serial.print("Noise: ");
  Serial.print(noise);
  Serial.println(" %");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Noise (%):");
  lcd.setCursor(0, 1);
  lcd.print(noise);
}

// Stores all of the bmp085's calibration values into global variables
// Calibration values are required to calculate temp and pressure
// This function should be called at the beginning of the program
void bmp085Calibration()
{
  ac1 = bmp085ReadInt(0xAA);
  ac2 = bmp085ReadInt(0xAC);
  ac3 = bmp085ReadInt(0xAE);
  ac4 = bmp085ReadInt(0xB0);
  ac5 = bmp085ReadInt(0xB2);
  ac6 = bmp085ReadInt(0xB4);
  b1 = bmp085ReadInt(0xB6);
  b2 = bmp085ReadInt(0xB8);
  mb = bmp085ReadInt(0xBA);
  mc = bmp085ReadInt(0xBC);
  md = bmp085ReadInt(0xBE);
}

// Calculate temperature given ut.
// Value returned will be in units of 0.1 deg C
short bmp085GetTemperature(unsigned int ut)
{
  long x1, x2;
  
  x1 = (((long)ut - (long)ac6)*(long)ac5) >> 15;
  x2 = ((long)mc << 11)/(x1 + md);
  b5 = x1 + x2;

  return ((b5 + 8)>>4);  
}

// Calculate pressure given up
// calibration values must be known
// b5 is also required so bmp085GetTemperature(...) must be called first.
// Value returned will be pressure in units of Pa.
long bmp085GetPressure(unsigned long up)
{
  long x1, x2, x3, b3, b6, p;
  unsigned long b4, b7;
  
  b6 = b5 - 4000;
  // Calculate B3
  x1 = (b2 * (b6 * b6)>>12)>>11;
  x2 = (ac2 * b6)>>11;
  x3 = x1 + x2;
  b3 = (((((long)ac1)*4 + x3)<<OSS) + 2)>>2;
  
  // Calculate B4
  x1 = (ac3 * b6)>>13;
  x2 = (b1 * ((b6 * b6)>>12))>>16;
  x3 = ((x1 + x2) + 2)>>2;
  b4 = (ac4 * (unsigned long)(x3 + 32768))>>15;
  
  b7 = ((unsigned long)(up - b3) * (50000>>OSS));
  if (b7 < 0x80000000)
    p = (b7<<1)/b4;
  else
    p = (b7/b4)<<1;
    
  x1 = (p>>8) * (p>>8);
  x1 = (x1 * 3038)>>16;
  x2 = (-7357 * p)>>16;
  p += (x1 + x2 + 3791)>>4;
  
  return p;
}

// Read 1 byte from the BMP085 at 'address'
char bmp085Read(unsigned char address)
{
  unsigned char data;
  
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  
  Wire.requestFrom(BMP085_ADDRESS, 1);
  while(!Wire.available())
    ;
    
  return Wire.read();
}

// Read 2 bytes from the BMP085
// First byte will be from 'address'
// Second byte will be from 'address'+1
int bmp085ReadInt(unsigned char address)
{
  unsigned char msb, lsb;
  
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(address);
  Wire.endTransmission();
  
  Wire.requestFrom(BMP085_ADDRESS, 2);
  while(Wire.available()<2)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  
  return (int) msb<<8 | lsb;
}

// Read the uncompensated temperature value
unsigned int bmp085ReadUT()
{
  unsigned int ut;
  
  // Write 0x2E into Register 0xF4
  // This requests a temperature reading
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();
  
  // Wait at least 4.5ms
  delay(5);
  
  // Read two bytes from registers 0xF6 and 0xF7
  ut = bmp085ReadInt(0xF6);
  return ut;
}

// Read the uncompensated pressure value
unsigned long bmp085ReadUP()
{
  unsigned char msb, lsb, xlsb;
  unsigned long up = 0;
  
  // Write 0x34+(OSS<<6) into register 0xF4
  // Request a pressure reading w/ oversampling setting
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF4);
  Wire.write(0x34 + (OSS<<6));
  Wire.endTransmission();
  
  // Wait for conversion, delay time dependent on OSS
  delay(2 + (3<<OSS));
  
  // Read register 0xF6 (MSB), 0xF7 (LSB), and 0xF8 (XLSB)
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.write(0xF6);
  Wire.endTransmission();
  Wire.requestFrom(BMP085_ADDRESS, 3);
  
  // Wait for data to become available
  while(Wire.available() < 3)
    ;
  msb = Wire.read();
  lsb = Wire.read();
  xlsb = Wire.read();
  
  up = (((unsigned long) msb << 16) | ((unsigned long) lsb << 8) | (unsigned long) xlsb) >> (8-OSS);
  
  return up;
}

//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
        return 1.8 * celsius + 32;
}

// fast integer version with rounding
//int Celcius2Fahrenheit(int celcius)
//{
//  return (celsius * 18 + 5)/10 + 32;
//}


//Celsius to Kelvin conversion
double Kelvin(double celsius)
{
        return celsius + 273.15;
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//
double dewPoint(double celsius, double humidity)
{
        // (1) Saturation Vapor Pressure = ESGG(T)
        double RATIO = 373.15 / (273.15 + celsius);
        double RHS = -7.90298 * (RATIO - 1);
        RHS += 5.02808 * log10(RATIO);
        RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
        RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
        RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
        double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
        double T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558 - T);
}

// delta max = 0.6544 wrt dewPoint()
// 6.9 x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
        double a = 17.271;
        double b = 237.7;
        double temp = (a * celsius) / (b + celsius) + log(humidity*0.01);
        double Td = (b * temp) / (a - temp);
        return Td;
}


