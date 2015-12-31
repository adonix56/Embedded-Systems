/*
 *********************************************************************************************************
 *
 *                        (c) Copyright 2015-2020 RedPine Signals Inc. HYDERABAD, INDIA
 *                                            All rights reserved.
 *
 *               This file is protected by international copyright laws. This file can only be used in
 *               accordance with a license and should not be redistributed in any way without written
 *               permission by Redpine Signals.
 *
 *                                            www.redpinesignals.com
 *
 *********************************************************************************************************
 */
 /*
 *********************************************************************************************************
 *
 *											  main file
 *
 * File           : main.c
 * Version        : V1.00
 * Programmer(s)  : Justin Salanga
			Peter Chim
 * Description    : This example uses Serial Peripheral Interface (SPI) of WyzBee board as master in polling method and OLED screen as slave to print a text message.
 *********************************************************************************************************
 * Note(s)		 :
 *********************************************************************************************************
 */
 
 /*
 * Includes
 */
#include <wyzbee_bt.h>
#include <WyzBee_gpio.h>
#include <WyzBee_ext.h>
#include <WyzBee_timer.h>
#include <Adafruit_GFX.h>
#include "Adafruit_SSD1351.h"
#include <SPI_OLED.h>
#include <WyzBee_spi.h>
#include <stdio.h>
#include <stdlib.h>
#include <delay.h>
#include <WyzBee.h>
#include <string.h>
#include <time.h>

/////////////////
// GLOBALS
/////////////////
//Game
bool gameOver = false;		// check if game is over
int spaceFighterSpeed = 4;	// speed of falling aliens
uint8_t spaceBulletSpeed = 8;	// speed of bullets
int myScore = 0;	// current score
int hiScore = 0;	// high score
uint8_t numBullets = 4;		// number of bullets allowed to be shot at once
uint8_t numFighters = 4;	// number of space fighter objects (yours + aliens)
bool newHighScore = false;	// check if new high score is scored
char hiScoreName[5] = "\0";		// name of high score player
char newHiScoreName[5] = "\0";	// name of new high score player
uint8_t currCharNum = 0;		// current character position when inputting high score name
//OLED
uint16_t ScreenHEIGHT = SSD1351HEIGHT;	// max screen height
uint16_t ScreenWIDTH = SSD1351WIDTH;	// max screen width
uint8_t screenNum = 2;		// screen number (1: title, 2: game, 3: game over, 4: BTSenderSetup, 5: BTSenderMain, 6: BTReceiver)
int bgColor = BLACK;	// background color (can be changed)
//Bluetooth
bool btReceive = false; // Toggle between bluetooth receiver (true) and infrared receiver (false)
uint8 btdata = '\0'; // Data sent though bluetooth
char justinAddress[] = "00:23:A7:80:59:92"; //Address for Justin's WyzBee Board (Sender)
char peterAddress[] = "00:23:A7:80:59:A3";  //Address for Peter's WyzBee Board (Receiver)
/////////////////////////////////////
//Uncomment if using Justin's Board//
uint8 localName[] = "WyzBeeJustin\0";  
char localAddress[] = "00:23:A7:80:59:92";
/////////////////////////////////////
// Uncomment if using Peters Board///
//uint8 localName[] = "WyzBeePeter\0";
//char localAddress[] = "00:23:A7:80:59:A3"; 
/////////////////////////////////////

//ExtInt
uint32_t prevStamp = 0x7FFFFFFF;	// previous time stamp 
uint32_t timeDiff = 0;		// time difference
char myString[50] = "x\0";  // string display variable
char binData[33] = {};		// variable to hold button pressed code
uint8_t binNum = 0;			// holds timing code position in binData array 
uint8_t binNumLimit = 17;	// holds limit of binData array
bool release = false; //release button press to prevent repeats
	
extern Adafruit_SSD1351 tft = Adafruit_SSD1351(); //@  OLED class variable
/*===================================================*/
/**
 * @fn			void print_oled(int8 *text, uint16_t color)
 * @brief		this functions is used to print the data on the oled screen
 * @param 1[in]	int8 *text
 * @param 2[in]	uint16_t color , color
 * @param[out]	none
 * @return		none
 * @description This API should contain the code / function call which will prints the data on the OLED screen.
 */

WyzBeeSpi_Config_t  config_stc={
		4000000,
		SpiMaster,
		SpiSyncWaitZero,
		SpiEightBits,
		SpiMsbBitFirst,
		SpiClockMarkHigh,
		SpiNoUseIntrmode,
		NULL,                                                                
		NULL
};

// space fighter object, stores positions
class spaceFighter{
  private:	
		int x;
		int y;
	
	public:
		spaceFighter(){
			x = -48;
			y = 256;
		}		
		spaceFighter(int myX, int myY){
			x = myX;
			y = myY;
		}
	
