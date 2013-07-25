// cgen2.c -- code generatoc
// outside of block
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

static int cgen_classdecls(AST);
static int cgen_classdecl(AST);
static int cgen_vardecls(AST);
static int cgen_vardecl(AST);
static int cgen_funcdecls(AST);
static int cgen_funcdecl(AST);
static int cgen_blocks(AST);
static int cgen_block(AST);
static int sizeof_locals(AST);
static int sizeof_local(AST);
static int cgen_stmts(AST);
static int cgen_stmt(AST);
static int cgen_funccall(AST);

static int ast_root;

int main() {
    initline();
    init_AST();
    init_SYM();
    init_LOC();

    ast_root = start_parser();

    printf("\n\n");
    dump_AST(stdout);

    printf("\n");
    dump_LOC(stdout);

    printf("\n");
    dump_STR(stdout);

    cgen(ast_root);
    dump_code(stdout);

    printf("\n");
    dump_SYM(stdout);

    fix_labels();
    interp();
    return 0;
}

int cgen(AST a) {
    int c=0;
    AST a1=0;
    get_sons(a,&a1, 0, 0, 0);
    c = cgen_classdecls(a1);
    return c;
}

static int cgen_classdecls(AST a) {
    AST a1, a2;
    int c=0;
    while (!isleaf(a)) {
	get_sons(a, &a1, &a2, 0, 0);
	c = cgen_classdecl(a1);
	a = a2;
    }
    return c;
}

static int cgen_classdecl(AST a) {
    AST a1, a2, a3;
    int c=0;

    get_sons(a, &a1, &a2, &a3, 0);
    c = cgen_vardecls(a2);
    c = cgen_funcdecls(a3);
    return c;
}

static int cgen_vardecls(AST a) {
    AST a1, a2;
    int c=0;
    while (!isleaf(a)) {
	get_sons(a, &a1, &a2, 0, 0);
	c = cgen_vardecl(a1);
	a = a2;
    }
    return c;
}

static int cgen_vardecl(AST a) {
    /* TOOD: code for initial value */
}

static int cgen_funcdecls(AST a) {
    AST a1, a2;
    int c=0;
    while (!isleaf(a)) {
	get_sons(a, &a1, &a2, 0, 0);
	c = cgen_funcdecl(a1);
	a = a2;
    }
    return c;
}

static int cgen_funcdecl(AST a) {
    AST a1, a2, a3, a4;
    int c=0;
    int lbl1;

    get_sons(a, &a1, &a2, &a3, &a4);

    lbl1 = new_label();
    // set begin label to val field of ast
    setval_SYM(get_ival(a1), lbl1);
    gen_label(lbl1);

    incr_depth();
    c = cgen_block(a4);
    decr_depth();

    gen_code(RET, 0, 0);
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

    get_sons(a, &decls, &stmts, 0, 0);
    sz = sizeof_locals(decls);

    c = gen_code(ENTER, 0, 0);
    c = gen_code(INCT, 0, sz);
    c = cgen_stmts(stmts);
    // c = gen_code(DECT, 0, sz);
    c = gen_code(LEAVE, 0, 0);

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
	    c = gen_code(OPR, 0, 201); // dummy
	    break;
	case nCALL:
	    c = cgen_funccall(a1); // dummy
	    break;
	case nIF:
	    c = gen_code(OPR, 0, 203); // dummy
	    break;
	case nRET:
	    c = gen_code(OPR, 0, 204); // dummy
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

static int cgen_funccall(AST a){
    AST a1, a2;
    get_sons(a, &a1, &a2, 0, 0);
    int idx = get_ival(a1);
    int begin_label = getval_SYM(idx);
    gen_code(CALL, 0, begin_label);
}
