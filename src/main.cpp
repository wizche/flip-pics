#include <M5EPD.h>
#include <memory>
#include <string>
#include <stdexcept>
#include <SD.h>

template <typename... Args>
std::string string_format(const std::string &format, Args... args)
{
  int size = snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
  if (size <= 0)
  {
    throw std::runtime_error("Error during formatting.");
  }
  std::unique_ptr<char[]> buf(new char[size]);
  snprintf(buf.get(), size, format.c_str(), args...);
  return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

M5EPD_Canvas canvas(&M5.EPD);
static int counter = 0;

void setup()
{
  M5.begin();
  M5.EPD.SetRotation(0);
  M5.TP.SetRotation(0);
  M5.EPD.Clear(true);
  M5.RTC.begin();
  canvas.createCanvas(540, 960);

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

  File file = root.openNextFile();
  Serial.printf("Filename %s\n", file.name());
  canvas.drawJpgFile(SD, file.name(), 0, 0 , 540, 960, 0, 0, JPEG_DIV_NONE);
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}

void loop()
{
  Serial.printf("Update %d\n", counter);
  counter += 1;
  delay(1000);
}