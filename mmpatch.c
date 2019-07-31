//
// Mega Man (DOS) and Mega Man 3 (DOS) patch utility
// Version 1
// Brad Smith, 2019
// http://rainwarrior/ca
//
// C99 source file
//

#include <stdio.h>
#include <stdint.h>

const int debug = 0; // 1 = lists each patch applied
// TEST 1 will operate on both 1MM.EXE and 3MM.EXE
#define TEST 0

typedef uint32_t     uint32;
typedef uint16_t     uint16;
typedef uint8_t      uint8;
typedef unsigned int uint;

// MM.EXE CRC32 to identify which file is present
const uint32 CRC_MM1 = 0xAEA06825;
const uint32 CRC_MM3 = 0x06C09829;

#if !TEST
#define FILE_MM1    "MM.EXE"
#define FILE_MM3    "MM.EXE"
#else
#define FILE_MM1    "1MM.EXE"
#define FILE_MM3    "3MM.EXE"
#endif

#define FILE_CRC    "MM.EXE"
#define OUT_MM1     "MM1.EXE"
#define OUT_MM3CGA  "MM3CGA.EXE"
#define OUT_MM3EGA  "MM3EGA.EXE"

#define default_speed   3

typedef struct
{
	uint addr;
	uint length;
	const uint8* data;
} patch;

#define LENGTH(x) (sizeof(x)/sizeof(x[0]))

#define WORD(x)        ((x)&0xFF),(((x)>>8)&0xFF)
#define CALL(src,dst)  (0xE8),WORD((dst)-(src)-3)
#define JMP(src,dst)   (0xE9),WORD((dst)-(src)-3)

const uint8 six[] = { 6 };

// New data from the patch is placed over top of old code.
// For Mega Man 1, there was non-functioning dead code for a 16-colour
// Tandy mode that was never used. For Meag Man 3, either the CGA or EGA
// mode will be disabled so that its code can be overwritten.
// Each video mode has a set of subroutines, and their entry points
// were used merely for my own convenience. In some cases there
// is a small offset from the entry point to avoid a segment
// relocation value that would get overwritten when loading the executable.

// Additionally, the executables had been packed by MASM EXEPACK.
// The following utility was used to unpack them for analysis:
// https://github.com/w4kfu/unEXEPACK

// Each patch applied has a segment address where it will reside in memory when loaded,
// and also a file offset where it resides in the original executable file.

//
// Mega Man 1 patch
//

// slowdown routine to delay a specified number of frames
// location: unused tandy video function 0 (+12 offset, 67 bytes until function 1)
#define mm1_slow_addr   0x219A
#define mm1_slow_file   0x1A08
const uint8 mm1_slow[] = {
	0x9C,                                 // pushf
	0x50,                                 // push ax
	0x51,                                 // push cx
	0x52,                                 // push dx
	0xB9, default_speed, 0x00,            // mov cx, speed
	0xE3, 15,                             // jcxz end
	0xBA, WORD(0x03DA),                   // mov dx, 03DAh
	                                      //wait1: ; wait until not in vertical retrace
	0xEC,                                 // in al, dx
	0xA8, 0x08,                           // test al, 8
	0x74, 0xFB,                           // jz wait1
	                                      //wait2: ; wait until vertical retrace finished
	0xEC,                                 // in al, dx
	0xA8, 0x08,                           // test al, 8
	0x75, 0xFB,                           // jnz wait2
	0xE2, 0xF4,                           // loop wait1
	                                      //end:
	0x5A,                                 // pop dx
	0x59,                                 // pop cx
	0x58,                                 // pop ax
	0x9D,                                 // popf
	0x2E, 0x80, 0x3E, WORD(0x1149), 0x00, // cmp cs:1149h, 0 ; joystick enabled
	0xC3,                                 // retn
};
// patch just before a joystick input poll to insert the delay,
// replacing "cmp cs:1149h, 0" with a call.
#define mm1_slow0_addr   0x5390
#define mm1_slow0_file   0x4BFE
const uint8 mm1_slow0[] = { CALL(mm1_slow0_addr,mm1_slow_addr), 0x90, 0x90, 0x90 };
// only the main game loop is patched, but several other input poll candidates were found,
// searching for uses of cs:1149h which is the joystick setting flag:
// address (file offset)
// 49B6h (4224)
// 4A23h (4291)
// 4B07h (4375) - followed by joystick calibration rather than polling
// 4DA5h (4613)
// 4E67h (47D5) - not followed by a poll
// 50DFh (494D) - setup menu initializing setting
// 50EDh (495B) - setup menu initializing setting
// 5284h (4AF2) - followed by calibration rather than poll
// 52C5h (4B33)
// 53EDh (4C5B)

