#ifndef _CODE_H_
#define _CODE_H_

enum opcodetype {
  OPR, LDV, LDI, LDC, LDA, LDX, LDS, 
  STV, STX, STS,
  JMP, JMPF,  CALL,  RET,
  INCT, DECT, ENTER, LEAVE,
};

typedef struct insn {
#ifdef __CYGWIN__  /* little endien */
  int val:16;
  int lev:8;
  int op :8;
#else /* big endien */
  int op :8;
  int lev:8;
  int val:16;
#endif
} insn;

#define insn_code(x) (*(int*)(&(x)))

typedef struct cblock {
  struct cblock *next;
  int begin;
  int end;
} cblock;

typedef struct regs {
  unsigned int B,T;
  unsigned int S,D;
  unsigned int P,R,V;
  unsigned char cc;
} regs; 

int gen_code(int,int,int);
int gen_label(int);
void dump_code(FILE *);
int  new_cblock();
void set_cblock(int,int,int);

#endif