		void setX(int myX){
			x = myX;
		}	
		void setY(int myY){
			y = myY;
		}
		int getX(){
			return(x);
		}
		int getY(){
			return(y);
		}		
};

// positions of space fighter bullets that get shot out
class spaceBullet{
  private:	
		uint16_t x;
		uint16_t y;
		bool moving;
	
	public:
		spaceBullet(){
			x = -10;
			y = 130;
		}		
		spaceBullet(uint16_t myX, uint16_t myY){
			x = myX;
			y = myY;
		}
	
		void setX(uint16_t myX){
			x = myX;
		}	
		void setY(uint16_t myY){
			y = myY;
		}
		uint16_t getX(){
			return(x);
		}
		uint16_t getY(){
			return(y);
		}
		void go(bool move){
			moving = move;
		}
};

// initialize positions of all the space fighters (yours + aliens)
void initializeSpaceFighters(spaceFighter * myFighter){
	myFighter[0].setX(0);
	myFighter[0].setY(112);
	for (int i = 1; i<4; i++){
		myFighter[i].setX(16);
		myFighter[i].setY(156);
	}
}

// Updates space fighter screen as you, aliens, and bullets move
void spaceFighters(Adafruit_SSD1351 pOled, spaceFighter * myFighters, spaceBullet * myBullets){
	// display where your space ship is (as a triangle)
	pOled.fillTriangle(myFighters[0].getX(), myFighters[0].getY()+15,
										 myFighters[0].getX()+15, myFighters[0].getY()+15,
										 myFighters[0].getX()+7, myFighters[0].getY(), GREEN);
	
	// display where your bullets are (as a small line)
	for (int i=0; i < numBullets; i++){
		if ( (myBullets[i].getY() >= 0) & (myBullets[i].getY() < ScreenHEIGHT) ){
			myBullets[i].setY(myBullets[i].getY()-spaceBulletSpeed);
			pOled.fillRect(myBullets[i].getX()+7, myBullets[i].getY(), 2, 8, YELLOW);
		}
	}
	
	// display the aliens (as squares)
	for (int i=1; i < numFighters; i++){
		myFighters[i].setY(myFighters[i].getY()+spaceFighterSpeed);
		if ( (myFighters[i].getY() >= 0) & (myFighters[i].getY() < ScreenHEIGHT) ){
			pOled.drawRect(myFighters[i].getX(), myFighters[i].getY(), 16, 16, RED);
		}
	}	

	// check conditions of space fighters and act accordingly
	for (int i=1; i < numFighters; i++){
		// if the alien space fighters are onscreen 
		if ( (myFighters[i].getY() >= 0) & (myFighters[i].getY() < ScreenHEIGHT) ){
				for (int j=0; j < numBullets; j++){
					// if any bullets hit the aliens
					if (   (myBullets[j].getX() == myFighters[i].getX()) & 
							   (myBullets[j].getY() <= (myFighters[i].getY() + 15 + spaceFighterSpeed)) &
							   (myBullets[j].getY() >= (myFighters[i].getY() - spaceBulletSpeed )) ){
									// show alien explosion as qhite square
									pOled.fillRect(myFighters[i].getX(), myFighters[i].getY(), 16, 16, WHITE);
									uint16_t newPosY = 156;		
									myFighters[i].setY(newPosY); // move alien off screen
									myScore = myScore+spaceFighterSpeed; // update score
						}
				}
				// if any aliens hit your spaceship
				if ( (myFighters[i].getX() == myFighters[0].getX()) & 
							   (myFighters[0].getY() >= myFighters[i].getY())  &
							   (myFighters[0].getY() <= (myFighters[i].getY() + 16 ))){
								pOled.fillCircle(myFighters[0].getX()+7, myFighters[0].getY()+7, 7, WHITE); 
								myFighters[0].setY(256);	// set your spaceship offscreen
								gameOver = true;	// set game over condition
								}
		} else if (myFighters[i].getY() < 0) {	// if fighter is above screen, do nothing		
		} // if the alien goes below screen
		else if ( (myFighters[i].getY() >= ScreenHEIGHT) & (myFighters[i].getY() < 0x8000) ){
				srand(Dt_ReadCurCntVal(Dt_Channel0));	// get a random seed number
				int newPosY = rand()%200 - 200;		// set a new y position
				int newPosX = (rand()%8)*16;		// set a new x position
				myFighters[i].setY(newPosY);
				myFighters[i].setX(newPosX);		// move alien to a new random location	
		}
	}
}

// find which bullets are offscreen (free) to allow spaceship to shoot 
spaceBullet * findFreeBullet(spaceBullet *myBullets){
	uint8_t i = 0;				// index of bullert number
	bool freeBullet = false;	// check if bullet is free
	spaceBullet * myBullet;
	
	// check all bullets to see if any are offscreen
	while ( (i < numBullets) & !freeBullet){
		// if current bullet i is free, return the bullet
		if ( (myBullets[i].getY() < 0) | (myBullets[i].getY() > ScreenHEIGHT) ){
			myBullet = &myBullets[i];
			freeBullet = true;
		}
		i++;
	}
	
	// if any free bullet found, return bullet, else return 0
	if (freeBullet)
		return myBullet;
	else
		return 0;
};