// text table for a revised setup menu
// location: unused tandy video function 1 (154 bytes until function 2)
#define mm1_table_addr   0x21DD
#define mm1_table_file   0x1A4B
const uint8 mm1_table[] = {
	// table of strings for the slowdown setting
	0x28,0x09,1,'0',
	0x2A,0x09,1,'1',
	0x2C,0x09,1,'2',
	0x2E,0x09,1,'3',
	0x30,0x09,1,'4',
	0x32,0x09,1,'5',
	0x34,0x09,1,'6',
	0x36,0x09,1,'7',
	0x38,0x09,1,'8',
	0x3A,0x09,1,'9',
	0x18,0x09,9,'S','l','o','w','d','o','w','n',':',
	default_speed, 9,
	WORD(mm1_table_addr+(4*10)),
	WORD(mm1_table_addr+(4*0)),
	WORD(mm1_table_addr+(4*1)),
	WORD(mm1_table_addr+(4*2)),
	WORD(mm1_table_addr+(4*3)),
	WORD(mm1_table_addr+(4*4)),
	WORD(mm1_table_addr+(4*5)),
	WORD(mm1_table_addr+(4*6)),
	WORD(mm1_table_addr+(4*7)),
	WORD(mm1_table_addr+(4*8)),
	WORD(mm1_table_addr+(4*9)),
	2,3, // default (2 = VGA), maximum (3 = TANDY)
	WORD(0x107C), // Graphics Card:
	WORD(0x108D), // CGA
	WORD(0x1093), // EGA
	WORD(0x1099), // VGA
	WORD(0x109F), // TANDY
	0,1, // animation
	WORD(0x10A7), // Animation:
	WORD(0x10B4), // ON
	WORD(0x10B9), // OFF
	0,1, // masking
	WORD(0x10BF), // Masking:
	WORD(0x10CA), // ON
	WORD(0x10CF), // OFF
	0,1, // sound
	WORD(0x10D5), // Sound:
	WORD(0x10DE), // ON
	WORD(0x10E3), // OFF
	1,1, // joystick (default off)
	WORD(0x10E9), // Joystick:
	WORD(0x10F5), // ON
	WORD(0x10FA), // OFF
	0,0, // start
	WORD(0x1100), // Start Game
	// settings pointer table, the "default" above also store the current option
	WORD(mm1_table_addr+52+(2*0)),
	WORD(mm1_table_addr+52+(2*(12))),
	WORD(mm1_table_addr+52+(2*(12+6))),
	WORD(mm1_table_addr+52+(2*(12+6+4))),
	WORD(mm1_table_addr+52+(2*(12+6+4+4))),
	WORD(mm1_table_addr+52+(2*(12+6+4+4+4))),
	WORD(mm1_table_addr+52+(2*(12+6+4+4+4+4))),
};
// the original text table and settings pointer table
// resided at 110Dh and 113Dh, and some references to them need to be replaced.
const uint8 mm1_table0[] = { WORD(mm1_table_addr+52) };    // 110Dh
const uint8 mm1_table1[] = { WORD(mm1_table_addr+52+72) }; // 113Dh

// a routine to copy the new settings table results into the original settings table
// and otherwise finalize the new slowdown setting
// location: unused tandy video function 2 (+3 offset, max 96 bytes)
#define mm1_settings_addr   0x2277
#define mm1_settings_file   0x1AE5
// original settings code location to redirect
#define mm1_settings0_addr   0x50AD
#define mm1_settings0_file   0x491B
const uint8 mm1_settings[] = {
	0x9C,                                     // pushf
	0x50,                                     // push ax
	0x53,                                     // push bx
	0x51,                                     // push cx
	0x1E,                                     // push ds
	0x8C, 0xC8,                               // mov ax, cs
	0x8E, 0xD8,                               // mov ds, ax
	0xB9, 0x06, 0x00,                         // mov cx, 6
	                                          //repeat: ; count down cx from 6 to 1
	0x89, 0xCB,                               // mov bx, cx
	0xD1, 0xE3,                               // shl bx, 1
	0x8B, 0x9F, WORD(mm1_table_addr+52+72),   // mov bx, [bx+new_settings_table]
	0x8A, 0x07,                               // mov al, [bx]
	0x89, 0xCB,                               // mov bx, cx
	0xD1, 0xE3,                               // shl bx, 1
	0x8B, 0x9F, WORD(0x113D-2),               // mov bx, [bx+old_settings_table-2]
	0x88, 0x07,                               // mov [bx], al
	0xE2, 0xEA,                               // loop repeat
	0x8B, 0x1E, WORD(mm1_table_addr+52+72),   // mov bx, new_settings_table ; extra entry (0) is slowdown
	0x8A, 0x07,                               // mov al, [bx]
	0xA2, WORD(mm1_slow_addr+5),              // mov speed_constant, al
	0x1F,                                     // pop ds
	0x59,                                     // pop cx
	0x5B,                                     // pop bx
	0x58,                                     // pop ax
	0x9D,                                     // popf
	0x2E, 0x80, 0x3E, WORD(0x114C), 0x01,     // cmp cs:114Ch, 1 ; sound_enabled
	0xC3,                                     // retn
};
const uint8 mm1_settings0[] = {
	CALL(mm1_settings0_addr,mm1_settings_addr),
	0x90, 0x90, 0x90, // nop, nop, nop
};

