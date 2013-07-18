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
static int cgen_block(AST);
static int cgen_stmts(AST);
static int cgen_stmt(AST);
static int cgen_asn(AST);
static int cgen_ifstmt(AST);
static int cgen_expr(AST);
static int cgen_lval(AST,int);
static int cgen_index(AST,int);
static int cgen_vref(AST,int);
static int cgen_con(AST);
static int cgen_imm(AST);
static int cgen_op2(AST);

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
    c = cgen_block(a);
    c = gen_code(RET,0,0);
    return c;
}

static int cgen_blocks(AST a) {
    AST a1, a2;
    int c=0;
    while (!isleaf(a)) {
         get_sons(a, &a1, &a2, 0, 0);
         c = cgen_block(a1);
         a = a2;
    }
    return c;
}

static int cgen_block(AST a) {
    AST decls, stmts;
    int c = 0;
    int sz;

    incr_depth();
    get_sons(a, &decls, &stmts, 0, 0);
    sz = sizeof_locals(decls);

    c = gen_code(ENTER, 0, 0);
    c = gen_code(INCT, 0, sz);

    c = cgen_stmts(stmts);

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

static int cgen_stmts(AST a) {
    AST a1, a2;
    int c=0;
    while (!isleaf(a)) {
         get_sons(a, &a1, &a2, 0, 0);
         c = cgen_stmt(a1);
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

static int cgen_stmt(AST a) {
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
	    c = cgen_ifstmt(a1);
            break;
        case nBLOCK:
            c = cgen_block(a1);
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

    c = cgen_expr(a2);
    c = cgen_vref(a1,true);
    return c;
}

static int cgen_ifstmt(AST a) {
    int c= 0;
    AST a1,a2,a3;
    get_sons(a, &a1,&a2,&a3,0);

    c = cgen_expr(a1);
    c = gen_code(OPR, 0, 101); /* dummy */
    c = cgen_stmt(a2);
    c = gen_code(OPR, 0, 102); /* dummy */
    c = cgen_stmt(a3);
    c = gen_code(OPR, 0, 103); /* dummy */
    return c;
}

static int cgen_expr(AST a) {
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
        default:
           break;
    }
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

    c = cgen_expr(a1);
    c = cgen_expr(a2);
    c = gen_code(OPR, 0, 99); /* dummy */
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
