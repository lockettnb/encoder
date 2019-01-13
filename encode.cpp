// encoder -- teensy USB keyboard encoder
//
// 2018/03/13 created
// 2019/01/03 extensive updates over the xmas holidays
//

#include <stdio.h>
#include <string.h>
#include "Arduino.h"
#include "usb_serial.h"
#include "usb_keyboard.h"
// #include "HardwareSerial.h"
// #include <avr/io.h>
// #include <avr/interrupt.h>
#include "encode.h"

#define FALSE 0
#define TRUE 1


// ++++++++++ DEBUG STUFF +++++++++++++
// serial port is used during debug 
#ifdef USB_SERIAL
    #define USBSERIAL TRUE
    #define USBKEYBOARD FALSE
    class usb_keyboard_class : public Print
    {
    public:
        void begin(void) { }
        void end(void) { }
        virtual size_t write(uint8_t c) { return 1; }
        void press(uint16_t n) { }
        void release(uint16_t n) { }
        void releaseAll(void) { }
    };
#endif

#ifdef USB_KEYBOARDONLY
    #define USBSERIAL FALSE 
    #define USBKEYBOARD TRUE
#endif
// +++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++ KEYBOARD MATRIX I/O PINS +++++++++++++
// keyboard matrix pins -- rows (outputs) X columns (inputs)
int rows[NUMROWS] = { ROW0,  ROW1,  ROW2,  ROW3 } ;
int cols[NUMCOLS] = { COL0, COL1, COL2, COL3, COL4, COL5, COL6, COL7, COL8,\
                 COL9, COL10, COL11, COL12, COL13, COL14, COL15 };
// +++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++ USB KEYCODES +++++++++++++
// matrix keycode to USB keycode mapping 
// some of these are unique to my keyboard and are aliased to existing 
// USB keycodes (or at least teensy library defines)
int usb_keycode[NUMKEYS] = { 
       KEY_ESC, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, 
       KEY_9, KEY_0, KEY_MINUS, KEY_AT, KEY_HAT, KEY_BACKSPACE, KEY_RIGHT_BLANK,

       KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I,
       KEY_O, KEY_P, KEY_LEFT_BRACE, KEY_RIGHT_BRACE, KEY_BACKSLASH, KEY_DELETE,
       KEY_NULL,

       KEY_LEFT_BLANK, KEY_SHIFT_LOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H,
       KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_COLON, KEY_LINEFEED,
       KEY_CLEAR, KEY_REPEAT,

       KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_Z, KEY_X, KEY_C, KEY_V,
       KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH,
       KEY_RIGHT_SHIFT, KEY_HEREIS, KEY_BREAK, KEY_SPACE
   };
// +++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++ KEYBOARD MATRIX MAPPINGS +++++++++++++
// matrix_keymap has eight key sequences/mappings for each key
// The regular KEY and the key with seven modifiers:
//    SHIFT, CTRL, ALT, META, CTRL+SHIFT, ALT+SHIFT, META+SHIFT
//
// Most keys map one-for-one from a matrix key press to a USB keycode.
// For example, if you press the A key we send KEY_A to the USB connection. 
//
// However, some keys like the shift-2 key has a "(double quote) symbol not the 
// regular @ (AT) symbol.  Is this case we need to send KEY_QUOTE keycode. 
// This is the trickest bit of the whole keymapping process.

// i for index
#define  iKEY        0
#define  iSHIFT      1
#define  iCTRL       2
#define  iALT        3
#define  iMETA       4
#define  iCTRLSHIFT  5
#define  iALTSHIFT   6
#define  iMETASHIFT  7
#define  iMACRO      9

uint32_t keymatrix[NUMKEYS][8]; 
// +++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++ KEYBOARD STATE +++++++++++++
typedef  enum {
    released,       //   key is up
    debounce_down,  //   detected key is down, wait one scan cycle for debounce
    pressed_event,  //   key is down and ready for press processing 
    pressed,        //   key is down and USB send is complete
    debounce_up,    //   key is up (released),  wait one scan cycle for debounce
    released_event  //   key is up  and ready release processing 
 } State;

State keystate[NUMKEYS];
usb_keyboard_class myKeyboard ;
// +++++++++++++++++++++++++++++++++++++++++++++++++


// ++++++++++ LOG MESSAGES DEFINITIONS & VARS +++++++++++++
#define LOGSIZE 12 
#define LOGMSG 64
char logs[LOGSIZE][LOGMSG];
// +++++++++++++++++++++++++++++++++++++++++++++++++