// joystick reading replacement that works with a much wider range of computer speeds,
// mostly based on relevant code from Mega Man 3
// location: unused tandy video function 6 (+7 offset, max 314 bytes)
#define mm1_joy_addr    0x23C0
#define mm1_joy_file    0x1C2E
// patch for the original poll routine
#define mm1_joy0_addr   0x174D
#define mm1_joy0_file   0x0FBB
// path for the original calibrate routine
#define mm1_joy1_addr   0x17EA
#define mm1_joy1_file   0x1058
const uint8 mm1_joy[] = {
	// joystick variable storage
	WORD(0),  // joy_x_accum for temporary average of polling
	WORD(10), // joy_x_low threshold for left
	WORD(30), // joy_x_high threshold for right
	WORD(0),  // joy_y_accum
	WORD(10), // joy_y_low threshold for up
	WORD(30), // joy_y_high threshold for down
	// joystick poll based on mega man 3 (+12)
	// assume: ds = cs
	// assume: dx = 0201h (joystick port)
	0xC7, 0x06, WORD(mm1_joy_addr+0), WORD(0), // mov joy_x_accum, 0
	0xC7, 0x06, WORD(mm1_joy_addr+6), WORD(0), // mov joy_y_accum, 0
	0xB9, 0x04, 0x00,                          // mov cx, 4
	                                           //average:
	0x51,                                      // push cx
	0x33, 0xC9,                                // xor cx, cx
	0x33, 0xFF,                                // xor di, di
	0x33, 0xF6,                                // xor si, si
	0x32, 0xC0,                                // xor al, al
	0xFA,                                      // cli
	0xEE,                                      // out dx, al
	                                           //read_loop:
	0xEC,                                      // in al, dx
	0xA8, 0x01,                                // test al, 1
	0x74, 0x01,                                // jz +1
	0x47,                                      // inc di
	0xA8, 0x02,                                // test al, 2
	0x74, 0x01,                                // jz +1
	0x46,                                      // inc si
	0xA8, 0x03,                                // test al, 3
	0xE0, 0xF1,                                // loopne read_loop
	0xFB,                                      // sti
	0x01, 0x3E, WORD(mm1_joy_addr+0),          // add joy_x_accum, di
	0x01, 0x36, WORD(mm1_joy_addr+6),          // add joy_y_accum, si
	0x59,                                      // pop cx
	0xE2, 0xDA,                                // loop average
	0x8B, 0x3E, WORD(mm1_joy_addr+0),          // mov di, joy_x_accum
	0x8B, 0x36, WORD(mm1_joy_addr+6),          // mov si, joy_y_accum
	0xD1, 0xEF,                                // shr di, 1
	0xD1, 0xEF,                                // shr di, 1
	0xD1, 0xEE,                                // shr si, 1
	0xD1, 0xEE,                                // shr si, 1
	0xC3,                                      // retn
	// joystick calibrate based on mega man 3 (+82)
	// sets the low/high thresholds at 25% +/-
	0x9C,                                      // pushf
	0x1E,                                      // push ds
	0x57,                                      // push di
	0x56,                                      // push si
	0x50,                                      // push ax
	0x51,                                      // push cx
	0x52,                                      // push dx
	0x8C, 0xC8,                                // mov ax, cs
	0x8E, 0xD8,                                // mov ds, ax
	CALL(mm1_joy_addr+93,mm1_joy_addr+12),     // call joystick poll
	0x89, 0x3E, WORD(mm1_joy_addr+2),          // mov joy_x low, di
	0x89, 0x3E, WORD(mm1_joy_addr+4),          // mov joy_x high, di
	0xD1, 0xEF,                                // shr di, 1
	0xD1, 0xEF,                                // shr di, 1
	0x29, 0x3E, WORD(mm1_joy_addr+2),          // sub joy_x low, di
	0x01, 0x3E, WORD(mm1_joy_addr+4),          // sub joy_x high, di
	0x89, 0x36, WORD(mm1_joy_addr+8),          // mov joy_y low, si
	0x89, 0x36, WORD(mm1_joy_addr+10),         // mov joy_y high, si
	0xD1, 0xEE,                                // shr si, 1
	0xD1, 0xEE,                                // shr si, 1
	0x29, 0x36, WORD(mm1_joy_addr+8),          // sub joy_y low, si
	0x01, 0x36, WORD(mm1_joy_addr+10),         // sub joy_y high, si          
	0x5A,                                      // pop dx
	0x59,                                      // pop cx
	0x58,                                      // pop ax
	0x5E,                                      // pop si
	0x5F,                                      // pop di
	0x1F,                                      // pop ds
	0x9D,                                      // popf
	0xBB, WORD(0x4040),                        // mov bx, 4040h ; "fake" original calibration centre at 40h, 40h
	0x88, 0x1E, WORD(0x114A),                  // mov joy_centre_x, bl ; replaces patched line at 17EA
	0xC3,                                      // retn
	// replacement for fragment of original joystick routine (+151)
	// jmp from 174D, return to 175E
	0x9C,                                      // pushf
	0x1E,                                      // push ds
	0x57,                                      // push di
	0x52,                                      // push dx
	0x8C, 0xC8,                                // mov ax, cs
	0x8E, 0xD8,                                // mov ds, cs
	CALL(mm1_joy_addr+159,mm1_joy_addr+12),    // call poll
	0xBB, WORD(0x4040),                        // mov bx, 4040h ; fake centre
	0x3B, 0x3E, WORD(mm1_joy_addr+2),          // cmp di, joy_x low
	0x77, 0x02,                                // ja +2
	0xB3, 0x00,                                // mov bl, 0    ; fake up
	0x3B, 0x3E, WORD(mm1_joy_addr+4),          // cmp di, joy_x high
	0x72, 0x02,                                // jb +2
	0xB3, 0x80,                                // mov bl, 0x80 ; fake down
	0x3B, 0x36, WORD(mm1_joy_addr+8),          // cmp di, joy_y low
	0x77, 0x02,                                // ja +2
	0xB7, 0x00,                                // mov bh, 0    ; fake up
	0x3B, 0x36, WORD(mm1_joy_addr+10),         // cmp di, joy_y high
	0x72, 0x02,                                // jb +2
	0xB7, 0x80,                                // mov bh, 0x80 ; fake down
	0x31, 0xC0,                                // xor ax, ax
	0x33, 0xC9,                                // xor cx, cx
	0x33, 0xF6,                                // xor si, si
	0x5A,                                      // pop dx
	0x5F,                                      // pop di
	0x1F,                                      // pop ds
	0x9D,                                      // popf
	JMP(mm1_joy_addr+207,mm1_joy0_addr+0x11),  // jmp 1753h
	// faked original poll: x,y => bl,bh = 00,40,80
	// ax,cx,si = 0 ; post-conditions of the original fragment that was skipped
};
// patch for original poll
const uint8 mm1_joy0[] = { JMP(mm1_joy0_addr,mm1_joy_addr+151), 0x90 };
// patch for original 
const uint8 mm1_joy1[] = { CALL(mm1_joy1_addr,mm1_joy_addr+82), 0x90 };

