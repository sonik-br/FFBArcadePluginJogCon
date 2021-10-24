/*  
 *  PSX JogCon based Arcade USB controller
 *  (C) Alexey Melnikov
 *   
 *  Based on project by Mikael Norrgård <mick@daemonbite.com>
 *  
 *  GNU GENERAL PUBLIC LICENSE
 *  Version 3, 29 June 2007
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 */

////////////////////////////////////////////////////////

/*  
 *  Sketch modified by sonik-br
 *  
 *  Use Joystick class by MHeironimus
 *  Use PsxNewLib class by SukkoPera
 *  Both are modified from the original version
 *  
 *  Serial com to receive FFB commands from FFBArcadePlugin.
 *  
 *  JogCon commands based on the work by RandomInsano's pscontroller-rs
 *  
 *  Uses different arduino pins compared to original sketch.
 *  Pins are connected to arduino's hardware spi.
 *  
 */


//PS controller ATT pin
#define ATT 10

#define SP_MAX  160

////////////////////////////////////////////////////////


//#include <EEPROM.h>
#include "Joystick.h"
#include "PsxControllerHwSpi.h"

PsxControllerHwSpi<ATT> psx;

Joystick_* Gamepad;

bool haveController = false;

byte ff;
byte mode;
byte force;
int16_t sp_step;

void init_jogcon() {
  if (psx.begin()) {
    haveController = true;

    //delay(300);
    
    if (!psx.enterConfigMode ()) {
      //Serial.println (F("Cannot enter config mode"));
    } else {
      //must enable analog mode to use jogcon's paddle
      psx.enableAnalogSticks ();
      //must enable rumble to use the jogcon's motor
      psx.enableRumble ();
      psx.exitConfigMode ();
    }
    psx.read ();    // Make sure the protocol is up to date
  }
}

#define UP    0x1
#define RIGHT 0x2
#define DOWN  0x4
#define LEFT  0x8

int dpad2hat(uint8_t dpad)
{
  switch(dpad & (UP|DOWN|LEFT|RIGHT))
  {
    case UP:         return 0;
    case UP|RIGHT:   return 45;
    case RIGHT:      return 90;
    case DOWN|RIGHT: return 135;
    case DOWN:       return 180;
    case DOWN|LEFT:  return 225;
    case LEFT:       return 270;
    case UP|LEFT:    return 315;
  }
  return JOYSTICK_HATSWITCH_RELEASE;
}

uint8_t sp_div;
int16_t sp_max;
int16_t sp_half;

void setup()
{
  //mode = EEPROM.read(0) & 0x3;
  //if(mode == 3) mode = 0;
  mode = 1;

  //force = EEPROM.read(1) & 0xF;
  //if(!force) force = 15;
  force = 7;
  
  //sp_step = EEPROM.read(2);
  //if(sp_step > 8) sp_step = 8;
  //if(sp_step < 1) sp_step = 1;
  sp_step = 4;

  //sp_div = EEPROM.read(3) ? 1 : 2;
  sp_div = 1;
  sp_max = SP_MAX/sp_div;
  sp_half = sp_max/2;
  
  //delay(100);
  //init_jogcon();
  
  haveController = psx.begin();
  //Select button enables mouse mode

    Gamepad = new Joystick_ (
    "JogCon FFB",
    JOYSTICK_DEFAULT_REPORT_ID,
    JOYSTICK_TYPE_JOYSTICK,
    16,      // buttonCount
    1,      // hatSwitchCount (0-2)
    true,   // includeXAxis
    true,   // includeYAxis
    false,    // includeZAxis
    false,    // includeRxAxis
    false,    // includeRyAxis
    false,    // includeRzAxis
    false,    // includeRudder
    false,    // includeThrottle
    false,    // includeAccelerator
    false,    // includeBrake
    false,   // includeSteering
    false,   // includeDial
    false   // includeWheel
  );
  //Gamepad->setXAxisRange(-128, 127);
  Gamepad->setXAxisRange(0, 255);
  Gamepad->setYAxisRange(-128, 127);
  
  Gamepad->setXAxis(127);
  Gamepad->setYAxis(0);
  Gamepad->begin (false);

  Serial.begin(9600);
}

void loop()
{
  if (!haveController) {
    init_jogcon();
  } else {
    if(!psx.read()){
      haveController = false;
    } else {
      handleJogconData();
    }
  }
}