void logclear(void)
{
 int i;

//  initalize log messages to nothing
    for (i=0; i<LOGSIZE; i++)
        logs[i][0]='\0';

}

void logmsg(char message[])
{
 int i;

    for(i=LOGSIZE; i>0; i--) {
        strcpy(logs[i], logs[i-1]);
    }

    strcpy(logs[0], message);
}

// twinkle the LEDs during startup
void twinkle(int wait_time) 
{
 int save_blue;
 int save_green;
 int save_yellow;
 int led;
 int tm=0;
 int twink=100;

    save_blue   = digitalRead(BLUE);
    save_green  = digitalRead(GREEN);
    save_yellow = digitalRead(YELLOW);

    digitalWrite(BLUE, LOW);   
    digitalWrite(GREEN, LOW);   
    digitalWrite(YELLOW, LOW);   

    while (tm < wait_time){
        led=digitalRead(YELLOW);
        digitalWrite(YELLOW, !led);
        tm=tm+twink; delay(twink);
        led=digitalRead(GREEN);
        digitalWrite(GREEN, !led);
        tm=tm+twink; delay(twink);
        led=digitalRead(BLUE);
        digitalWrite(BLUE, !led);
        tm=tm+twink; delay(twink);
    }

    digitalWrite(BLUE, save_blue);   
    digitalWrite(GREEN, save_green);   
    digitalWrite(YELLOW, save_yellow);

}

void clear_screen(void) 
{
 char buf[LOGMSG];
// clear cursor to eos ESC[0J
// clear screen ESC[2J
// <ESC>[{ROW};{COLUMN}H

    if(! USBSERIAL) return;

    sprintf(buf, "\e[2J");
    Serial.print(buf);
    Serial.flush();
}

void clear_eos(void) 
{
 char buf[LOGMSG];
// clear cursor to eos ESC[0J

    if(! USBSERIAL) return;

    sprintf(buf, "\e[0J");
    Serial.print(buf);
    Serial.flush();
}

uint16_t keynum(int row, int col)
{
    return (row * NUMCOLS) + (col);
}

void display_keymap(uint16_t k)
{
 char buf[LOGMSG];
 int i;

    sprintf(buf, "\e[0;0H KEY: %i", k);
    Serial.println(buf);

    for(i=0; i<8; i++) {
int code;
        code =keymatrix[k][i] & 0x00ff;
        sprintf(buf, "%2i: %4i %12li %08lx", i, code , keymatrix[k][i], keymatrix[k][i]);
        Serial.println(buf);
    }
}

void display(void)
{
 int r,c, key;
 char buf[LOGMSG];
 int i;

    if(!USBSERIAL) return;

    sprintf(buf, "\e[10;0H");
    Serial.println(buf);
    clear_eos();

    for(i=0; i<LOGSIZE; i++) {
        sprintf(buf, "%i>> %s", i, logs[i]);
        Serial.println(buf);
    }

    sprintf(buf, "\e[22;0H");
    Serial.println(buf);
    Serial.println("scan     0 1 2 3 4 5 6 7 8 9 A B C D E F");
    for (r=0; r<NUMROWS; r++) {
        sprintf(buf, "row <%i> ", r);
        Serial.print(buf);
        for (c=0; c<NUMCOLS; c++) {
            key=keynum(r,c);
            sprintf(buf, " %i", keystate[key]);
            Serial.print(buf);
//             Serial.flush();
        }
    Serial.println("");
    Serial.flush();
    }
    Serial.println("");
    Serial.println("");
}