// joystick fire button filter replacement for joystick poll
// location: unused tandy video function 7 (+7 offset, max 111 bytes)
#define mm1_select_addr   0x24FD
#define mm1_select_file   0x1D6B
const uint8 mm1_select[] = {
	// filter variable storage
	0x00,
	// input filter for select screen (+1)
	CALL(mm1_select_addr+1,0x173C),       // call joystick poll
	0x9C,                                 // pushf
	0x50,                                 // push ax
	0x1E,                                 // push ds
	0x8C, 0xC8,                           // mov ax, cs
	0x8E, 0xD8,                           // mov ds, cs
	0xA0, WORD(0x1204),                   // mov al, input_bitfield
	0x8A, 0xE0,                           // mov ah, al
	0x22, 0x06, WORD(mm1_select_addr+0),  // and al, filter
	0xA2, WORD(0x1204),                   // mov input_bitfield, al
	0x80, 0xE4, 0x80,                     // and ah, 80h ; filter fire
	0xF6, 0xD4,                           // not ah
	0x88, 0x26, WORD(mm1_select_addr+0),  // mov filter, ah
	0x1F,                                 // pop ds
	0x58,                                 // pop ax
	0x9D,                                 // popf
	0xC3,                                 // retn
};
// select screen's call to joystick poll replaced with the filtered poll routine above
#define mm1_select0_addr   0x49BE
#define mm1_select1_addr   0x4A2B
#define mm1_select2_addr   0x52CD
#define mm1_select3_addr   0x53F5
#define mm1_select0_file   0x422C
#define mm1_select1_file   0x4299
#define mm1_select2_file   0x4B3B
#define mm1_select3_file   0x4C63
const uint8 mm1_select0[] = { CALL(mm1_select0_addr, mm1_select_addr+1) };
const uint8 mm1_select1[] = { CALL(mm1_select1_addr, mm1_select_addr+1) };
const uint8 mm1_select2[] = { CALL(mm1_select2_addr, mm1_select_addr+1) };
const uint8 mm1_select3[] = { CALL(mm1_select3_addr, mm1_select_addr+1) };

const patch mm1_patch[] =
{
	// slowdown
	{ mm1_slow_file, LENGTH(mm1_slow), mm1_slow },
	{ mm1_slow0_file, LENGTH(mm1_slow0), mm1_slow0 },
	// settings table
	{ mm1_table_file, LENGTH(mm1_table), mm1_table },
	{ 0x4820, 1, six }, // increasing index of last table entry 5 -> 6
	{ 0x487D, 1, six },
	{ 0x490D, 1, six },
	{ 0x47DB, 2, mm1_table0 },
	{ 0x489C, 2, mm1_table1 },
	{ 0x48BB, 2, mm1_table1 },
	// settings finalization
	{ mm1_settings_file, LENGTH(mm1_settings), mm1_settings },
	{ mm1_settings0_file, LENGTH(mm1_settings0), mm1_settings0 },
	// joystick routine replacement
	{ mm1_joy_file, LENGTH(mm1_joy), mm1_joy },
	{ mm1_joy0_file, LENGTH(mm1_joy0), mm1_joy0 },
	{ mm1_joy1_file, LENGTH(mm1_joy1), mm1_joy1 },
	// selection screen joystick input filter
	{ mm1_select_file, LENGTH(mm1_select), mm1_select },
	{ mm1_select0_file, LENGTH(mm1_select0), mm1_select0 },
	{ mm1_select1_file, LENGTH(mm1_select1), mm1_select1 },
	{ mm1_select2_file, LENGTH(mm1_select2), mm1_select2 },
	{ mm1_select3_file, LENGTH(mm1_select3), mm1_select3 },
	// end
	{0,0,NULL}
};

//
// Mega Man 3 patch
//

