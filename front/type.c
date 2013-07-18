#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "sym.h"
#include "ast.h"

/* 
    type expression : use struct Node with AST
    type : PRIM | POINTER | ARRAY | STRUCT | FUNC
    ival : size (used only ARRAY, STRUCT)
    son[0] : element type (used only ARRAY, POINTER)
*/

inline AST elem(AST a) { return get_son0(a); }

AST make_type(char *name) {
    AST a = make_AST_name(name);
    if (a<=4) return a;
    set_node(a, tPRIM, name, 0);
    return a;
}

AST pointer_type(char *name, AST e) {
    AST a = new_AST();
    set_node(a, tPOINTER, name, 0); 
    set_typeofnode(a, e);
    return a;
}

AST func_type(char *name, AST e) {
    AST a = new_AST();
    set_node(a, tFUNC, name, 0); 
    set_typeofnode(a, e);
    return a;
}

AST array_type(char *name, AST e, int sz) {
    AST a = new_AST();
    set_node(a, tARRAY, name, sz); 
    set_typeofnode(a, e);
    return a;
}

void print_type(AST a) {
    int ty = nodetype(a); 
    int e;

    switch (ty) {
        case tPRIM:
                printf("prim<%s>", get_text(a));
                break;
        case tPOINTER:
		printf("pointer<");
		goto out_elem;
        case tFUNC:
		printf("function<");
		goto out_elem;
        case tARRAY:
		printf("array(%d)<", get_ival(a));
out_elem:
    		get_sons(a, &e, 0, 0, 0);
                print_type(e);
		printf("> ");
		break;
        default:
	        break;
    }
}

AST typeof_AST(AST t) {
    int idx;
    int ty=0;

    switch (nodetype(t)) {
      case nVREF:
        idx = get_ival(t);
        ty  = gettype_SYM(idx);         
        break;

      case nCON:
        ty = get_typeofnode(t);
        break;

      case nOP2:  
        ty = typeof_AST(get_typeofnode(t));
        break;

      case nDEREF:
        ty = typeof_AST(get_typeofnode(t));
        if (nodetype(ty) != tPOINTER) {
            parse_error("expected pointer type");
        }
        ty = get_typeofnode(ty);
        break;

      case nLVAL:
        ty = typeof_AST(get_typeofnode(t));
        if (nodetype(ty) != tARRAY) {
            parse_error("expected array type");
        }
        ty = get_typeofnode(ty);
        break;

    }
    return ty;
}

static bool equal(AST x1, AST x2) {
     switch (nodetype(x1)) {
         case tINT: case tCHAR: case tFLOAT: case tSTRING:
         case tPRIM: 
                if (x1 == x2) return true;
		break;
         case tARRAY:
                if (nodetype(x2) != tARRAY) break;
                if (get_ival(x1) != get_ival(x2)) break;
                return equal( elem(x1), elem(x2));
         case tPOINTER:
                if (nodetype(x2) != tPOINTER) break;
                return equal( elem(x1), elem(x2));
         default:
		break;
     }

     parse_error("type mismatch");
     return false;
}

void equaltype(AST a1, AST a2) {
     int x1 = typeof_AST(a1);
     int x2 = typeof_AST(a2);

     equal(x1,x2);
}

int get_sizeoftype(AST ty) {
    switch (nodetype(ty)) {
      case tFUNC: return 0;
      case tARRAY: 
         return get_ival(ty) * get_sizeoftype(elem(ty));
      default:
         return 1;
    }
}