// for screen 3, update high score screen
void drawHighScore(Adafruit_SSD1351 pOled){
	// draw the new high score letters taht you input
	pOled.drawChar(0,56,newHiScoreName[0], WHITE, bgColor, 1);
	pOled.drawChar(8,56,newHiScoreName[1], WHITE, bgColor, 1);
	pOled.drawChar(16,56,newHiScoreName[2], WHITE, bgColor, 1);		
	pOled.drawChar(24,56,'x', WHITE, bgColor, 1);	
	
	// show position of cursor when inputting high score
	if (currCharNum == 0){
		pOled.drawRect(0, 56, 8, 10, YELLOW);
	}
	else if(currCharNum == 1){
		pOled.drawRect(8, 56, 8, 10, YELLOW);	
	}
	else if(currCharNum == 2){
		pOled.drawRect(16, 56, 8, 10, YELLOW);	
	}
	else if(currCharNum == 3){
		pOled.drawRect(24, 56, 8, 10, YELLOW);		
	}
}

/*IR receiver call back.
Get time coding of the interrupt*/
void Main_ExtIntCallback1()
{		
	// do not allow button release until a certain time has passes
	release = false;
	// timing difference
	timeDiff = prevStamp - Dt_ReadCurCntVal(Dt_Channel0);
	prevStamp = Dt_ReadCurCntVal(Dt_Channel0);	// set new previous time
	
	// encode characters based on timing difference
	if (timeDiff < 2500){	// < .5ms
		binData[binNum] = '0';	// character encoding
		binNum++;	// increase position of binData array
		binNumLimit = 17;	// set limit of code array ('17' for numericals, channels)
	} else if (timeDiff < 3500){	// < .7ms	
		binData[binNum] = '1';
		binNum++;
		binNumLimit = 17;
	} else if (timeDiff < 4500){	// < .9ms
		binData[binNum] = '2';
		binNum++;
		binNumLimit = 17;
	} else if (timeDiff < 5000) { // < 1.0ms
		binData[binNum] = '3';
		binNum++;
		binNumLimit = 17;
	} else if (timeDiff < 10000) { // < 2.0 ms
		binData[binNum] = 'a';	// character encoding
		binNum++;
		binNumLimit = 22; // set limit of code array (22 for non-numericals, e.g. power button, OK button)
	} else if (timeDiff < 50000){	// < 10.0ms
		binData[binNum] = 'b';
		binNum++;
		binNumLimit = 22;
	}

	// check if code array is at its limit
	bool atLimit = (binNum == binNumLimit);
	if ((timeDiff >= 50000) | atLimit ){
		binData[binNumLimit] = '\0';	// set null terminator for code array
		binNum = 0;		// reset position of binData code
		if (atLimit){		
			WyzBee_Exint_DisableChannel(2);		// diable interrupt
		}
	}	
	
}

//Print on the OLED
void pstring(Adafruit_SSD1351 pled, int x, int y, char myChar[], uint16_t color, uint8_t size)
{
	pled.setCursor(x, y);
	pled.setTextSize(size);
	pled.setTextColor(color);
	int tmp = 0;
	while(myChar[tmp] != '\0')
	{
		pled.write(myChar[tmp]);
		tmp++;
	}
}

/******************************************************************************
 ** Setup Bluetooth Profile
 **
 ******************************************************************************/
uint16 btSetup(Adafruit_SSD1351 myOled)
{
	uint16 ret;
	
	ret = WyzBee_BT_init();
	if (0 != ret) {
		char error[] = "Error: Failed WyzBee_BT_init";
		pstring(myOled, 0, 0, error, RED, 1); 
		return ret; 
	}
	ret = WyzBee_SetLocalName(localName);
	if (0 != ret) {
		char error[] = "Error: Failed WyzBee_SetLocalName";
		pstring(myOled, 0, 0, error, RED, 1); 
		return ret; 
	}
	ret = WyzBee_SetPairMode(0);
	if (0 != ret){
		char error[] = "Error: Failed WyzBee_SetPairMode";
		pstring(myOled, 0, 0, error, RED, 1);
		return ret;
	}
	ret = WyzBee_SetDiscoverMode(1, 5000);
	if (0 != ret) {
		char error[] = "Error: Failed WyzBee_SetDiscoverMode";
		pstring(myOled, 0, 0, error, RED, 1);
		return ret;
	}
	ret = WyzBee_SetConnMode(1);
	if (0 != ret) {
		char error[] = "Error: Failed WyzBee_SetConnMode";
		pstring(myOled, 0, 0, error, RED, 1); 
		return ret; 
	}
	ret = WyzBee_InitSppProfile();
	if (0 != ret) {
		char error[] = "Error: Failed WyzBee_InitSppProfile";
		pstring(myOled, 0, 0, error, RED, 1); 
		return ret; 
	}
	
	return ret;
}