void handleJogconData()
{
  static uint16_t counter = 0, newcnt = 0, cleancnt = 0;
  //static uint16_t cleancnt = 0;
  static uint16_t newbtn = 0, oldbtn = 0;
  static int8_t oldspinner = 0;
  static uint8_t oldpaddle = 0;
  static int32_t pdlpos = sp_half;
  static uint16_t prevcnt = 0;
  
  uint8_t jogPosition = 0;
  uint8_t jogRevolutions = 0;
  JogconState jogStatus = psx.getJogconState(jogPosition, jogRevolutions);

  newcnt = (jogRevolutions << 8) | jogPosition;//(data[5] << 8) | data[4];
  newbtn = psx.getButtonWord();//(data[3] << 8) | data[2];
  newbtn = (newbtn & ~3) | ((newbtn&1)<<2);

  if(jogStatus == JOGCON_STATE_OTHER) {//(data[0] == 0xF3)
    // Mode switch by pressing "mode" button while holding:
    // L2 - paddle mode (with FF stoppers)
    // R2 - steering mode (FF always enabled)
    // L2+R2 - spinner mode (no FF)
    if(psx.buttonPressed(PSB_L2) && psx.buttonPressed(PSB_R2))
      mode = 0;
    else if(psx.buttonPressed(PSB_L2))
      mode = 1;
    else if(psx.buttonPressed(PSB_R2))
      mode = 2;


    // Force Feedback adjust
    // by pressing "mode" button while holding /\,O,X,[]
    if(psx.buttonPressed(PSB_TRIANGLE))
      force = 1;
    else if(psx.buttonPressed(PSB_CIRCLE))
      force = 3;
    else if(psx.buttonPressed(PSB_CROSS))
      force = 7;
    else if(psx.buttonPressed(PSB_SQUARE))
      force = 15;

    // Spinner pulses per step adjust
    // by pressing "mode" button while holding up,right,down,left
    if(psx.buttonPressed(PSB_PAD_UP))
      sp_step = 1;
    else if(psx.buttonPressed(PSB_PAD_RIGHT))
      sp_step = 2;
    else if(psx.buttonPressed(PSB_PAD_DOWN))
      sp_step = 4;
    else if(psx.buttonPressed(PSB_PAD_LEFT))
      sp_step = 8;

    // Paddle range switch by pressing "mode" button while holding:
    // L1 - 270 degree
    // R1 - 135 degree
    if(psx.buttonPressed(PSB_L1) || psx.buttonPressed(PSB_R1)) { //L 2, R 1
      sp_div = psx.buttonPressed(PSB_L1) ? 2 : 1;
      sp_max = SP_MAX/sp_div;
      sp_half = sp_max/2;
    }

    // some time for visual confirmation
    delay(200);

    // reset zero position
    init_jogcon();
    
    jogStatus = psx.getJogconState(jogPosition, jogRevolutions);

    prevcnt = 0;
    cleancnt = 0;
    counter = (jogRevolutions << 8) | jogPosition;//(data[5] << 8) | data[4];
    pdlpos = sp_half;
  } else {
    
    if(jogStatus != JOGCON_STATE_NONE) {
      cleancnt += newcnt - counter;
      if(!mode)
      {
        ff = 0;
        pdlpos += (int16_t)(newcnt - counter);
        if(pdlpos<0) pdlpos = 0;
        if(pdlpos>sp_max) pdlpos = sp_max;
      }
    }

    if(mode)
    {
      if(((int16_t)newcnt) < -sp_half)
      {
        pdlpos = 0;
        if(mode == 1) ff = 1;
      }
      else if(((int16_t)newcnt) > sp_half)
      {
        pdlpos = sp_max;
        if(mode == 1) ff = 1;
      }
      else
      {
        if(mode == 1) ff = 0;
        pdlpos = (uint16_t)(newcnt + sp_half);
      }
    }
    
    //if(mode == 2) ff = 1;

    

    //if (ff == 1) psx.setJogconMode(JOGCON_MODE_HOLD, force);
    //else psx.setJogconMode(JOGCON_MODE_STOP, 0);

    int16_t val = ((int16_t)(cleancnt - prevcnt))/sp_step;
    if(val>127) val = 127; else if(val<-127) val = -127;
    prevcnt += val*sp_step;
    int8_t spinner = val;
    
    uint8_t paddle = ((pdlpos*255)/sp_max);

    //if(oldbtn != newbtn || Gamepad->_GamepadReport.paddle != paddle || Gamepad->_GamepadReport.spinner != spinner) {
    if(oldbtn != newbtn || oldpaddle != paddle || oldspinner != spinner) {
      oldbtn = newbtn;
      oldpaddle = paddle;
      oldspinner = spinner;

      int16_t btn = (newbtn & 0xF) | ((newbtn>>4) & ~0xF);
      for (int8_t i = 2; i < 14; i++)
        Gamepad->setButton(i, bitRead(btn, i));

      //Gamepad->setWheel(paddle);
      //Gamepad->setDial(spinner);
      Gamepad->setXAxis(paddle);
      Gamepad->setHatSwitch(0, dpad2hat(newbtn>>4));
      Gamepad->sendState();
    }


    //Begin FFB section
    static uint8_t serialTimeOut = 0;
    bool haveCommand = false;
    
    if (Serial.available()) {
      byte returned = Serial.read();
      byte mode = returned & B11110000;
      byte force = returned & B00001111;
      serialTimeOut = 0;

      haveCommand = mode != 0x00;
      
      if (mode == 0x00)
        psx.setJogconMode(JOGCON_MODE_STOP, 0);
      else if (mode == 0x10)
        psx.setJogconMode(JOGCON_MODE_RIGHT, force);
      else if (mode == 0x20)
        psx.setJogconMode(JOGCON_MODE_LEFT, force);
      else if (mode == 0x30)
        psx.setJogconMode(JOGCON_MODE_HOLD, force);
      else if (mode == 0x80)
        psx.setJogconMode(JOGCON_MODE_DROP_REVOLUTIONS, force);
      else if (mode == 0xB0)
        psx.setJogconMode(JOGCON_MODE_DROP_REVOLUTIONS_AND_HOLD, force);
      else if (mode == 0xC0)
        psx.setJogconMode(JOGCON_MODE_NEWHOLD, force);
    }
  
    //paddle limiter
    if (ff == 1) { //this will override any previous command
      psx.setJogconMode(JOGCON_MODE_HOLD, 7);
    } else if (!haveCommand) {
      serialTimeOut++;
      if (serialTimeOut == 50) {//127
        serialTimeOut = 0;
        psx.setJogconMode(JOGCON_MODE_STOP, 0);
      }
    }
    
    //disable ff near center of paddle
    //if (paddle > 124 && paddle < 130) psx.setJogconMode(JOGCON_MODE_STOP, 0);

    //End FFB section
   
  }
  counter = newcnt;
}