void create_usb_mapping(void) 
{
 int k;

    for(k=0; k<NUMKEYS; k++) {
        keymatrix[k][iKEY]       = usb_keycode[k];
        keymatrix[k][iSHIFT]     = xSHIFT | usb_keycode[k];
        keymatrix[k][iCTRL]      = xCTRL  | usb_keycode[k];
        keymatrix[k][iALT]       = xALT   | usb_keycode[k];
        keymatrix[k][iMETA]      = xMETA  | usb_keycode[k];
        keymatrix[k][iCTRLSHIFT] = xCTRL  | xSHIFT | usb_keycode[k];
        keymatrix[k][iALTSHIFT]  = xALT   | xSHIFT | usb_keycode[k];
        keymatrix[k][iMETASHIFT] = xMETA  | xSHIFT | usb_keycode[k];
    }

    // exceptions to simple mapping
    keymatrix[xKEY_1][iMETA]  = KEY_F1;     // meta+digit --> funtion key
    keymatrix[xKEY_2][iMETA]  = KEY_F2;
    keymatrix[xKEY_3][iMETA]  = KEY_F3;
    keymatrix[xKEY_4][iMETA]  = KEY_F4;
    keymatrix[xKEY_5][iMETA]  = KEY_F5;
    keymatrix[xKEY_6][iMETA]  = KEY_F6;
    keymatrix[xKEY_7][iMETA]  = KEY_F7;
    keymatrix[xKEY_8][iMETA]  = KEY_F8;
    keymatrix[xKEY_9][iMETA]  = KEY_F9;
    keymatrix[xKEY_0][iMETA]  = KEY_F10;

    keymatrix[xKEY_2][iSHIFT]     = xSHIFT | KEY_QUOTE;             // shift+2 --> shift-quote
    keymatrix[xKEY_2][iCTRLSHIFT] = xCTRL  | xSHIFT | KEY_QUOTE;    // shift+2 --> shift-quote

    keymatrix[xKEY_6][iSHIFT]     = xSHIFT  | KEY_7 ;   // shift+6 --> shift-7 (&)
    keymatrix[xKEY_7][iSHIFT]     = KEY_BACKQUOTE;      // shift+7 --> back quote 
    keymatrix[xKEY_8][iSHIFT]     = xSHIFT | KEY_9;     // shift+8 --> shift+9 (left-bracket)
    keymatrix[xKEY_9][iSHIFT]     = xSHIFT | KEY_0;     // shift+9 --> shift+0 (right-bracket)

    keymatrix[xKEY_MINUS][iSHIFT]     = KEY_EQUAL;           // shift+minus --> equals 
    keymatrix[xKEY_MINUS][iCTRLSHIFT] = xCTRL | KEY_EQUAL;   // shift+minus --> equals 

    keymatrix[xKEY_AT][iKEY]      = xSHIFT  | KEY_2;         // shift+@ --> at @
    keymatrix[xKEY_AT][iSHIFT]    = KEY_QUOTE;               // shift+@ --> quote

    keymatrix[xKEY_HAT][iKEY]       = xSHIFT | KEY_6;
    keymatrix[xKEY_HAT][iSHIFT]     = xSHIFT | KEY_BACKQUOTE;
    keymatrix[xKEY_HAT][iCTRLSHIFT] = xCTRL  | xSHIFT | KEY_BACKQUOTE;

    keymatrix[xKEY_SEMICOLON][iSHIFT]     = xSHIFT | KEY_EQUAL;             // plus sign
    keymatrix[xKEY_SEMICOLON][iCTRLSHIFT] = xCTRL  | xSHIFT | KEY_EQUAL;    // crtl plus sign

    keymatrix[xKEY_COLON][iKEY]   = xSHIFT | KEY_SEMICOLON; 
    keymatrix[xKEY_COLON][iSHIFT] = xSHIFT | KEY_8;

    keymatrix[xKEY_DELETE][iMETA]  = KEY_INSERT;
    keymatrix[xKEY_CLEAR][iMETA]   = KEY_PAGE_UP;
    keymatrix[xKEY_REPEAT][iMETA]  = KEY_PAGE_DOWN;
    keymatrix[xKEY_HEREIS][iMETA]  = KEY_HOME;
    keymatrix[xKEY_BREAK][iMETA]   = KEY_END;
}



void setup() {
  int i;

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BLUE, OUTPUT);      digitalWrite(BLUE, LOW);   
    pinMode(GREEN, OUTPUT);     digitalWrite(GREEN, LOW);   
    pinMode(YELLOW, OUTPUT);    digitalWrite(YELLOW, LOW);   

    for(i=0; i<NUMROWS; i++) {
      pinMode(rows[i], OUTPUT);
    };

//  we read from columns; one for released key, zero for presssed key
    for(i=0; i<NUMCOLS; i++) {
      pinMode(cols[i], INPUT_PULLUP);
    };

//  initialize each key state to "released"
    for (i=0; i<NUMKEYS; i++){
        keystate[i]=released;
    }

//  keyboard matrix to usb mappings 
    create_usb_mapping();

    if(USBKEYBOARD) myKeyboard.begin();
    if(USBSERIAL) Serial.begin(115200);


} //setup