/******************************************************************************
 ** Setup Bluetooth Sender
 **
 ******************************************************************************/
int btSender(Adafruit_SSD1351 myOled)
{
	uint16 ret;
	uint8 data;

	ret = WyzBee_SPPConnet((uint8 *)peterAddress);
	if (0 != ret) {
		char error[] = "Error: Failed WyzBee_SPPConnet";
		pstring(myOled, 0, 0, error, RED, 1);
		return ret;
	}
	
	ret = WaitForSPPConnComplete();
	if (0 != ret) {
		char error[] = "Error: Failed WaitForSPPConnComplete";
		pstring(myOled, 0, 0, error, RED, 1);
		return ret;
	}
}
/******************************************************************************
 ** Setup Bluetooth Receiver
 **
 ******************************************************************************/

int btReceiver(Adafruit_SSD1351 myOled)
{
	uint16 ret;
	uint8 data;
	
	ret = WaitForSPPConnComplete();
	if (0 != ret) {
		char error[] = "Error: Failed WaitForSPPConnComplete";
		pstring(myOled, 0, 0, error, RED, 1);
		return ret;
	}
}

/******************************************************************************
 ** Decode Space Fighter
 ** for button press interrupts
 ******************************************************************************/

void decodeSpaceFighter(spaceFighter *myFighter, spaceBullet *myBullets){
		char numCode[18] = "10203100002120\0";	// decode for numericals and channel buttons
		char numCode2[18] = "10203100022120\0"; // also decode for numericals and channel buttons
		char numCode3[19] = "baaabaaaaaaabaaaa\0"; // decode for non-numerical buttons
		int currX = myFighter[0].getX();	// x position of your spaceship
		int currY = myFighter[0].getY();    // y position of your spaceship
		char num = '\0';
		char resetNum[6] = "0000\0";	// reset code
		spaceBullet *myBullet = 0;		// pointer to bullets
	
		// decode numericals, "CHANNEL" change, and "LAST" buttons
		if (binNumLimit == 17){
				if (strncmp(binData, numCode, 14)==0 | strncmp(binData, numCode2, 14)==0){
						if(binData[14] == '2'){
								if(binData[16] == '0'){	// channel up (+)
										if(screenNum == 5) { // if bluetooth connected
                                            //Send 'C' for Channel Up
											sprintf(myString, "Channel Up");
											btdata = 'C';
										} 
										else if (newHighScore){	// if high score screen
												if (currCharNum < 3){
														currCharNum++;	// move position of text cursor right
												}						
										} else {
												switch(bgColor){ // change bg color
														case BLACK: bgColor = YELLOW; break;
														case YELLOW: bgColor = CYAN; break;
														case CYAN: bgColor = MAGENTA; break;
														case MAGENTA: bgColor = BLACK; break;
														default: bgColor = BLACK;
												}
										}
								} else if(binData[16] == '1'){ // channel down (-)	
										if(screenNum == 5) {// if bluetooth connected
                                                //Send 'c' for Channel Down
												sprintf(myString, "Channel Down");
												btdata = 'c';
										} 								
										if (newHighScore){ // if high score screen
												if (currCharNum >= 0){
														currCharNum--;	// move position of text cursor left
												}	
										}
								}
					} else if(binData[14] == '0'){
								if(binData[15] == '0'){
										if(binData[16] == '0'){		// decode to 0
												if(screenNum == 5) {// if bluetooth connected
                                                    //Send '0' for Zero
													sprintf(myString, "Zero");
													btdata = '0';
												} 
												if (newHighScore) // if high score screen
												newHiScoreName[currCharNum] = '\0'; // display "space"
										} else if(binData[16] == '1'){		// decode to 1
											if(screenNum == 5) {// if bluetooth connected
                                                //Send '1' for One
												sprintf(myString, "One");
												btdata = '1';
											} 
											if (screenNum == 2) { //start bluetooth session
												screenNum = 4;
											}
										} else if (binData[16] == '2'){		// decode to 2
												if(screenNum == 5) {// if bluetooth connected
                                                    //Send '2' for Two
													sprintf(myString, "Two");
													btdata = '2';
												} 
												if (newHighScore){
														// flip through letters ABC for each press when entering initials
														switch(newHiScoreName[currCharNum]){
														case 'A': newHiScoreName[currCharNum] = 'B'; break; 
														case 'B': newHiScoreName[currCharNum] = 'C'; break; 
														case 'C': newHiScoreName[currCharNum] = 'A'; break; 
														default: newHiScoreName[currCharNum] = 'A';
														}
												}
									} else if (binData[16] == '3'){		// decode to 3
												if( screenNum == 5) {// if bluetooth connected
                                                    //Send '3' for Three
													sprintf(myString, "Three");
													btdata = '3';
												}
												if (newHighScore){
														// flip through letters DEF for each press when entering initials
														switch(newHiScoreName[currCharNum]){
														case 'D': newHiScoreName[currCharNum] = 'E'; break; 
														case 'E': newHiScoreName[currCharNum] = 'F'; break; 
														case 'F': newHiScoreName[currCharNum] = 'D'; break; 
														default: newHiScoreName[currCharNum] = 'D';
														}
												}
									}
							} else if(binData[15] == '1'){
										if(binData[16] == '0'){		// decode to 4
												if( screenNum == 5) { // if bluetooth connected
                                                    //Send '4' for Four
													sprintf(myString, "Four");
													btdata = '4';
												}
												if (newHighScore){
														// flip through letters GHI for each press when entering initials
														switch(newHiScoreName[currCharNum]){
														case 'G': newHiScoreName[currCharNum] = 'H'; break; 
														case 'H': newHiScoreName[currCharNum] = 'I'; break; 
														case 'I': newHiScoreName[currCharNum] = 'G'; break; 
														default: newHiScoreName[currCharNum] = 'G';
														}
												}
										} else if(binData[16] == '1'){		// decode to 5
												if( screenNum == 5) { // if bluetooth connected
                                                    //Send '5' for Shoot!
													sprintf(myString, "Five - SHOOT!");
													btdata = '5';
												}
												if (newHighScore){
														// flip through letters JKL for each press when entering initials
														switch(newHiScoreName[currCharNum]){
														case 'J': newHiScoreName[currCharNum] = 'K'; break; 
														case 'K': newHiScoreName[currCharNum] = 'L'; break; 
														case 'L': newHiScoreName[currCharNum] = 'J'; break; 
														default: newHiScoreName[currCharNum] = 'J';
														}
												}else {
														// shoot your bullet if on game screen
														myBullet = findFreeBullet(myBullets);
														if (myBullet){
																myBullet->setX(currX);
																myBullet->setY(currY);					
														}			
												}
									} else if (binData[16] == '2'){		// decode to 6
											if( screenNum == 5) { // if bluetooth connected
                                                //Send '6' for Six
												sprintf(myString, "Six");
												btdata = '6';
											}
											if (newHighScore){
													// flip through letters MNO for each press when entering initials
													switch(newHiScoreName[currCharNum]){
													case 'M': newHiScoreName[currCharNum] = 'N'; break; 
													case 'N': newHiScoreName[currCharNum] = 'O'; break; 
													case 'O': newHiScoreName[currCharNum] = 'M'; break; 
													default: newHiScoreName[currCharNum] = 'M';
													}
											}
									}	else if (binData[16] == '3'){		// decode to 7
											if( screenNum == 5) { // if bluetooth connected
                                                //Send '7' for Seven
												sprintf(myString, "Seven - Move Left");
												btdata = '7';
											}
											if (newHighScore){
													// flip through letters PQRS for each press when entering initials
													switch(newHiScoreName[currCharNum]){
													case 'P': newHiScoreName[currCharNum] = 'Q'; break; 
													case 'Q': newHiScoreName[currCharNum] = 'R'; break; 
													case 'R': newHiScoreName[currCharNum] = 'S'; break;
													case 'S': newHiScoreName[currCharNum] = 'P'; break;							
													default: newHiScoreName[currCharNum] = 'P';
													}
											} else {
													// set move your spaceship left, if on game screen
													if (currX - 16 >= 0)
													myFighter[0].setX(currX - 16);
											}					
								}
						}	else if(binData[15] == '2'){
										if(binData[16] == '0'){		// decode to 8
												if( screenNum == 5) {// if bluetooth connected
                                                    //Send '8' for Eight
													sprintf(myString, "Eight");
													btdata = '8';
												} //BTSender SCREEN5
												if (newHighScore){
														// flip through letters TUV for each press when entering initials
														switch(newHiScoreName[currCharNum]){
														case 'T': newHiScoreName[currCharNum] = 'U'; break; 
														case 'U': newHiScoreName[currCharNum] = 'V'; break; 
														case 'V': newHiScoreName[currCharNum] = 'T'; break; 
														default: newHiScoreName[currCharNum] = 'T';
													}
												}
										} else if(binData[16] == '1'){		// decode to 9
												if( screenNum == 5) {// if bluetooth connected
                                                    //Send '9' for Nine
													sprintf(myString, "Nine - Move Right");
													btdata = '9';
												}
												if (newHighScore){
														// flip through letters WXYZ for each press when entering initials
														switch(newHiScoreName[currCharNum]){
														case 'W': newHiScoreName[currCharNum] = 'X'; break; 
														case 'X': newHiScoreName[currCharNum] = 'Y'; break; 
														case 'Y': newHiScoreName[currCharNum] = 'Z'; break; 
														case 'Z': newHiScoreName[currCharNum] = 'W'; break;
														default: newHiScoreName[currCharNum] = 'W';
												}
											} else {		
													// move your spaceship right
													if (currX + 16 < ScreenWIDTH)
													myFighter[0].setX(currX + 16);					
												}	
									} else if(binData[16] == '2'){	// decode to "Last"
											num = 'L';	// do nothing			
									}
				} // end bin 15
			} // end bin 14
	} // end strncmp
// decode other main buttons
} else if (strncmp(binData, numCode3, 17)==0){
			 if(binData[17] == 'a'){
						if(binData[18] == 'a'){
								if(binData[19]=='b'){	// decode to volume up (+)
										if( screenNum == 5) { // if bluetooth connected
                                            //Send 'V' for Volume Up
											sprintf(myString, "Volume Up");
											btdata = 'V';
										}
										if (spaceFighterSpeed < 32)
												spaceFighterSpeed++;	// increase alien speed
								}
						} else if (binData[18] == 'b'){	// decode to MUTE
								if(binData[19]=='b')
										num = 'm';	// do nothing
						} 
			} else if (binData[17] == 'b'){
						if(binData[18] == 'a'){
								if(binData[19]=='b'){	// decode to volume down (-)
										if( screenNum == 5) {// if bluetooth connected
                                            //Send 'v' for Volume Down
											sprintf(myString, "Volume Down");
											btdata = 'v';
										}
										if (spaceFighterSpeed > 1)
												spaceFighterSpeed--;	// decrease alien speed
								}
						} else if (binData[18] == 'b'){
								if(binData[19] == 'b'){
										if(binData[20] == 'a'){	// decode to Okay		
												if( screenNum == 5) { // if bluetooth connected
                                                    //Send 'K' for OK
													sprintf(myString, "Okay");
													btdata = 'K';
												}
												if(newHighScore){
														if (currCharNum == 3){ // if high score screen
															screenNum = 2; // move to intro screen
														}
												}	else {
														if (screenNum == 2){ // if on main screen
															screenNum = 1;	// move to game screen
															gameOver = false;	// start game
														}
												}
										}
								 else if (binData[20] == 'b'){
											if(binData[21] == 'a'){	// decode to Power button
													if(screenNum == 5) { // If bluetooth connected, return to Home Screen
														screenNum = 2;
														btdata = 'P'; // Send 'P' to power down
														WyzBee_SPPTransfer((uint8 *)peterAddress, &btdata, 1);
														btdata = NULL;
														WyzBee_SPPDisconnet((uint8 *)peterAddress);
													}
											} else if(binData[21] == 'b'){ // decode to INFO button
													num = 'I';		// do nothing
											}	
										}
								}
							}// end bin 18
					}// end bin 17	
		}// end strncmp
	strcpy(binData, resetNum);	// reset code array
}// end function