// Because I have not identified unused code that I could replace,
// such as the unfinished/disabled Tandy video routines in Mega Man 1,
// I have instead decidde to create two versions of this patch,
// one which always runs as EGA and overwrites the CGA video code,
// and another which always runs as CGA/TANDY and overwrites the EGA video code.

// ega video function 0 (61 bytes until function 1)
// ega video function 1 (316 bytes until function 3)
// ega video function 3 (97 bytes until function 4)
// ega video function 4 (max 286 bytes)
#define mm3c_slow_addr        0x61D5
#define mm3c_table_addr       0x6236
#define mm3c_settings_addr    0x6372
#define mm3c_select_addr      0x63DE

#define mm3c_slow_file        0x1644
#define mm3c_table_file       0x16A5
#define mm3c_settings_file    0x17E1
#define mm3c_select_file      0x184D

// CGA, allows 0,2 = CGA, TANDY
#define mm3c_video_default    0
#define mm3c_video_and        0x02
#define mm3c_video_or         0x00

// cga video function 0 (68 bytes until function 1)
// cga video function 1 (155 bytes until function 3)
// cga video function 3 (125 bytes until function 4)
// cga video function 4 (max 259 bytes)
#define mm3e_slow_addr        0x75C5
#define mm3e_table_addr       0x7609
#define mm3e_settings_addr    0x76A4
#define mm3e_select_addr      0x7721

#define mm3e_slow_file        0x2A34
#define mm3e_table_file       0x2A78
#define mm3e_settings_file    0x2B13
#define mm3e_select_file      0x2B90

// EGA allows 1 = EGA
#define mm3e_video_default    1
#define mm3e_video_and        0x00
#define mm3e_video_or         0x01

void offset_word(uint8* buffer, uint pos, int offset)
{
	uint base = buffer[pos+0] | (buffer[pos+1] << 8);
	uint result = (base + offset) & 0xFFFF;
	buffer[pos+0] = result & 0xFF;
	buffer[pos+1] = (result >> 8) & 0xFF;
}

void replace_word(uint8* buffer, uint pos, uint16 word)
{
	buffer[pos+0] = word & 0xFF;
	buffer[pos+1] = word >> 8;
}

// slowdown routine to delay a specified number of frames
const uint8 mm3_slow[] = {
	0x9C,                                 // pushf
	0x50,                                 // push ax
	0x51,                                 // push cx
	0x52,                                 // push dx
	0xB9, default_speed, 0x00,            // mov cx, speed
	0xE3, 15,                             // jcxz end
	0xBA, 0xDA, 0x03,                     // mov dx, 03DAh
	                                      //wait1: ; wait until not in vertical retrace
	0xEC,                                 // in al, dx
	0xA8, 0x08,                           // test al, 8
	0x74, 0xFB,                           // jz wait1
	                                      //wait2: ; wait until vertical retrace finished
	0xEC,                                 // in al, dx
	0xA8, 0x08,                           // test al, 8
	0x75, 0xFB,                           // jnz wait2
	0xE2, 0xF4,                           // loop wait1
	                                      //end:
	0x5A,                                 // pop dx
	0x59,                                 // pop cx
	0x58,                                 // pop ax
	0x9D,                                 // popf
	0x80, 0x3E, WORD(0x505B), 0x00,       // cmp ds:505Bh, 0 ; joystick enabled
	0xC3,                                 // retn
};
// patch just before a joystick input poll to insert the delay,
// replacing "cmp cs:505Bh, 0" with a call.
#define mm3_slow0_addr   0xD7FD
#define mm3_slow0_file   0x8ADA
const uint8 mm3c_slow0[] = { CALL(mm3_slow0_addr,mm3c_slow_addr), 0x90, 0x90 };
const uint8 mm3e_slow0[] = { CALL(mm3_slow0_addr,mm3e_slow_addr), 0x90, 0x90 };
// only the main game loop is patched, but several other input poll candidates were found,
// searching for uses of cs:505Bh which is the joystick setting flag:
// address (file offset)
// CE7Ch (8159)
// D061h (833E) - followed by calibration rather than poll
// D25Bh (8538)
// D2C7h (85A4) - not followed by a poll
// D38Eh (866B)
// D595h (8872) - setup menu initialization setting
// D5A1h (8873) - setup menu initialization setting
// D6CDh (88AA) - followed by calibration rather than poll
// D6EDh (89CA)
// D722h (89FF)
// D757h (8A34)
// D85Dh (8B3A)