int scan(void)
{
 int key_event = FALSE;
 int r, c;
 int scan_key;      // specific key from the row/col matrix
 uint16_t key;      // key code (number) for row/col matrix

    for (r=0; r<NUMROWS; r++) {
        digitalWrite(rows[r], LOW);

        for (c=0; c<NUMCOLS; c++) {
            scan_key=digitalRead(cols[c]);
            key=keynum(r,c);

            switch (keystate[key]) { 
                case released :
                    if(scan_key == 0) {
                        keystate[key] = debounce_down;
                    }
                    break;
                case debounce_down :
                    if(scan_key == 0) {
                        keystate[key] = pressed_event; 
                        key_event=TRUE;
                    }
                    if(scan_key == 1) {
                        keystate[key] = released; 
                    }
                    break;
                 case pressed_event:
                    if(scan_key == 1) {
                        keystate[key] = released; 
                    }
                    break;

                case pressed : 
                    if(scan_key == 1) {
                        keystate[key] = debounce_up; 
                    }
                    break;
                case debounce_up : 
                    if(scan_key == 1) {
                        keystate[key] = released_event; 
                        key_event=TRUE;
                    }
                    if(scan_key == 0) {
                        keystate[key] = pressed; 
                    }
                    break;
                 case released_event:
                    if(scan_key == 0) {
                        keystate[key] = pressed; 
                    }
                    break;
 
            } //switch
        }
        digitalWrite(rows[r], HIGH);
    } //for
    return key_event;
} //scan 

// translate the modifier mask into a index into the key mapping array
int maskindex(uint32_t mask)
{
 int shift, ctrl, alt, meta, macro;
 char buf[LOGMSG];

    shift = (xSHIFT & mask) >> 24;
    ctrl  = (xCTRL  & mask) >> 25;
    alt   = (xALT   & mask) >> 26;
    meta  = (xMETA  & mask) >> 27;
    macro = (xMACRO & mask) >> 28;

    sprintf(buf, "Mask Index: 0x%08lx: s%04x c%04x a%04x m%04x %04x", mask, shift, ctrl, alt, meta, macro);
    logmsg(buf);

    if(ctrl+shift == 2)  return iCTRLSHIFT; 
    if(alt+shift  == 2)  return iALTSHIFT; 
    if(meta+shift == 2)  return iMETASHIFT; 
    if(shift      == 1)  return iSHIFT; 
    if(ctrl       == 1)  return iCTRL; 
    if(alt        == 1)  return iALT; 
    if(meta       == 1)  return iMETA; 
    if(macro      == 1)  return iMACRO; 
    return iKEY;
}

void releasekey(uint16_t k, uint32_t mask)
{
 char buf[LOGMSG];
 uint16_t key;
 uint16_t modifier;
 uint16_t mod_idx;
 uint32_t keymapping;

// translate/map matrix key to USB keys
    mod_idx = maskindex(mask);
    keymapping = keymatrix[k][mod_idx]; 
    key = keymapping & 0xffff;
    modifier = (keymapping & 0xffff0000 ) >> 24;

    if(USBSERIAL){
        sprintf(buf, "KEY released: %i 0x%02x; mask 0x%08lx", k, k, mask);
        logmsg(buf); 
        return;
    }
    if(USBKEYBOARD) {           // release keys in reverse order
        myKeyboard.release(key);

        switch (modifier) {
        case 1:
            myKeyboard.release(MODIFIERKEY_LEFT_SHIFT);
            break;
        case 2:
            myKeyboard.release(MODIFIERKEY_LEFT_CTRL);
            break;
        case 4:
            myKeyboard.release(MODIFIERKEY_ALT);
            break;
        case 8:
            myKeyboard.release(MODIFIERKEY_GUI);
            break;
        case 3:
            myKeyboard.release(MODIFIERKEY_LEFT_CTRL);
            myKeyboard.release(MODIFIERKEY_LEFT_SHIFT);
            break;
        case 5:
            myKeyboard.release(MODIFIERKEY_LEFT_ALT);
            myKeyboard.release(MODIFIERKEY_LEFT_SHIFT);
            break;
        case 9:
            myKeyboard.release(MODIFIERKEY_LEFT_GUI);
            myKeyboard.release(MODIFIERKEY_LEFT_SHIFT);
            break;
        } // switch 
    }
}

