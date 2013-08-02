// code.c -- operation on code and interpreter
//  (append is not supported)
//
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "code.h"

#define MAX_CODE 10000
#define MAX_CODEBLOCK 10000
#define MAX_MEM  10000

static insn code[MAX_CODE];
static int  code_cnt;
static cblock codeblock[MAX_CODEBLOCK];
static int  codeblock_cnt;

static int  mem[MAX_MEM];
static int  mem_cnt;

static int label_seq = 0;

static regs reg;

static char *optypename[] = {
  "OPR", 
  "LDV", "LDI", "LDC", "LDA", "LDX", "LDS", 
  "STV", "STX", "STS",
  "JMP", "JMPF", "CALL", "RET",
  "INCT", "DECT", "ENTER", "LEAVE",
  0
};

static int getoptype(char *name) {
  int i;
  char **p;
  for (i=0,p=optypename; *p; i++,p++)
    if (strcmp(*p,name)==0) return i;   
  return 0;
}

static char buf[256];

void print_insn(insn,FILE*);
void dump_reg();
void init_reg();
void dump_mem();
void interp();
void operator(insn);
void movestr(insn);

int new_code() { return ++code_cnt; }
int get_cur_code() { return code_cnt; }
int new_label() { return ++label_seq; }

void set_code(int c, int op, int lev, int val) {
    insn *cp;
    if (c==0) return;
    cp = &code[c];
    cp->op = op; cp->lev = lev; cp->val = val;
}

int gen_code(int op, int lev, int val) {
    int c = new_code();
    set_code(c, op, lev, val);
    return c;
}

int gen_label(int l) {
    return gen_code(OPR, l, 0);
}

void dump_code(FILE *fp) {
    int i;

    fprintf(fp,"\ncode:cnt=%d\n",code_cnt);
    for (i=1;i<=code_cnt;i++) {
       fprintf(fp,"%4d:",i);
       print_insn(code[i], fp);
       fprintf(fp, "\n");
    }
}

void print_insn(insn x,FILE *fp) {
   sprintf(buf,"[%-8s %4d %4d]", 
        (x.op==255)? "STOP": optypename[x.op],
        x.lev, x.val);
   fprintf(fp,"%s", buf);
}

static int labeltbl[100];
static int labelcnt;

int fix_labels() {
    int i;
    insn *p;
    labelcnt = 0;
    for (i=1;i<=code_cnt;i++) {
         p = &code[i];
         if (p->op == OPR && p->val == 0) {
              labeltbl[p->lev] = i;
              if (p->lev > labelcnt) labelcnt = p->lev;
         }
    }
    for (i=1;i<=code_cnt;i++) {
         p = &code[i];
         if (p->op == JMP || p->op == JMPF || p->op == CALL) {
              if (p->lev > 0 && p->lev <= labelcnt)
		      p->val = labeltbl[p->lev];
         }
    } 
    return labelcnt;
}

int new_cblock() { return ++codeblock_cnt; }

void set_cblock(int c, int b, int e) {
    cblock *cp;
    if (c==0) return;
    cp = &codeblock[c];
    cp->begin = b;
    cp->end = e;
}

void dump_reg() {
   insn x = code[reg.P];
   printf("B=%2d T=%2d S=%2d D=%2d R=%2d V=%2d P=%2d cc=%d:insn=%08x ", 
	reg.B, reg.T, reg.S, reg.D, reg.R, reg.V, reg.P, reg.cc,
	insn_code(x));
   print_insn(x,stdout);
}

void init_reg() {
   reg.B = reg.T = 20;
   reg.S = reg.D = 10;
   reg.R = reg.V = 0;
   reg.P = 1;
}

void dump_mem() {
   int i;
   for (i=0;i<60;i++) {
      if ((i%10)==0) printf("\n%4d: ",i);
      printf("%4d ", mem[i]);
   }
   printf("\n");
}

void init_mem() {
   int i;
   for (i= 0;i<80;i++) mem[i] = -1;
}

inline void push(int x) { mem[reg.T++] = x; }
inline int  pop()       { return mem[--reg.T]; }

int display(int lev) {
    int tmp;
    switch(lev) {
        case 0: return reg.B;
        case 1: return reg.S;
        default: 
          tmp = reg.S;
          while (--lev > 0) tmp = mem[tmp-1]; // BUG?
          return tmp;
    }
}

