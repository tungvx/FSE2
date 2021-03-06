// cgen1.c -- code generator
// inside of block
#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "token.h"
#include "ast.h"
#include "loc.h"
#include "sym.h"
#include "code.h"

static AST ast_root;
static int code_root;

static int cgen_blocks(AST);
static int cgen_block(AST, int, int);
static int cgen_stmts(AST, int, int);
static int cgen_stmt(AST, int, int);
static int cgen_asn(AST);
static int cgen_ifstmt(AST, int, int);
static int cgen_whilestmt(AST);
static int cgen_expr(AST, bool);
static int cgen_lval(AST,int);
static int cgen_index(AST,int);
static int cgen_vref(AST,int);
static int cgen_con(AST);
static int cgen_imm(AST);
static int cgen_op2(AST);

static int get_op_code_val(int);
static int get_op_type(int);

static int sizeof_locals(AST);
static int sizeof_local(AST);

static int ast_root;

int main() {
    initline();
    init_AST();
    init_SYM();
    init_LOC();

    ast_root = start_parser();  /* inside the block */

    dump_AST(stdout);

    printf("\n");
    dump_SYM(stdout);

    printf("\n");
    dump_LOC(stdout);

    printf("\n");
    dump_STR(stdout);

    cgen(ast_root);
    dump_code(stdout);

    fix_labels();
    interp();
    return 0;
}

int cgen(AST a) {
    int c=0;
    c = cgen_block(a, 0, 0);
    c = gen_code(RET,0,0);
    return c;
}

static int cgen_blocks(AST a) {
    AST a1, a2;
    int c=0;
    while (!isleaf(a)) {
	get_sons(a, &a1, &a2, 0, 0);
	c = cgen_block(a1, 0, 0);
	a = a2;
    }
    return c;
}

static int cgen_block(AST a, int label_break, int label_continue) {
    AST decls, stmts;
    int c = 0;
    int sz;

    incr_depth();
    get_sons(a, &decls, &stmts, 0, 0);
    sz = sizeof_locals(decls);

    c = gen_code(ENTER, 0, 0);
    c = gen_code(INCT, 0, sz);

    c = cgen_stmts(stmts, label_break, label_continue);

    // c = gen_code(DECT, 0, sz);
    c = gen_code(LEAVE, 0, 0);

    decr_depth();
    return c;
}

static int sizeof_local(AST a) {
    int sz;
    AST a1,a2;
    get_sons(a, &a1, &a2, 0, 0);
    sz = get_sizeoftype(a2);
    return sz; 
}

static int sizeof_locals(AST a) {
    int sz = 0;
    AST a1,a2;
    while (!isleaf(a)) {
	get_sons(a, &a1, &a2, 0, 0);
	switch (nodetype(a1)) {
	    case nVARDECL : sz += sizeof_local(a1); break;
	    case nVARDECLS: sz += sizeof_locals(a1); break;
	    default: sz += 1; break;
	}
	a = a2;
    }
    return sz;
}

static int cgen_stmts(AST a, int label_break, int label_continue) {
    AST a1, a2;
    int c=0;
    while (!isleaf(a)) {
	get_sons(a, &a1, &a2, 0, 0);
	c = cgen_stmt(a1, label_break, label_continue);
	a = a2;
    }
    return c;
}

static int show_cblock_stmt(int cb,int ce) {
    int b=0;

    if (cb < ce) {
	printf(" cblock=(%d,%d)\n",cb+1,ce);
	b = new_cblock();
	set_cblock(b,cb+1,ce);
    } else {
	printf(" cblock=null\n");
    }
    return b;
}

static int cgen_stmt(AST a, int label_break, int label_continue) {
    AST a1=0;
    int c=0;
    int cb, ce;

    cb = get_cur_code();
    get_sons(a, &a1, 0,0,0);

    switch (nodetype(a1)) {
	case nASN:
	    c = cgen_asn(a1);
	    break;
	case nIF:
	    c = cgen_ifstmt(a1, label_break, label_continue);
	    break;
	case nWHILE:
	    c = cgen_whilestmt(a1);
	    break;
	case nBLOCK:
	    c = cgen_block(a1, label_break, label_continue);
	    break;
	case nBREAK:
	    if (label_break > 0){
	    	gen_code(JMP, label_break, 0);
	    } else parse_error("Break must be in while stmt");
	    break;
	case nCONTINUE:
	    if (label_continue > 0){
	    	gen_code(JMP, label_continue, 0);
	    } else parse_error("continue must be in while stmt");
	    break;
	default:
	    break;
    }

    ce = get_cur_code();

    if (cb < ce) {
	// printf("\n");
	// print_AST(a);
	// show_cblock_stmt(cb,ce);
    }

    return c;
}

static int cgen_asn(AST a) {
    int c= 0;
    AST a1,a2;
    get_sons(a, &a1, &a2,0,0);

    c = cgen_expr(a2, false);

    switch(nodetype(a1)){
	case nVREF:
	    c = cgen_vref(a1,true);
	    break;
	case nLVAL:
	    c = cgen_lval(a1, true);
	    break;
    }
    return c;
}