uint8 mm3_table[] = { // not const, see fixup below
	// table of strings for the slowdown setting
	0x28,0x09,1,'0',
	0x2A,0x09,1,'1',
	0x2C,0x09,1,'2',
	0x2E,0x09,1,'3',
	0x30,0x09,1,'4',
	0x32,0x09,1,'5',
	0x34,0x09,1,'6',
	0x36,0x09,1,'7',
	0x38,0x09,1,'8',
	0x3A,0x09,1,'9',
	0x18,0x09,9,'S','l','o','w','d','o','w','n',':',
	default_speed, 9,
	WORD(mm3c_table_addr+(4*10)),
	WORD(mm3c_table_addr+(4*0)),
	WORD(mm3c_table_addr+(4*1)),
	WORD(mm3c_table_addr+(4*2)),
	WORD(mm3c_table_addr+(4*3)),
	WORD(mm3c_table_addr+(4*4)),
	WORD(mm3c_table_addr+(4*5)),
	WORD(mm3c_table_addr+(4*6)),
	WORD(mm3c_table_addr+(4*7)),
	WORD(mm3c_table_addr+(4*8)),
	WORD(mm3c_table_addr+(4*9)),
	mm3c_video_default,2, // default (0 = CGA, 1 = EGA), maximum (2 = TANDY)
	WORD(0x4F8C), // Graphics Card:
	WORD(0x4F9D), // CGA
	WORD(0x4FA3), // EGA
	WORD(0x4FA9), // TANDY
	0,1, // animation
	WORD(0x4FB1), // Animation:
	WORD(0x4FBE), // ON
	WORD(0x4FC3), // OFF
	0,1, // masking
	WORD(0x4FC9), // Masking:
	WORD(0x4FD4), // ON
	WORD(0x4FD9), // OFF
	0,1, // sound
	WORD(0x4FDF), // Sound:
	WORD(0x4FE8), // ON
	WORD(0x4FED), // OFF
	1,1, // joystick (default off)
	WORD(0x4FF3), // Joystick:
	WORD(0x4FFF), // ON
	WORD(0x5004), // OFF
	0,0, // start
	WORD(0x500A), // Start Game
	// settings pointer table, the "default" above also store the current option
	WORD(mm3c_table_addr+52+(2*0)),
	WORD(mm3c_table_addr+52+(2*(12))),
	WORD(mm3c_table_addr+52+(2*(12+5))),
	WORD(mm3c_table_addr+52+(2*(12+5+4))),
	WORD(mm3c_table_addr+52+(2*(12+5+4+4))),
	WORD(mm3c_table_addr+52+(2*(12+5+4+4+4))),
	WORD(mm3c_table_addr+52+(2*(12+5+4+4+4+4))),
};
// the original text table and settings pointer table
// resided at 501Ah and 5048h, and some references to them need to be replaced.
const uint8 mm3c_table0[] = { WORD(mm3c_table_addr+52) };    // 501Ah
const uint8 mm3e_table0[] = { WORD(mm3e_table_addr+52) };    // 501Ah
const uint8 mm3c_table1[] = { WORD(mm3c_table_addr+52+70) }; // 5048h
const uint8 mm3e_table1[] = { WORD(mm3e_table_addr+52+70) }; // 5048h
// converts mm3_table from CGA to EGA version
void mm3e_table_fixup()
{
	int i;
	int offset = mm3e_table_addr - mm3c_table_addr;
	for (i=0; i<11; ++i) offset_word(mm3_table,52+2+(i*2),offset);
	for (i=0; i<7; ++i) offset_word(mm3_table,52+70+(i*2),offset);
	mm3_table[52+(2*12)] = mm3e_video_default;
}

// a routine to copy the new settings table results into the original settings table
// and otherwise finalize the new slowdown setting

// original settings code location to redirect
#define mm3_settings0_addr   0xD55D
#define mm3_settings0_file   0x883A
uint8 mm3_settings[] = { // not const, see fixup below
	0x9C,                                     // pushf
	0x50,                                     // push ax
	0x53,                                     // push bx
	0x51,                                     // push cx
	0x1E,                                     // push ds
	0x8C, 0xC8,                               // mov ax, cs
	0x8E, 0xD8,                               // mov ds, ax
	0xB9, 0x06, 0x00,                         // mov cx, 6
	                                          //repeat: ; count down cx from 6 to 1
	0x89, 0xCB,                               // mov bx, cx
	0xD1, 0xE3,                               // shl bx, 1
	0x8B, 0x9F, WORD(mm3c_table_addr+52+70),  // mov bx, [bx+new_settings_table]
	0x8A, 0x07,                               // mov al, [bx]
	0x89, 0xCB,                               // mov bx, cx
	0xD1, 0xE3,                               // shl bx, 1
	0x8B, 0x9F, WORD(0x5048-2),               // mov bx, [bx+old_settings_table-2]
	0x88, 0x07,                               // mov [bx], al
	0xE2, 0xEA,                               // loop repeat
	0x8B, 0x1E, WORD(mm3c_table_addr+52+70),  // mov bx, new_settings_table ; extra entry (0) is slowdown
	0x8A, 0x07,                               // mov al, [bx]
	0xA2, WORD(mm3c_slow_addr+5),             // mov speed_constant, al
	0x80, 0x26, WORD(0x501A), mm3c_video_and, //and ds:501A, 2
	0x80, 0x0E, WORD(0x501A), mm3c_video_or,  // or ds:501A, 0
	0x1F,                                     // pop ds
	0x59,                                     // pop cx
	0x5B,                                     // pop bx
	0x58,                                     // pop ax
	0x9D,                                     // popf
	0x8A, 0x26, WORD(0x505A),                 // mov ah, ds:505Ah
	0xC3,                                     // retn
};
const uint8 mm3c_settings0[] = { CALL(mm3_settings0_addr, mm3c_settings_addr), 0x90 };
const uint8 mm3e_settings0[] = { CALL(mm3_settings0_addr, mm3e_settings_addr), 0x90 };
// converts mm3_settings from CGA to EGA version
void mm3e_settings_fixup()
{
	int offset =  mm3e_table_addr - mm3c_table_addr;
	offset_word(mm3_settings, 18, offset);
	offset_word(mm3_settings, 36, offset);
	offset_word(mm3_settings, 41, mm3e_slow_addr - mm3c_slow_addr);
	mm3_settings[47] = mm3e_video_and;
	mm3_settings[52] = mm3e_video_or;
}

