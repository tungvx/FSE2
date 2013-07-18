#ifndef _TYPE_H_
#define _TYPE_H_

typedef enum { 
    tINT=512, tCHAR, tFLOAT, tSTRING,
    tPRIM, tARRAY, tPOINTER, tSTRUCT, tFUNC 
} texp_type;

int make_type(char*);
int pointer_type(char*, int);
int array_type(char*, int, int);
int func_type(char *, int);

int typeof_AST(int);
void print_type(int);

void equaltype(int,int);

#endif /* _TYPE_H_ */
