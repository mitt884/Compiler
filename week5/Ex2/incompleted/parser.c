/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "error.h"
#include "debug.h"

Token *currentToken;
Token *lookAhead;

extern Type* intType;
extern Type* charType;
extern SymTab* symtab;


void scan(void) {
  Token* tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType) {
  if (lookAhead->tokenType == tokenType)
    scan();
  else
    missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}


void compileProgram(void) {
  Object* program;

  eat(KW_PROGRAM);
  program = createProgramObject(lookAhead->string);
  eat(TK_IDENT);
  eat(SB_SEMICOLON);

  declareObject(program);
  enterBlock(program->progAttrs->scope);

  compileBlock();

  exitBlock();
  eat(SB_PERIOD);
}

void compileBlock(void) {
  if (lookAhead->tokenType == KW_CONST) {
    eat(KW_CONST);
    do {
      Object* c;
      char* name = lookAhead->string;
      eat(TK_IDENT);
      eat(SB_EQ);

      c = createConstantObject(name);
      c->constAttrs->value = compileConstant();
      declareObject(c);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
  compileBlock2();
}

void compileBlock2(void) {
  if (lookAhead->tokenType == KW_TYPE) {
    eat(KW_TYPE);
    do {
      Object* t;
      char* name = lookAhead->string;
      eat(TK_IDENT);
      eat(SB_EQ);

      t = createTypeObject(name);
      t->typeAttrs->actualType = compileType();
      declareObject(t);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
  compileBlock3();
}

void compileBlock3(void) {
  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);
    do {
      Object* v;
      char* name = lookAhead->string;
      eat(TK_IDENT);
      eat(SB_COLON);

      v = createVariableObject(name);
      v->varAttrs->type = compileType();
      declareObject(v);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);
  }
  compileBlock4();
}

void compileBlock4(void) {
  compileSubDecls();
  compileBlock5();
}

void compileBlock5(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}


void compileSubDecls(void) {
  while (lookAhead->tokenType == KW_FUNCTION ||
         lookAhead->tokenType == KW_PROCEDURE) {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else
      compileProcDecl();
  }
}

void compileFuncDecl(void) {
  Object* f;
  char* name;

  eat(KW_FUNCTION);
  name = lookAhead->string;
  eat(TK_IDENT);

  f = createFunctionObject(name);
  declareObject(f);
  enterBlock(f->funcAttrs->scope);

  compileParams();
  eat(SB_COLON);
  f->funcAttrs->returnType = compileType();

  eat(SB_SEMICOLON);
  compileBlock();
  exitBlock();
  eat(SB_SEMICOLON);
}

void compileProcDecl(void) {
  Object* p;
  char* name;

  eat(KW_PROCEDURE);
  name = lookAhead->string;
  eat(TK_IDENT);

  p = createProcedureObject(name);
  declareObject(p);
  enterBlock(p->procAttrs->scope);

  compileParams();
  eat(SB_SEMICOLON);
  compileBlock();

  exitBlock();
  eat(SB_SEMICOLON);
}


ConstantValue* compileUnsignedConstant(void) {
  ConstantValue* v = (ConstantValue*)malloc(sizeof(ConstantValue));

  switch (lookAhead->tokenType) {
  case TK_NUMBER:
    v->type = TP_INT;
    v->intValue = atoi(lookAhead->string);
    eat(TK_NUMBER);
    break;

  case TK_CHAR:
    v->type = TP_CHAR;
    v->charValue = lookAhead->string[0];
    eat(TK_CHAR);
    break;

  case TK_IDENT: {
    Object* obj = lookupObject(lookAhead->string);
    if (obj->kind != OBJ_CONSTANT)
      error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    *v = *(obj->constAttrs->value);
    eat(TK_IDENT);
    break;
  }

  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
  }
  return v;
}

ConstantValue* compileConstant(void) {
  if (lookAhead->tokenType == SB_PLUS) {
    eat(SB_PLUS);
    return compileUnsignedConstant();
  }
  if (lookAhead->tokenType == SB_MINUS) {
    eat(SB_MINUS);
    ConstantValue* v = compileUnsignedConstant();
    v->intValue = -v->intValue;
    return v;
  }
  return compileUnsignedConstant();
}


Type* compileType(void) {
  Type* type;

  switch (lookAhead->tokenType) {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = intType;
    break;

  case KW_CHAR:
    eat(KW_CHAR);
    type = charType;
    break;

  case KW_ARRAY: {
    eat(KW_ARRAY);
    eat(SB_LSEL);
    int size = atoi(lookAhead->string);
    eat(TK_NUMBER);
    eat(SB_RSEL);
    eat(KW_OF);
    type = makeArrayType(size, compileType());
    break;
  }

  case TK_IDENT: {
    Object* obj = lookupObject(lookAhead->string);
    if (obj->kind != OBJ_TYPE)
      error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    type = obj->typeAttrs->actualType;
    eat(TK_IDENT);
    break;
  }

  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
  }
  return type;
}

Type* compileBasicType(void) {
  if (lookAhead->tokenType == KW_INTEGER) {
    eat(KW_INTEGER);
    return intType;
  }
  if (lookAhead->tokenType == KW_CHAR) {
    eat(KW_CHAR);
    return charType;
  }
  error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
  return NULL;
}


void compileParams(void) {
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON) {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

void compileParam(void) {
  enum ParamKind kind = PARAM_VALUE;
  Object* param;
  char* name;

  if (lookAhead->tokenType == KW_VAR) {
    eat(KW_VAR);
    kind = PARAM_REFERENCE;
  }

  name = lookAhead->string;
  eat(TK_IDENT);
  eat(SB_COLON);

  param = createParameterObject(name, kind, symtab->currentScope->owner);
  param->paramAttrs->type = compileBasicType();
  declareObject(param);
}


void compileStatements(void) {
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON) {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

void compileStatement(void) {
  switch (lookAhead->tokenType) {
  case TK_IDENT: compileAssignSt(); break;
  case KW_CALL: compileCallSt(); break;
  case KW_BEGIN: compileGroupSt(); break;
  case KW_IF: compileIfSt(); break;
  case KW_WHILE: compileWhileSt(); break;
  case KW_FOR: compileForSt(); break;
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    break;
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileAssignSt(void) {
  /* LHS */
  do {
    Object* lhs = lookupObject(lookAhead->string);

    if (lhs->kind != OBJ_VARIABLE && lhs->kind != OBJ_PARAMETER)
      error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);

    eat(TK_IDENT);
    compileIndexes();

    if (lookAhead->tokenType == SB_COMMA)
      eat(SB_COMMA);
    else
      break;
  } while (1);

  eat(SB_ASSIGN);

  /* RHS */
  compileExpression();
  while (lookAhead->tokenType == SB_COMMA) {
    eat(SB_COMMA);
    compileExpression();
  }
}



void compileCallSt(void) {
  eat(KW_CALL);
  eat(TK_IDENT);
  compileArguments();
}

void compileGroupSt(void) {
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void) {
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE)
    compileElseSt();
}

void compileElseSt(void) {
  eat(KW_ELSE);
  compileStatement();
}

void compileWhileSt(void) {
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
}

void compileForSt(void) {
  eat(KW_FOR);
  eat(TK_IDENT);
  eat(SB_ASSIGN);
  compileExpression();
  eat(KW_TO);
  compileExpression();
  eat(KW_DO);
  compileStatement();
}

void compileArgument(void) { compileExpression(); }

void compileArguments(void) {
  if (lookAhead->tokenType == SB_LPAR) {
    eat(SB_LPAR);
    compileArgument();
    while (lookAhead->tokenType == SB_COMMA) {
      eat(SB_COMMA);
      compileArgument();
    }
    eat(SB_RPAR);
  }
}

void compileCondition(void) {
  compileExpression();
  switch (lookAhead->tokenType) {
  case SB_EQ: case SB_NEQ: case SB_LE:
  case SB_LT: case SB_GE: case SB_GT:
    scan(); break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }
  compileExpression();
}

void compileExpression(void) {
  if (lookAhead->tokenType == SB_PLUS || lookAhead->tokenType == SB_MINUS)
    scan();
  compileTerm();
  while (lookAhead->tokenType == SB_PLUS || lookAhead->tokenType == SB_MINUS) {
    scan();
    compileTerm();
  }
}

void compileTerm(void) {
  compileFactor();
  while (lookAhead->tokenType == SB_TIMES || lookAhead->tokenType == SB_SLASH) {
    scan();
    compileFactor();
  }
}

void compileFactor(void) {
  switch (lookAhead->tokenType) {
  case TK_NUMBER: eat(TK_NUMBER); break;
  case TK_CHAR: eat(TK_CHAR); break;
  case TK_IDENT:
    eat(TK_IDENT);
    if (lookAhead->tokenType == SB_LPAR) compileArguments();
    else if (lookAhead->tokenType == SB_LSEL) compileIndexes();
    break;
  default:
    error(ERR_INVALID_FACTOR, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileIndexes(void) {
  while (lookAhead->tokenType == SB_LSEL) {
    eat(SB_LSEL);
    compileExpression();
    eat(SB_RSEL);
  }
}

int compile(char *fileName) {
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();
  compileProgram();
  printObject(symtab->program, 0);
  cleanSymTab();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;
}