#define MAX_INSN_CNT	10000

void interp() {
   insn x;
   int insn_cnt = 0;

   init_reg();
   init_mem();

   printf("Start\n");

   while (reg.P > 0 && ++insn_cnt < MAX_INSN_CNT) {
       dump_reg();
       if (reg.P == 0) break;
       x = code[reg.P++];
       switch (x.op) {
         case OPR: operator(x);
           break;
         case LDA: push(display(x.lev)+x.val);
           break;
         case LDV: push(mem[display(x.lev)+x.val]);
           break;
         case STV: mem[display(x.lev)+x.val] = pop();
           break;
         case LDI:
           push(x.val);
           break;
         case LDX: { int t1;
           t1 = pop();
           push(mem[t1]);
           } break;
         case STX: { int t1,t2;
           t1 = pop(); t2 = pop();
           mem[t1] = t2;
           } break;
         case LDS: case STS: 
           movestr(x);
           break;
         case JMPF:
	   if (reg.cc) break;
	   /* through to next case */
         case JMP:
           reg.P = x.val;
           break;
         case INCT:
           reg.T += x.val;
           break;
         case DECT:
           reg.T -= x.val;
           break;
         case ENTER: { int tmp;
           tmp = reg.T; reg.T += 4;
           mem[tmp]   = reg.V;
           mem[tmp+1] = reg.R;
           mem[tmp+2] = reg.D;
           mem[tmp+3] = reg.S;
           reg.D = reg.S = reg.B;
           reg.B = reg.T;
	   }
           break;
         case LEAVE: { int tmp;
           reg.T = reg.B; reg.B = reg.S; 
	   reg.T -= 4; tmp = reg.T;
           reg.V = mem[tmp]  ;
           reg.R = mem[tmp+1];
           reg.D = mem[tmp+2];
           reg.S = mem[tmp+3];
           }
           break;
         case RET:
	   reg.P = reg.R;
           break;
         default: 
           printf("unsupported opcode:%d\n",x.op);
           break; 
       }
       dump_mem();
   }
   printf("\nEnd insn=%d\n", insn_cnt);
}

void operator(insn x) {
    int t1,t2;
    switch (x.val) {
      case -1: /* STOP */
        reg.P = 0;
        break;
      case 0: /* LABEL */
	break;
      case 2: /* ADD */
        t2 = pop(); t1 = pop();
        push(t1+t2);
	reg.cc = ((t1+t2) > 0);
        break;
      case 3: /* SUB */
        t2 = pop(); t1 = pop();
        push(t1-t2);
	reg.cc = ((t1-t2) > 0);
        break;
      case 4: /* MUL */
        t2 = pop(); t1 = pop();
        push(t1*t2);
	reg.cc = ((t1*t2) > 0);
        break;
      case 5: /* DIV */
        t2 = pop(); t1 = pop();
        push(t1/t2);
	reg.cc = ((t1/t2) > 0);
        break;
      case 6: /* MOD */
        t2 = pop(); t1 = pop();
        push(t1%t2);
	reg.cc = ((t1%t2) > 0);
        break;
      case 7: /* NEG */
        t1 = pop();
        push(-t1);
	reg.cc = (t1 > 0);
        break;
      case 8: 
        t2 = pop(); t1 = pop();
        reg.cc = (t1==t2);
        break;
      case 9: 
        t2 = pop(); t1 = pop();
        reg.cc = (t1!=t2);
        break;
      case 10: 
        t2 = pop(); t1 = pop();
        reg.cc = (t1>=t2);
        break;
      case 11: 
        t2 = pop(); t1 = pop();
        reg.cc = (t1<=t2);
        break;
      case 12: 
        t2 = pop(); t1 = pop();
        reg.cc = (t1>t2);
        break;
      case 13: 
        t2 = pop(); t1 = pop();
        reg.cc = (t1<t2);
        break;
      default:
	printf("unsupported operator:%d\n", x.val);
        break;
    }
}

// LDS, STS
void movestr(insn x) {
    int t, i;
    t = pop();
    for (i=0;i<x.val;i++) {
        if (x.op==LDS) push(mem[t-i+x.val-1]);
        else           mem[t+i] = pop();
    } 
}
