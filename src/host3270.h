#include "tn5250-private.h"

#define MAP_DEFAULT "37"
#define BLINK (1<<6)



struct _Tn5250Host {
  Tn5250Stream	  *stream;
  Tn5250CharMap	  *map;
  Tn5250Record	  *record;
  Tn5250Record	  *screenRec;
  Tn5250Buffer	  buffer;
  unsigned short curattr;
  unsigned short lastattr;
  unsigned short cursorPos;
  unsigned       wtd_set        : 1;
  unsigned       clearState	 : 1;
  unsigned       zapState	 : 1;
  unsigned       inputInhibited : 1;
  unsigned       disconnected	 : 1;
  unsigned       inSysInterrupt : 1;
  int	         ra_count;
  unsigned char  ra_char;
  int maxcol;
};

typedef struct _Tn5250Host Tn5250Host;


struct sohPacket_t {
  unsigned char order;
  unsigned char len;
  unsigned char flag;
  unsigned char rsvd;
  unsigned char resq;
  unsigned char errRow;
  unsigned char keySwitch[3];
};

Tn5250Host *tn5250_host_new(Tn5250Stream *This);
void set5250CharMap(Tn5250Host *This, const char *name); 
void sendReadMDT(Tn5250Stream * This, Tn5250Buffer * buff,
		 unsigned char ctrlchar);
void appendBlock2Ebcdic(Tn5250Buffer * buff, unsigned char *str, 
			int len, Tn5250CharMap * map);
void writeToDisplay(Tn5250Host *This);
void setBufferAddr(Tn5250Host *This, int row, int col);
void repeat2Addr(Tn5250Host *This, int row, int col, unsigned char uc);
void setAttribute(Tn5250Host * This, unsigned short attr);
int readMDTfields(Tn5250Host *This, int sendRMF);
int processFlags(Tn5250Host *This, unsigned char flags, unsigned char *buf);
int cancelInvite(Tn5250Stream *This);
char *getErrMsg(short ecode);
void sendWriteErrorCode(Tn5250Host *This, char *msg, unsigned char opcode);
char *processErr(unsigned char *buf);
void clearScreen(Tn5250Host * This);
int processSRQ(Tn5250Stream *This);
void hiliteString(Tn5250Buffer *buff, char *str, Tn5250CharMap *map);
void flushTN5250(Tn5250Host *This);
Tn5250Record *saveScreen(Tn5250Stream *This);
void restoreScreen(Tn5250Stream *This, Tn5250Buffer *buff);
int SendTestScreen(Tn5250Host * This);
void tn5250_host_destroy(Tn5250Host *This);





