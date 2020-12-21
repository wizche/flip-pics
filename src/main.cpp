#include <M5EPD.h>
#include <memory>
#include <stdexcept>
#include <SD.h>

#include <iostream>
#include <vector>
#include <sstream>
#include <string>

#define SLEEP_HOURS 6

M5EPD_Canvas canvas(&M5.EPD);
const char *DATA_FILE = "/data.txt";
uint32_t lastCount;

bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool is_jpg(const std::string &filename)
{
  return (has_suffix(filename, ".jpg") || has_suffix(filename, ".jpeg"));
}

bool is_png(const std::string &filename)
{
  return has_suffix(filename, ".png");
}

bool is_valid_image(const std::string &filename)
{
  return is_jpg(filename) || is_png(filename);
}

std::vector<std::string> split(const std::string &s, char delimiter)
{
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter))
  {
    tokens.push_back(token);
  }
  return tokens;
}

void load_image()
{
  canvas.createCanvas(960, 540);
  canvas.fillCanvas(TFT_BLUE);
  canvas.setTextSize(2);

  File root = SD.open("/");
  if (!root)
  {
    Serial.printf("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.printf("Not a directory");
    return;
  }

  String lastFile;
  if (SD.exists(DATA_FILE))
  {
    auto f = SD.open(DATA_FILE, "rb");
    lastCount = f.read();
    Serial.printf("Read value from SD %d\n", lastCount);
    f.close();
  }
  else
  {
    lastCount = 0;
  }

  uint32_t currentCount = 0;
  while (true)
  {
    File file = root.openNextFile();

    if (!file)
    {
      Serial.printf("Reached end of directory, restart!\n");
      lastCount = 0;
      currentCount = 0;
      root.rewindDirectory();
      continue;
    }

    if (is_valid_image(file.name()))
    {
      Serial.printf("Filename %s\n", file.name());
      currentCount++;
    }
    else
    {
      continue;
    }

    if (currentCount > lastCount)
    {
      auto tokens = split(file.name(), '_');
      auto sizeTokens = split(tokens[1], 'x');
      auto offTokens = split(tokens[2], 'x');
      auto basename = tokens[0];
      int w = atoi(sizeTokens[0].c_str());
      int h = atoi(sizeTokens[1].c_str());
      int oX = atoi(offTokens[0].c_str());
      int oY = atoi(offTokens[1].c_str());

      Serial.printf("Found new image to render! Number %d\n", currentCount);
      Serial.printf("Size %dx%d - Offsets %dx%d\n", w, h, oX, oY);

      if (is_jpg(file.name()))
      {
        canvas.drawJpgFile(SD, file.name(), oX, oY, w, h, 0, 0, JPEG_DIV_NONE);
      }
      else if (is_png(file.name()))
      {
        canvas.drawPngFile(SD, file.name(), oX, oY, w, h, 0, 0, JPEG_DIV_NONE);
      }
      else
      {
        Serial.printf("Something went wrong!\n");
        break;
      }
      lastCount = currentCount;
      canvas.drawRightString(basename.c_str(), 960, 540, 1);
      break;
    }
  }

  auto f = SD.open(DATA_FILE, "wb");
  uint8_t buf[4];
  buf[0] = lastCount;
  buf[1] = lastCount >> 8;
  buf[2] = lastCount >> 16;
  buf[3] = lastCount >> 24;
  auto bytes = f.write(&buf[0], 4);
  f.close();
  Serial.printf("Written %d to file: %d\n", currentCount, bytes);

  char batteryBuffer[20];
  uint32_t vol = M5.getBatteryVoltage();

  if (vol < 3300)
  {
    vol = 3300;
  }
  else if (vol > 4350)
  {
    vol = 4350;
  }
  float battery = (float)(vol - 3300) / (float)(4350 - 3300);
  if (battery <= 0.01)
  {
    battery = 0.01;
  }
  if (battery > 1)
  {
    battery = 1;
  }
  sprintf(batteryBuffer, "%d%%", (int)(battery * 100));

  char statusBuffer[256] = "CHARGING";
  M5.SHT30.UpdateData();
  float tem = M5.SHT30.GetTemperature();
  float hum = M5.SHT30.GetRelHumidity();
  sprintf(statusBuffer, "%2.2fC | %0.2f%% | %s", tem, hum, batteryBuffer);

  canvas.drawRightString(statusBuffer, 960, 0, 1);
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  canvas.deleteCanvas();
}

void setup()
{
  M5.begin();
  M5.EPD.SetRotation(0);
  M5.EPD.Clear(1);
  M5.RTC.begin();

  load_image();
  //M5.shutdown(SLEEP_HOURS * 60 * 60);
}

void loop()
{
  if (M5.BtnL.wasPressed() || M5.BtnP.wasPressed() || M5.BtnR.wasPressed())
  {
    load_image();
  }
  M5.update();
  delay(100);

  //this only works when NOT attached
  //Serial.printf("Restart in 10 seconds!");
  delay(2000);
  //M5.shutdown(10);
  M5.shutdown(SLEEP_HOURS * 60 * 60);
}