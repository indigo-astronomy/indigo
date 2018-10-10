//
//  mjkzz_def.h
//  indigo
//
//  Created by Peter Polakovic on 10/10/2018.
//  Copyright © 2018 CloudMakers, s. r. o. All rights reserved.
//

#ifndef mjkzz_def_h
#define mjkzz_def_h

#define CMD_GVER 'v' // get version
#define CMD_SCAM 'A' // activate camera for lwValue
#define CMD_SFCS 'a' // activate half pressed
#define CMD_MOVE 'M' // move by positive or negative increment
#define CMD_SREG 'R' // set register value
#define CMD_GREG 'r' // get register value
#define CMD_SPOS 'P' // set position
#define CMD_GPOS 'p' // get position
#define CMD_SSPD 'S' // set speed
#define CMD_GSPD 's' // get speed
#define CMD_SSET 'I' // set settle time
#define CMD_GSET 'i' // get settle time
#define CMD_SHLD 'D' // set hold period
#define CMD_GHLD 'd' // get hold period
#define CMD_SBCK 'K' // set backlash
#define CMD_GBCK 'k' // get backlash
#define CMD_SLAG 'G' // set shutter lag
#define CMD_GLAG 'g' // get shutter lag
#define CMD_SSPS 'B' // set start position
#define CMD_GSPS 'b' // get start position
#define CMD_SCFG 'F' // set configuration flag
#define CMD_GCFG 'f' // get configuration flag
#define CMD_SEPS 'E' // set ending position
#define CMD_GEPS 'e' // get ending position
#define CMD_SCNT 'N' // set number of captures
#define CMD_GCNT 'n' // get number of captures
#define CMD_SSSZ 'Z' // set step size
#define CMD_GSSZ 'z' // get step size
#define CMD_EXEC 'X' // execute commands
#define CMD_STOP 'x' // stop execution of commands

enum enumREG {
	reg_STAT = 100, reg_LPWR, reg_HPWR, reg_MSTEP, reg_MAXP, reg_EXEC, reg_ADDR
};

typedef struct {
	uint8_t ucADD;
	uint8_t ucCMD;
	uint8_t ucIDX;
	uint8_t ucMSG[4];
	uint8_t ucSUM;
} mjkzz_message;

#endif /* mjkzz_def_h */
