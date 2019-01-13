// 2019/01/06 

// 2018/12/30 key layout for Delete, Arrows, Home, End, Page Up/Down
//         DELETE
//         [delete]
//         (insert)
// 
//         CLEAR     REPEAT
//         [up]      [down]
//         (pg up) (pg down)
// 
//         HEREIS     BREAK
//         [left]     [right]
//         (home)     (end)

// Keyboard LED pins
#define BLUE 21
#define GREEN 22
#define YELLOW 23

// keyboard physical matrix row/column pins
#define NUMROWS 4
#define NUMCOLS 16
#define NUMKEYS NUMROWS*NUMCOLS

#define ROW0 0
#define ROW1 1
#define ROW2 2
#define ROW3 3
#define COL0   4
#define COL1   5
#define COL2   6
#define COL3   7
#define COL4   8
#define COL5   9
#define COL6  10
#define COL7  11
#define COL8  12
#define COL9  14
#define COL10 15
#define COL11 16
#define COL12 17
#define COL13 18
#define COL14 19
#define COL15 20

// map unique keyboard keys to exiting USB Keycodes
#define  KEY_LEFT_BLANK  KEY_LEFT_ALT
#define  KEY_RIGHT_BLANK KEY_LEFT_ALT   // not really used, overriden by mappings
#define  KEY_SHIFT_LOCK  KEY_LEFT_GUI
#define  KEY_LINEFEED    KEY_ENTER
#define  KEY_CLEAR       KEY_DOWN
#define  KEY_REPEAT      KEY_UP
#define  KEY_HEREIS      KEY_LEFT
#define  KEY_BREAK       KEY_RIGHT
#define  KEY_COLON       KEY_SEMICOLON
#define  KEY_AT          KEY_2          // not really used, overriden by mappings
#define  KEY_HAT         KEY_6          // not really used, overriden by mappings
#define  KEY_NULL        KEY_ENTER      //  key does not really existing 

// teensy library calls this the tilde key, I prefer backquote
// just making an alias for the two names
#define  KEY_BACKQUOTE KEY_TILDE

// we need to remember which key we are processing (pressed)
// this flag is used to tell us no key is currently pressed
#define NOKEY 0xffff
// mask for virtual modifer keys
#define xSHIFT 0x01000000
#define xCTRL  0x02000000
#define xALT   0x04000000
#define xMETA  0x08000000
#define xMACRO 0x10000000           // not used.... yet

// matriX KEYcodes  -- xkeys
#define  xKEY_ESC          0
#define  xKEY_1            1
#define  xKEY_2            2
#define  xKEY_3            3
#define  xKEY_4            4
#define  xKEY_5            5
#define  xKEY_6            6
#define  xKEY_7            7
#define  xKEY_8            8
#define  xKEY_9            9
#define  xKEY_0            10
#define  xKEY_MINUS        11
#define  xKEY_AT           12
#define  xKEY_HAT          13
#define  xKEY_BACKSPACE    14
#define  xKEY_RIGHT_BLANK  15
#define  xKEY_TAB          16
#define  xKEY_Q            17
#define  xKEY_W            18
#define  xKEY_E            19
#define  xKEY_R            20
#define  xKEY_T            21
#define  xKEY_Y            22
#define  xKEY_U            23
#define  xKEY_I            24
#define  xKEY_O            25
#define  xKEY_P            26
#define  xKEY_LEFT_BRACE   27
#define  xKEY_RIGHT_BRACE  28
#define  xKEY_BACKSLASH    29
#define  xKEY_DELETE       30
#define  xKEY_NULL         31
#define  xKEY_LEFT_BLANK   32
#define  xKEY_SHIFT_LOCK   33
#define  xKEY_A            34
#define  xKEY_S            35
#define  xKEY_D            36
#define  xKEY_F            37
#define  xKEY_G            38
#define  xKEY_H            39
#define  xKEY_J            40
#define  xKEY_K            41
#define  xKEY_L            42
#define  xKEY_SEMICOLON    43
#define  xKEY_COLON        44
#define  xKEY_LINEFEED     45
#define  xKEY_CLEAR        46
#define  xKEY_REPEAT       47
#define  xKEY_CTRL         48
#define  xKEY_LEFT_SHIFT   49
#define  xKEY_Z            50
#define  xKEY_X            51
#define  xKEY_C            52
#define  xKEY_V            53
#define  xKEY_B            54
#define  xKEY_N            55
#define  xKEY_M            56
#define  xKEY_COMMA        57
#define  xKEY_PERIOD       58
#define  xKEY_SLASH        59
#define  xKEY_RIGHT_SHIFT  60
#define  xKEY_HEREIS       61
#define  xKEY_BREAK        62
#define  xKEY_SPACE        63

// #define  xKEY_META      xKEY_SHIFT_LOCK

// physical keycode for modifier keys
#define MOD_MACRO   xKEY_RIGHT_BLANK
#define MOD_ALT     xKEY_LEFT_BLANK 
#define MOD_META    xKEY_SHIFT_LOCK 
#define MOD_CTRL    xKEY_CTRL 
#define MOD_SHIFTL  xKEY_LEFT_SHIFT 
#define MOD_SHIFTR  xKEY_RIGHT_SHIFT





// #define  xKEY_INSERT       64  // virutal key <meta><delete>
// #define  xKEY_HOME         65  // virutal key <meta><hereis>
// #define  xKEY_PAGE_UP      66  // virutal key <meta><clear>
// #define  xKEY_END          67  // virtual key <meta><break>
// #define  xKEY_PAGE_DOWN    68  // virtual key <meta><repeat>
// #define  xKEY_QUOTE        69
// #define  xKEY_EQUAL        70  // virtual key <shift><minus>
// #define  xKEY_TILDE        71  // virtual key <shift><tilda>
// #define  xKEY_F1           72  // virtual key <meta><1>
// #define  xKEY_F2           73  // virtual key <meta><2>
// #define  xKEY_F3           74  // virtual key <meta><3>
// #define  xKEY_F4           75  // virtual key <meta><4>
// #define  xKEY_F5           76  // virtual key <meta><5>
// #define  xKEY_F6           77  // virtual key <meta><6>
// #define  xKEY_F7           78  // virtual key <meta><7>
// #define  xKEY_F8           79  // virtual key <meta><8>
// #define  xKEY_F9           80  // virtual key <meta><9>
// #define  xKEY_F10          81  // virtual key <meta><0>
//
// #define NUMXKEYS 81
// extra name for meta key