/******************************************************************************
 ** Decode Space Fighter
 ** Bluetooth
 ******************************************************************************/

void btDecodeSpaceFighter(spaceFighter *myFighter, spaceBullet *myBullets){
	uint8 ret;
	ret = WyzBee_SPPReceive(&btdata, 1); // BT Input
	int currX = myFighter[0].getX(); // X Position of your Fighter
	int currY = myFighter[0].getY(); // Y Position of your Fighter
	spaceBullet *myBullet = 0; // Pointer to Bullets
	
	switch(btdata) {
		case 'C': { //Channel up (+)
			if (newHighScore){	// if high score screen
				if (currCharNum < 3){
					currCharNum++;	// move position of text cursor right
				}						
			}
			else {
				switch(bgColor){ // change bg color
					case BLACK: bgColor = YELLOW; break;
					case YELLOW: bgColor = CYAN; break;
					case CYAN: bgColor = MAGENTA; break;
					case MAGENTA: bgColor = BLACK; break;
					default: bgColor = BLACK;
				}
			}
			break;
		}
		case 'c': { //Channel down (-)		
			if (newHighScore){ // if high score screen
				if (currCharNum >= 0){
					currCharNum--;	// move position of text cursor left
				}	
			}
			break;
		}
		case '0': {		// decode to 0
			if (newHighScore) // if high score screen
				newHiScoreName[currCharNum] = '\0'; // display "space"
			break;
		} 
		case '2': {		// decode to 2
			if (newHighScore){
				// flip through letters ABC for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'A': newHiScoreName[currCharNum] = 'B'; break; 
					case 'B': newHiScoreName[currCharNum] = 'C'; break; 
					case 'C': newHiScoreName[currCharNum] = 'A'; break; 
					default: newHiScoreName[currCharNum] = 'A';
				}
			}
			break;
		}
		case '3': {		// decode to 3
			if (newHighScore){
				// flip through letters DEF for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'D': newHiScoreName[currCharNum] = 'E'; break; 
					case 'E': newHiScoreName[currCharNum] = 'F'; break; 
					case 'F': newHiScoreName[currCharNum] = 'D'; break; 
					default: newHiScoreName[currCharNum] = 'D';
				}
			}
			break;
		}
		case '4': {		// decode to 4
			if (newHighScore){
				// flip through letters GHI for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'G': newHiScoreName[currCharNum] = 'H'; break; 
					case 'H': newHiScoreName[currCharNum] = 'I'; break; 
					case 'I': newHiScoreName[currCharNum] = 'G'; break; 
					default: newHiScoreName[currCharNum] = 'G';
				}
			}
			break;
		}
		case '5': {		// decode to 5
			if (newHighScore){
				// flip through letters JKL for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'J': newHiScoreName[currCharNum] = 'K'; break; 
					case 'K': newHiScoreName[currCharNum] = 'L'; break; 
					case 'L': newHiScoreName[currCharNum] = 'J'; break; 
					default: newHiScoreName[currCharNum] = 'J';
				}
			}
			else {
				// shoot your bullet if on game screen
				myBullet = findFreeBullet(myBullets);
				if (myBullet){
					myBullet->setX(currX);
					myBullet->setY(currY);					
				}			
			}
			break;
		}
		case '6': {		// decode to 6
			if (newHighScore){
				// flip through letters MNO for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'M': newHiScoreName[currCharNum] = 'N'; break; 
					case 'N': newHiScoreName[currCharNum] = 'O'; break; 
					case 'O': newHiScoreName[currCharNum] = 'M'; break; 
					default: newHiScoreName[currCharNum] = 'M';
				}
			}
			break;
		}
		case '7': {		// decode to 7
			if (newHighScore){
				// flip through letters PQRS for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'P': newHiScoreName[currCharNum] = 'Q'; break; 
					case 'Q': newHiScoreName[currCharNum] = 'R'; break; 
					case 'R': newHiScoreName[currCharNum] = 'S'; break;
					case 'S': newHiScoreName[currCharNum] = 'P'; break;							
					default: newHiScoreName[currCharNum] = 'P';
				}
			}
			else {
				// set move your spaceship left, if on game screen
				if (currX - 16 >= 0)
					myFighter[0].setX(currX - 16);
			}
			break;
		}
		case '8': {		// decode to 8
			if (newHighScore) {
				// flip through letters TUV for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'T': newHiScoreName[currCharNum] = 'U'; break; 
					case 'U': newHiScoreName[currCharNum] = 'V'; break; 
					case 'V': newHiScoreName[currCharNum] = 'T'; break; 
					default: newHiScoreName[currCharNum] = 'T';
				}
			}
			break;
		}
		case '9': {		// decode to 9
			if (newHighScore) {
				// flip through letters WXYZ for each press when entering initials
				switch(newHiScoreName[currCharNum]){
					case 'W': newHiScoreName[currCharNum] = 'X'; break; 
					case 'X': newHiScoreName[currCharNum] = 'Y'; break; 
					case 'Y': newHiScoreName[currCharNum] = 'Z'; break; 
					case 'Z': newHiScoreName[currCharNum] = 'W'; break;
					default: newHiScoreName[currCharNum] = 'W';
				}
			} 
			else {		
				// move your spaceship right
				if (currX + 16 < ScreenWIDTH)
					myFighter[0].setX(currX + 16);					
			}
			break;
		}
		case 'V': {	// decode to volume up (+)
			if (spaceFighterSpeed < 32)
				spaceFighterSpeed++;	// increase alien speed
			break;
		}
		case 'v': {	// decode to volume down (-)
			if (spaceFighterSpeed > 1)
				spaceFighterSpeed--;	// decrease alien speed
			break;
		}
		case 'K': {	// decode to Okay		
			if(newHighScore){
				if (currCharNum == 3) { // if high score screen
					screenNum = 2; // move to intro screen
					btReceive = false; 
				}
			}	
			break;
		}
		case 'P': { // decode to Power
			screenNum = 2; // return to home screen
			btReceive = false; // turn infrared back on
			WyzBee_SPPDisconnet((uint8 *)justinAddress);  // disconnect from justinAddress
		}
		default: ;
	} // end switch
	btdata = NULL; //reset btdata
}

