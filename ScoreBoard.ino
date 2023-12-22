#include "ESP32-HUB75-MatrixPanel-I2S-DMA.h"
#include <string.h>
#include "BluetoothSerial.h"
#include <iostream>
using namespace std;
#include <sstream>

/*--------------------- MATRIX PANEL CONFIG -------------------------*/
#define PANEL_RES_X 64      // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1      // Total number of panels chained one to another

MatrixPanel_I2S_DMA *display = nullptr;

// Module configuration
HUB75_I2S_CFG mxconfig(
  PANEL_RES_X,   // module width
  PANEL_RES_Y,   // module height
  PANEL_CHAIN    // Chain length
);

struct Team{
  char* name;
  char* points;
};


static struct Team team1;
static struct Team team2;
static char team1Name[4] = {0};
static char team2Name[4] = {0};
static char team1Point[3] = {0};
static char team2Point[3] = {0};


bool saveScore = false;

static char frame[15];

/********** Bluetooth **************/
BluetoothSerial = SerialBT;
int incoming = 0; /* incoming message */

void setup()
{
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
    /************************* Initialize bluetooth ***********************/
  SerialBT.begin("Scoreboard");
  delay(200);

  // Allocate memory for team1.name and team1.points
  team1.name = (char*)malloc(sizeof(char) * 30); 
  team1.points = (char*)malloc(sizeof(char) * 30); 

  // Allocate memory for team2.name and team2.points
  team2.name = (char*)malloc(sizeof(char) * 30);
  team2.points = (char*)malloc(sizeof(char) * 30);

  team1.name = "TM_1";
  team1.points = "0  ";
  team2.name = "TM_2";
  team2.points = "0  ";

  Serial.println("...Starting Display");
  //mxconfig.double_buff = true; // Turn of double buffer
  mxconfig.clkphase = false;
  mxconfig.gpio.e = 32;
  // OK, now we can create our matrix object
  display = new MatrixPanel_I2S_DMA(mxconfig);
  display->begin();  // setup display with pins as pre-defined in the library

  writeText("WELCOME", 7, 20, 10, 1, 1);
  delay(500);
  showMatch(team1, team2);
}

void loop()
{
  display->flipDMABuffer(); // not used if double buffering isn't enabled
  delay(25);
  display->clearScreen();
  // Serial.println("...Starting aADASD");
  showMatch(team1, team2);

    /**************************** When connected to phone ********************************/   
    if(SerialBT.available())
    {
      incoming = SerialBT.read();
      Serial.print(F("[NEW INC]        incoming="));
      Serial.println(incoming, DEC);

      if(true == readFrame(incoming, &frame[0]))
      {
        Serial.print(F("[NEW INC]        frame="));
        Serial.println(frame[0], DEC);
        Serial.println(frame[1], DEC);
        Serial.println(frame[2], DEC);
        Serial.println(frame[3], DEC);
        Serial.println(frame[4], DEC);
        Serial.println(frame[5], DEC);
        Serial.println(frame[6], DEC);
        updateTeams(&frame[0]);

      }
    
    }

}

void updateTeams(char* frame)
{
  if(frame[0] == 254)
  {
    Serial.print(F("team1.name="));
    Serial.println((int)team1.name, DEC);
    Serial.print(F("team2.name="));
    Serial.println((int)team2.name, DEC);

    // memcpy(team3.name, &frame[1], 3);
    team1Name[0] = (char)frame[1];
    team1Name[1] = (char)frame[2];
    team1Name[2] = (char)frame[3];
    team1Name[3] = (char)frame[4];    
    team2Name[0] = (char)frame[5];
    team2Name[1] = (char)frame[6];
    team2Name[2] = (char)frame[7];
    team2Name[3] = (char)frame[8];

    team1.name = &team1Name[0];
    team2.name = &team2Name[0];

  }
  else if(frame[0] == 255)
  {
      if(frame[1] == 1)
      {
        int_to_char((int)frame[2], &team1Point[0]);
        team1.points = &team1Point[0];
      }
      else
      {
        // team2Point = (char)frame[2];
        int_to_char((int)frame[2], &team2Point[0]);
        team2.points = &team2Point[0];
      }
  }

}