void presskey(uint16_t k, uint32_t mask)
{
 char buf[LOGMSG];
 uint16_t key;
 uint16_t modifier;
 uint16_t mod_idx;
 uint32_t keymapping;

// translate/map matrix key to USB keys
    mod_idx = maskindex(mask);
    keymapping = keymatrix[k][mod_idx]; 
    key = keymapping & 0xffff;
    modifier = (keymapping & 0xffff0000 ) >> 24;

    sprintf(buf, "KEY pressed : %i 0x%02x for key/modindex: %i/0x%04x", key, key, k, mod_idx);
    logmsg(buf);

    display_keymap(k);

    if(USBSERIAL){
        return;
    }
    if(USBKEYBOARD) {
        switch (modifier) {
        case 1:
            myKeyboard.press(MODIFIERKEY_LEFT_SHIFT);
            break;
        case 2:
            myKeyboard.press(MODIFIERKEY_LEFT_CTRL);
            break;
        case 4:
            myKeyboard.press(MODIFIERKEY_ALT);
            break;
        case 8:
            myKeyboard.press(MODIFIERKEY_GUI);
            break;
        case 3:
            myKeyboard.press(MODIFIERKEY_LEFT_SHIFT);
            myKeyboard.press(MODIFIERKEY_LEFT_CTRL);
            break;
        case 5:
            myKeyboard.press(MODIFIERKEY_LEFT_SHIFT);
            myKeyboard.press(MODIFIERKEY_LEFT_ALT);
            break;
        case 9:
            myKeyboard.press(MODIFIERKEY_LEFT_SHIFT);
            myKeyboard.press(MODIFIERKEY_LEFT_GUI);
            break;
        } // switch 
        myKeyboard.press(key);
    }
}


void ack_event(uint16_t key)
{
 char buf[LOGMSG];

    if(keystate[key] == pressed_event) {
        keystate[key] = pressed;
        sprintf(buf, "press ACK   : %i 0x%02x ", key, key);
        logmsg(buf);
    }

    if(keystate[key] == released_event) {
        keystate[key] = released;
        sprintf(buf, "release ACK : %i 0x%02x ", key, key);
        logmsg(buf);
    }
}

// return TRUE if modifier key, also set modifier mask
int ismodifier(uint16_t keycode, uint32_t *mask)
{
 int ismod = FALSE;
 char buf[LOGMSG];

    *mask = 0;  // default to no modifier mask;

    switch (keycode) { 
        case MOD_SHIFTL:
        case MOD_SHIFTR:
            *mask = xSHIFT; 
            ismod = TRUE;
        break;

        case MOD_CTRL:
            *mask = xCTRL; 
            ismod = TRUE;
        break;

        case MOD_ALT:
            *mask = xALT; 
            ismod = TRUE;
        break;

        case MOD_META:
            *mask = xMETA; 
            ismod = TRUE;
        break;

        case MOD_MACRO:
            *mask = xMACRO; 
            ismod = TRUE;
        break;
    } //switch

    if(ismod) {
       sprintf(buf, "MOD key %i 0x%02x ; key mask : %lu 0x%04lx", keycode, keycode, *mask, *mask );
        logmsg(buf);
    }
    return ismod;
}


int main(void)
{
// int rc;
// int led;
 char buf[LOGMSG];
int k;
// uint16_t key;
uint32_t event_mask;
uint16_t current_key = NOKEY;
uint32_t mod_mask = 0;
elapsedMillis display_timer; // variable automatically increase as time elapses

    setup();
    logclear();
    twinkle(5000);
    clear_screen();
    display();
    display_timer=0;

    while(1) {
        scan();
        if(USBSERIAL) {
            delay(350);
            display(); 
            delay(350);
            display_timer=0;
        }
        delay(10);          // normal 10msec delay, good for debounce

        for(k=0; k<NUMKEYS; k++) {  
            if(keystate[k] == pressed_event){
                if(ismodifier(k, &event_mask)) {  //  modifier key
                    mod_mask |= event_mask;
                    ack_event(k);
                } else if (current_key == NOKEY) {  // next key to process
                    current_key = k;
                    presskey(k, mod_mask);
                    ack_event(k);
                } else {                            // skip anything else
                    sprintf(buf, "KEY Pressed: %i pending on key %i", k, current_key); 
                    logmsg(buf);
                }
            }

            if(keystate[k] == released_event){ // check for modifier key release
                if(ismodifier(k, &event_mask)) {
                    mod_mask &= ~event_mask;
                    ack_event(k);
                } else if (k == current_key) {
                    current_key = NOKEY;
                    releasekey(k, mod_mask);
                    ack_event(k);
                } else {
                    sprintf(buf, "KEY Released: pending another key" );
                    logmsg(buf);
                }
            }
        } //for
    } //while

} //main