int main()
{
	WyzBee_exint_config_t WyzBeeExtIntConfig;
	uint8_t ext_port = 2;
	
	//setup an IR interrupt
	WyzBee_PDL_ZERO(WyzBeeExtIntConfig);
	WyzBeeExtIntConfig.abEnable[ext_port] = FALSE;   // INT2
	WyzBeeExtIntConfig.aenLevel[ext_port] = ExIntFallingEdge;
	WyzBeeExtIntConfig.apfnExintCallback[ext_port] = &Main_ExtIntCallback1;
	WyzBee_Exint_IR_Init(&WyzBeeExtIntConfig);
	
	WyzBeeSpi_Init(&config_stc);
	sys_ticks_init();
	Adafruit_SSD1351 myOled = Adafruit_SSD1351();
	myOled.begin();
	myOled.fillScreen(BLACK);
	
	//uint16 ret;
	//Setup a timer according to the TIMER example on the Wyzbee webpage.
	err_t err;
	Dual_Timer.u8Mode = Dt_FreeRun;
	Dual_Timer.u8PrescalerDiv = Dt_PrescalerDiv16;
	Dual_Timer.u8CounterSize =  Dt_CounterSize32;

	err = WyzBeeDualTimer_Init(Dual_Timer,Dt_Channel0);
	Dt_WriteLoadVal(0x7FFFFFFF, Dt_Channel0);	// start timer at max positive position
	Dt_EnableCount(Dt_Channel0);
	WyzBee_Exint_EnableChannel(ext_port);
	
	//SpaceFigher Initialize
	spaceFighter myFighters[5];
	spaceBullet myBullets[6];
	initializeSpaceFighters(myFighters);
	
	while(1)
	{
		// decode button pressed 
		if(!btReceive) decodeSpaceFighter(myFighters, myBullets);
		// decode BT Data
		else btDecodeSpaceFighter(myFighters, myBullets);

	// allow interrupt after a certain time
	if (prevStamp - Dt_ReadCurCntVal(Dt_Channel0) > 50000){
		release = true;		
	} else {
		release = false;
	}		
	
    // can press button only if interrupt allowed
	if (release){
		WyzBee_Exint_EnableChannel(ext_port);
		// allow interrupt during this time
		for (int i = 0; i<7500000; i++){
		}			
		 WyzBee_Exint_DisableChannel(ext_port);
		} 
 	
	
if (screenNum == 1){	// Game screen	
		myOled.fillScreen(bgColor);
		
		// update positions/display/controls of your ship, aliens, and bullets
		spaceFighters(myOled, myFighters, myBullets);

		// if game is over
		if (gameOver){
			// pause
			for (int i = 0; i<50000000; i++){
			}
			screenNum = 3;	// go to Game Over Screen
		}
	}
else if (screenNum == 2){	// title screen
        // title page design
		myOled.fillScreen(bgColor);
		sprintf(myString, "Space\nFighter");
		pstring(myOled, 16, 16, myString, RED, 3);	
		myOled.fillTriangle(32,127,64,64,96,127,GREEN);
		sprintf(myString, "Press OK to Start!");
		pstring(myOled, 16, 104, myString, WHITE, 1);
		sprintf(myString, "Press 1 for Bluetooth");
		pstring(myOled, 0, 112, myString, WHITE, 1);
		//sprintf(myString, "Press 2 for BTReceive");
		//pstring(myOled, 0, 120, myString, WHITE, 1);

		// if game is over, reset initial conditions
		if(gameOver){
				if (newHighScore){
						// update new high score
						newHighScore = false;
						hiScore = myScore;
						strcpy(hiScoreName, newHiScoreName);				
				}
				myScore = 0;          // resetn score
				initializeSpaceFighters(myFighters); // set to initial conditions		
				gameOver = false;    // allow start of new game
		}
	}
else if (screenNum == 3){	// Game Over Screen
			myOled.fillScreen(BLACK);
			sprintf(myString, "Game Over\nYour Score: %d\n High Score:%d\n", myScore, hiScore);
			pstring(myOled, 0, 0, myString, WHITE, 1);	  	
			pstring(myOled, 0, 33, hiScoreName, WHITE, 1);
	
			// if new high score, allow input of initials
			if (myScore > hiScore){
				newHighScore = true;    // condition to allow button press to input name
				drawHighScore(myOled);  // displays what you are typing
			}
}
		// BT Sender
		else if (screenNum == 4)
		{
			int ret;
			myOled.fillScreen(bgColor);
			ret = btSetup(myOled); // setup Bluetooth Profile
			if (0 != ret) return 0;
			
            // If Justin's Board (Sender) send connection to Peter's Board
			if(!strcmp(localAddress, justinAddress)) { 
				ret = btSender(myOled);
				if(0 != ret) btNoConnect = true;
				else screenNum = 5; // go to Bluetooth Control Screen
				sprintf(myString, "Press Something!");
			}
            // If Peter's Board (Receiver) wait for Justin's connection request
			else if(!strcmp(localAddress, peterAddress)){
				ret = btReceiver(myOled);
				btReceive = true;
				screenNum = 1;
			}
		}
		// Bluetooth Control Screen
		else if (screenNum == 5)
		{
			myOled.fillScreen(bgColor);
			pstring(myOled, 0, 0, "Sending Command:\n\n\n\n\n\n\n\nPress Power to Exit.", GREEN, 1);
			pstring(myOled, 0, 16, myString, RED, 1); // print number from Infrared Scanner
			WyzBee_SPPTransfer((uint8 *)peterAddress, &btdata, 1); // send data to receiver
			btdata = NULL; // reset btdata
			
		}
	}
}
/*
 *********************************************************************************************************
 *                                           END
 *********************************************************************************************************
 */