bool readFrame(int incoming, char* frame)
{
  enum STATES{
    READ_ID, READ_LEN, READ_DATA
  };
  static STATES state = READ_ID;
  static int len = 0;
  static int iter = 0;
  static int id;
  Serial.print(F("[NEW INC]        new incoming="));
  Serial.println(incoming, DEC);
  Serial.print(F("[CURR STATE]        state(READ_ID, READ_LEN, READ_DATA)="));
  Serial.println(state, DEC);

  switch(state)
  {
    case READ_ID:
      id = incoming;
      state = READ_LEN;
      frame[iter] = id;
      iter++;
      Serial.print(F("[NEW STATE]        state(READ_ID, READ_LEN, READ_DATA)="));
      Serial.println(state, DEC);
      break;
    case READ_LEN:
      len = incoming;
      state = READ_DATA;
      Serial.print(F("READ_LEN LEN="));
      Serial.println(len, DEC);
      Serial.print(F("[NEW STATE]        state(READ_ID, READ_LEN, READ_DATA)="));
      Serial.println(state, DEC);
      break;
    case READ_DATA:
      frame[iter] = incoming;
      iter++;
      len--;

      if(len == 0){
        Serial.print(F("Len = 0"));
        state = READ_ID;
        iter = 0;
        return true;
      }
      break;
  }
  return false;
}

void writeText(char* text, int size, int x, int y, int textSize, int color)
{
  uint16_t colorChar = 0;

  if(color == 1)
  {
      colorChar=0xF111;
  }
  else if(color == 2)
  {
    colorChar=0x111F;
  }
  else
  {
    colorChar=0x1FF1;
  }

  for (int i = 0; i <size; i++)
  {
    // Draw rect and then calculate
    display->drawChar(x, y, *(text+i), colorChar, 0x0000, textSize);

    if(textSize == 1)
    {
      x+=6; 
    }else if(textSize == 2 && (text[i] == '1'))
    {
      x+=9; 
    }else
    {
      x+=11; 
    }

    if(x >=MATRIX_WIDTH-6)
    {
      y+=9;
      // display->setCursor(1,line*8);
    }
  }

  // oldCursorPos = display->getCursorY;

}

void showMatch(Team team1, Team team2)
{
  // Serial.println("1");

    char* teamsName = (char*)malloc(10*sizeof(char)); 
  // Serial.println("2");

    saveTeamsName(team1.name, team2.name, teamsName);
  // Serial.println("3");

  
    writeText(teamsName,9, 2, 2, 1, 3);
    // writeText("GSW VS LAL",10, 2, 2, 1, 3);

    writeText(team1.points,3, 0, 11, 2, 1);
    writeText(team2.points,3, 32, 11, 2, 2);
}

void saveTeamsName(char* team1, char* team2, char* teamsName)
{
  // char[] = {team1[0],team1[1], team1[2], ' ', 'v', 's', ' ', team2[0],team2[1], team2[2]};
  teamsName[0]=team1[0];
  teamsName[1]=team1[1];
  teamsName[2]=team1[2];
  teamsName[3]=team1[3];
  teamsName[4]='|';
  teamsName[5]=team2[0];
  teamsName[6]=team2[1];
  teamsName[7]=team2[2];
  teamsName[8]=team2[3];

}

void int_to_char(int num, char *result) {
    int temp = num;
    int len = 0;
 
    while (temp > 0) {
        len++;
        temp /= 10;
    }
 
    for (int i = len - 1; i >= 0; i--) {
        result[i] = num % 10 + '0';
        num /= 10;
    }
 
    result[len] = '\0';
}