uint8 mm3_select[] = { // not const, see fixup below
	// filter variable storage
	0x00,
	// input filter for select screen (+1)
	CALL(mm3c_select_addr+1,0x6046),      // call joystick poll
	0x9C,                                 // pushf
	0x50,                                 // push ax
	0xA0, WORD(0x538E),                   // mov al, input_bitfield
	0x8A, 0xE0,                           // mov ah, al
	0x22, 0x06, WORD(mm3c_select_addr+0), // and al, filter
	0xA2, WORD(0x538E),                   // mov input_bitfield, al
	0x80, 0xE4, 0x83,                     // and ah, 83h ; filter fire, left, right
	0xF6, 0xD4,                           // not ah
	0x88, 0x26, WORD(mm3c_select_addr+0), // mov filter, ah
	0x58,                                 // pop ax
	0x9D,                                 // popf
	0xC3,                                 // retn
};
// select screen's call to joystick poll replaced with the filtered poll routine above
// select1 is the stage select screen (additionally filters left/right)
#define mm3_select0_addr   0xCE83
#define mm3_select1_addr   0xD262
#define mm3_select2_addr   0xD395
#define mm3_select3_addr   0xD6F4
#define mm3_select4_addr   0xD729
#define mm3_select5_addr   0xD75E
#define mm3_select6_addr   0xD864
#define mm3_select0_file   0x8160
#define mm3_select1_file   0x853F
#define mm3_select2_file   0x8672
#define mm3_select3_file   0x89D1
#define mm3_select4_file   0x8A06
#define mm3_select5_file   0x8A3B
#define mm3_select6_file   0x8B41
const uint8 mm3c_select0[] = { CALL(mm3_select0_addr, mm3c_select_addr+1) };
const uint8 mm3c_select1[] = { CALL(mm3_select1_addr, mm3c_select_addr+1) };
const uint8 mm3c_select2[] = { CALL(mm3_select2_addr, mm3c_select_addr+1) };
const uint8 mm3c_select3[] = { CALL(mm3_select3_addr, mm3c_select_addr+1) };
const uint8 mm3c_select4[] = { CALL(mm3_select4_addr, mm3c_select_addr+1) };
const uint8 mm3c_select5[] = { CALL(mm3_select5_addr, mm3c_select_addr+1) };
const uint8 mm3c_select6[] = { CALL(mm3_select6_addr, mm3c_select_addr+1) };
const uint8 mm3e_select0[] = { CALL(mm3_select0_addr, mm3e_select_addr+1) };
const uint8 mm3e_select1[] = { CALL(mm3_select1_addr, mm3e_select_addr+1) };
const uint8 mm3e_select2[] = { CALL(mm3_select2_addr, mm3e_select_addr+1) };
const uint8 mm3e_select3[] = { CALL(mm3_select3_addr, mm3e_select_addr+1) };
const uint8 mm3e_select4[] = { CALL(mm3_select4_addr, mm3e_select_addr+1) };
const uint8 mm3e_select5[] = { CALL(mm3_select5_addr, mm3e_select_addr+1) };
const uint8 mm3e_select6[] = { CALL(mm3_select6_addr, mm3e_select_addr+1) };
// converts mm3_select from CGA to EGA version
void mm3e_select_fixup()
{
	int offset = mm3e_select_addr - mm3c_select_addr;
	offset_word(mm3_select, 2, -offset);
	offset_word(mm3_select, 13, offset);
	offset_word(mm3_select, 25, offset);
}