static int cgen_ifstmt(AST a, int label_break, int label_continue) {
    int c= 0;
    AST a1,a2,a3;
    //labels:
    int l1 = new_label();
    int l2 = new_label();

    get_sons(a, &a1,&a2,&a3,0);

    c = cgen_expr(a1, true);
    c = gen_code(JMPF, l1, 0);
    c = cgen_stmt(a2, label_break, label_continue);
    c = gen_code(JMP, l2, 0);    
    c = gen_label(l1);
    c = cgen_stmt(a3, label_break, label_continue);
    c = gen_label(l2);
    return c;
}

static int cgen_whilestmt(AST a) {
    int c= 0;
    AST a1,a2;
    //labels:
    int l1 = new_label();//continue label
    int l2 = new_label();//break label

    get_sons(a, &a1,&a2,0,0);

    c = gen_label(l1);
    c = cgen_expr(a1, true);
    c = gen_code(JMPF, l2, 0);
    c = cgen_stmt(a2, l2, l1);
    c = gen_code(JMP, l1, 0);    
    c = gen_label(l2);
    return c;
}

static int cgen_expr(AST a, bool isConExpr) {
    int c= 0;
    int val = get_ival(a);

    switch (nodetype(a)) {
	case nVREF:
	    c = cgen_vref(a,false);
	    break;
	case nIMM:
	    c = gen_code(LDI, 0, val); 
	    break;
	case nCON:
	    c = gen_code(LDC, 0, val); 
	    break;
	case nOP2:
	    c = cgen_op2(a);
	    break;
	case nLVAL:
	    c = cgen_lval(a, false);
	    break;
	default:
	    break;
    }
    if (isConExpr) gen_code(DECT, 0, 1);
    return c;
}

//generate lval code.
static int cgen_lval(AST a,int lhs){
    int c = 0;
    AST aref, exprs, expr, type;

    get_sons(a, &aref, &exprs, 0, 0);

    int idx = get_ival(aref);
    
    //process type:
    type = gettype_SYM(idx);
    int size[10];
    int i;
    for (i = 0; i < 10; i++) size[i] = 1;
    int length = 0;
    while(nodetype(get_son0(type)) == tARRAY){
	length++;
	for (i = length-1; i > 0; i--) size[i] = size[i-1];
	size[0] *= get_ival(type);
	type = get_son0(type);
    }
    length++;
    for (i = 0; i < length; i++){
	get_sons(exprs, &expr, &exprs, 0, 0);
	if (expr) cgen_expr(expr, false);
	if (size[i] > 1) {
	    gen_code(LDI, 0, size[i]);
	    gen_code(OPR, 0, 4);
	}
	if (i > 0) gen_code(OPR, 0, 2);
    }

    int lev = get_cur_depth() - getdepth_SYM(idx);
    int val = getoffset_SYM(idx);

    gen_code(LDA, lev, val);

    c = gen_code(OPR, 0, 2);

    c = gen_code((lhs)?STX:LDV, lev, val);
    return c;
} 

static int cgen_vref(AST a, int lhs) {
    int c= 0;
    int idx = get_ival(a);

    c = gen_code((lhs)?STV:LDV, get_cur_depth() - getdepth_SYM(idx), getoffset_SYM(idx));  /* dummy */

    return c;
}

static int cgen_op2(AST a) {
    int c= 0;
    AST a1,a2;
    int op;

    get_sons(a, &a1,&a2,0,0);
    op = get_ival(a);

    c = cgen_expr(a1, false);
    c = cgen_expr(a2, false);

    //calculate the kind, type of operator.
    c = gen_code(OPR, get_op_type(a), get_op_code_val(get_ival(a))); /* dummy */
    return c;
}

static int cgen_con(AST a) {
    int c=0;
    int idx = get_ival(a);

    c = gen_code(LDC, 0, idx);
    return c;
}

static int cgen_imm(AST a) {
    int c=0;
    int v = get_ival(a);

    c = gen_code(LDI, 0, v); /* imm */
    return c;
}

static int get_op_code_val(int tokenval){
    int optype = 0;
    switch(tokenval){
	case '+':
	    optype = 2;
	    break;
	case '-':
	    optype = 3;
	    break;
	case '*':
	    optype = 4;
	    break;
	case '/':
	    optype = 5;
	    break;
	case '%':
	    optype = 6;
	    break;
	case EQEQ: 
	    optype = 8;
	    break;
	case NOTEQ: 
	    optype = 9;
	    break;
	case GTEQ: 
	    optype = 10;
	    break;
	case LTEQ: 
	    optype = 11;
	    break;
	case GT: 
	    optype = 12;
	    break;
	case LT: 
	    optype = 13;
	    break;
    }
    return optype;
}

int get_op_type(AST a){
    return typeof_AST(a);
}
