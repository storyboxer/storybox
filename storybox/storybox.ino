/***************************************************
  This is an example for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer =
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

////

// constants won't change. They're used here to set pin numbers:
const int volumePin = 5;
const int trackPin = 4;
int currentTrackVal = -1;
int newTrackVal = -1;
int currentFolderVal = -1;
int newFolderVal = -1;
bool isPausePressed = false;
// bass values
byte bassAmp = 9; // value should be 0-15
byte bassFrq = 9; // value should be 0-150

// treble values
byte trebAmp = 1; // value should be 0-15
byte trebFrq = 1;

// convert values into ushort for cpu command
unsigned short bassCalced = ((trebAmp  & 0x0f) << 12)
                            | ((trebFrq  & 0x0f) <<  8)
                            | ((bassAmp  & 0x0f) <<  4)
                            | ((bassFrq  & 0x0f) <<  0);

void setup() {
  Serial.begin(9600);
  Serial.println("Adafruit VS1053 Library Test");
  delay(1000);
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  if (! musicPlayer.begin()) { // initialise the music player
    Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
    while (1);
  }
  Serial.println(F("VS1053 found"));
  // process bass cpu command
  //musicPlayer.sciWrite(0x02, bassCalced);
  //musicPlayer.setVolume(40, 40);

  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");
  if (! musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT))
  {
    Serial.println(F("DREQ pin is not an interrupt pin"));
  }
  //musicPlayer.startPlayingFile("/music2/file001.mp3", 10000);
}

void loop() {
  listenButtons();
  delay(100);
}

void listenButtons()
{
  if (musicPlayer.GPIO_digitalRead(3) == 1)
  {
    if (isPausePressed == true)
    {
      Serial.println("is already pressed");
      return;
    }
    else
    {
      isPausePressed = true;
      Serial.println("entering pause press");
    }
  }
  else
  {
    if (isPausePressed)
    {
      Serial.println("pressing ended");
    }
    isPausePressed = false;
  }
  // volume
  Serial.println(analogRead(volumePin));
  if (analogRead(volumePin)>1022)
  {
      digitalWrite(10, LOW);
  }
  else
  {
      digitalWrite(10, HIGH);
    }
  int volumeInput = 10 + (int)((analogRead(volumePin)) / 20);
  musicPlayer.setVolume(volumeInput, volumeInput);
  // track
  uint16_t input;
  uint16_t val;
  input = analogRead(trackPin);
  newFolderVal = ((input + 56) * 9 / 1023) + 1;

  if (newFolderVal != currentFolderVal)
  {
    Serial.print("Current folder val changed to : "); Serial.println(newFolderVal);
    currentFolderVal = newFolderVal;
    musicPlayer.stopPlaying();
    currentTrackVal = -1;
    newTrackVal = -1;
  }


  // firstly handle pause
  if (isPausePressed)
  {
    Serial.println("pressed pause");
    if (musicPlayer.paused())
    {
      musicPlayer.pausePlaying(false);
      Serial.println("was paused and resumed");
    }
    else
    {
      musicPlayer.pausePlaying(true);
      Serial.println("was playing and paused");
    }
    return;
  }

  for (uint8_t i = 1; i < 6; i++) {
    if (musicPlayer.GPIO_digitalRead(i) == 1 && i != 3)
    {
      //Serial.print("GPIO: "); Serial.print(i); Serial.println(" is pressed");
      //Serial.print("currentTrackVal: "); Serial.print(currentTrackVal); Serial.print("newTrackVal: ");Serial.println(newTrackVal);
      if (i > 3)
      {
        newTrackVal = i - 1;
      }
      else
      {
        newTrackVal = i;
      }
      String trackToPlay = String("/files") + currentFolderVal + String("/file00") + newTrackVal + String(".mp3");
      if (currentTrackVal != newTrackVal)
      {
        currentTrackVal = newTrackVal;
        Serial.print("GPIO: "); Serial.print(i); Serial.println(" is new track");
        Serial.println(trackToPlay);
        musicPlayer.stopPlaying();
        musicPlayer.startPlayingFile(trackToPlay.c_str());
      }
      else if (currentTrackVal == newTrackVal && musicPlayer.getFilePosition() > 50000 &&  !musicPlayer.paused())
      {
        uint32_t currentPosition = musicPlayer.getFilePosition();
        Serial.println(musicPlayer.getFilePosition());
        musicPlayer.setVolume(50, 50);
        delay(200);
        musicPlayer.stopPlaying();
        delay(200);
        musicPlayer.startPlayingFile(trackToPlay.c_str(), currentPosition + 300000);
      }
    }
  }
}