// patch set
const patch mm3c_patch[] =
{
	// slowdown
	{ mm3c_slow_file, LENGTH(mm3_slow), mm3_slow },
	{ mm3_slow0_file, LENGTH(mm3c_slow0), mm3c_slow0 },
	// settings table
	{ mm3c_table_file, LENGTH(mm3_table), mm3_table },
	{ 0x874B, 1, six }, // increasing index of last table entry 5 -> 6
	{ 0x879A, 1, six },
	{ 0x882D, 1, six },
	{ 0x870D, 2, mm3c_table0 },
	{ 0x87B5, 2, mm3c_table1 },
	{ 0x87CF, 2, mm3c_table1 },
	// settings finalization
	{ mm3c_settings_file, LENGTH(mm3_settings), mm3_settings },
	{ mm3_settings0_file, LENGTH(mm3c_settings0), mm3c_settings0 },
	// selection screen joystick input filter
	{ mm3c_select_file, LENGTH(mm3_select), mm3_select },
	{ mm3_select0_file, LENGTH(mm3c_select0), mm3c_select0 },
	{ mm3_select1_file, LENGTH(mm3c_select1), mm3c_select1 },
	{ mm3_select2_file, LENGTH(mm3c_select2), mm3c_select2 },
	{ mm3_select3_file, LENGTH(mm3c_select3), mm3c_select3 },
	{ mm3_select4_file, LENGTH(mm3c_select4), mm3c_select4 },
	{ mm3_select5_file, LENGTH(mm3c_select5), mm3c_select5 },
	{ mm3_select6_file, LENGTH(mm3c_select6), mm3c_select6 },
	// end
	{0,0,NULL}
};
const patch mm3e_patch[] =
{
	// slowdown
	{ mm3e_slow_file, LENGTH(mm3_slow), mm3_slow },
	{ mm3_slow0_file, LENGTH(mm3e_slow0), mm3e_slow0 },
	// settings table
	{ mm3e_table_file, LENGTH(mm3_table), mm3_table },
	{ 0x874B, 1, six }, // increasing index of last table entry 5 -> 6
	{ 0x879A, 1, six },
	{ 0x882D, 1, six },
	{ 0x870D, 2, mm3e_table0 },
	{ 0x87B5, 2, mm3e_table1 },
	{ 0x87CF, 2, mm3e_table1 },
	// settings finalization
	{ mm3e_settings_file, LENGTH(mm3_settings), mm3_settings },
	{ mm3_settings0_file, LENGTH(mm3e_settings0), mm3e_settings0 },
	// selection screen joystick input filter
	{ mm3e_select_file, LENGTH(mm3_select), mm3_select },
	{ mm3_select0_file, LENGTH(mm3e_select0), mm3e_select0 },
	{ mm3_select1_file, LENGTH(mm3e_select1), mm3e_select1 },
	{ mm3_select2_file, LENGTH(mm3e_select2), mm3e_select2 },
	{ mm3_select3_file, LENGTH(mm3e_select3), mm3e_select3 },
	{ mm3_select4_file, LENGTH(mm3e_select4), mm3e_select4 },
	{ mm3_select5_file, LENGTH(mm3e_select5), mm3e_select5 },
	{ mm3_select6_file, LENGTH(mm3e_select6), mm3e_select6 },
	// end
	{0,0,NULL}
};
// convert CGA patch set to VGA
void mm3e_patch_fixup()
{
	// The EGA and CGA patches have a lot of common data.
	// To save redefinition, this routine "patches" the patch to convert.
	mm3e_table_fixup();
	mm3e_settings_fixup();
	mm3e_select_fixup();
}

//
// Common utilities and main program
//

int patch_file(const char* filename_in, const char* filename_out, const patch* const patches)
{
	FILE* fi;
	FILE* fo;
	int i, pos;
	uint32 copied, patched;
	const patch* p;
	char c;

	printf("Patching %s into %s...\n", filename_in, filename_out);
	fi = fopen(filename_in,"rb");
	if (fi == NULL)
	{
		printf("Unable to open: %s\n",filename_in);
		return 2;
	}
	fo = fopen(filename_out,"wb");
	if (fo == NULL)
	{
		fclose(fi);
		printf("Unable to open: %s\n",filename_out);
		return 3;
	}

	copied = 0;
	patched = 0;
	pos = 0;
	while (1)
	{
		i = 0;
		p = patches;
		while (p->length != 0)
		{
			if (pos == p->addr)
			{
				if (debug) printf("%3d: %04X-%04X: %d bytes\n",i,p->addr,p->addr+p->length-1,p->length);
				for (i=0; i<p->length; ++i)
				{
					fputc(p->data[i],fo);
					fgetc(fi); // discard (replaced)
					++pos;
					++patched;
				}
				break;
			}
			++i;
			p = patches + i;
		}
		if (p->length == 0) // not applying patch, copy byte
		{
			c = fgetc(fi);
			if (feof(fi)) break;
			++pos;
			fputc(c,fo);
			++copied;
		}
	}

	printf("%ld bytes copied, %ld bytes patched.\n",copied,patched);
	fclose(fi);
	fclose(fo);
	return 0;
}

uint32 crc32(const char* filename)
{
	FILE* f;
	uint32 crc, mask;
	uint8 c;
	int i;

	f = fopen(filename,"rb");
	if (f == NULL)
	{
		printf("Unable to open: %s\n",filename);
		return 0;
	}

	crc = ~0UL;
	while (1)
	{
		c = fgetc(f);
		if (feof(f)) break;
		crc = crc ^ c;
		for (i=0; i<8; ++i)
		{
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
	}

	fclose(f);
	return ~crc;
}

int main()
{
	uint32 crc;
	int result = 0;

	printf("Opening " FILE_CRC "...\n");
	crc = crc32(FILE_CRC);
	printf("CRC32: %08lX\n", crc);

	if (crc == CRC_MM1 || TEST)
	{
		printf("\n");
		result = patch_file(FILE_MM1, OUT_MM1, mm1_patch);
		if (result) return result;
	}
	if (crc == CRC_MM3 || TEST)
	{
		printf("\n");
		result = patch_file(FILE_MM3, OUT_MM3CGA, mm3c_patch);
		if (result) return result;
		printf("\n");
		mm3e_patch_fixup();
		result = patch_file(FILE_MM3, OUT_MM3EGA, mm3e_patch);
		if (result) return result;
	}
	if (crc != CRC_MM1 && crc != CRC_MM3)
	{
		printf("Unrecognized CRC32. Expected:\n");
		printf("  %08lX - Mega Man\n",CRC_MM1);
		printf("  %08lX - Mega Man 3\n",CRC_MM3);
		result = 1;
	}

	return result;
}
