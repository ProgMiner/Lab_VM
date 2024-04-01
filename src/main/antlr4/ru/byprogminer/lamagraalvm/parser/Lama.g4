grammar Lama;


// Tokens

fragment W: [a-zA-Z_0-9];

WS: [ \t\n\r]+ -> skip;
LINE_COMMENT: '--' .*? ('\n' | EOF) -> skip;
BLOCK_COMMENT: '(*' (BLOCK_COMMENT | .)*? '*)' -> skip;

UIDENT: [A-Z] W*;
LIDENT: [a-z] W*;
DECIMAL: '-'? [0-9]+;
STRING: '"' ([^"] | '""')* '"';
CHAR: '\'' ([^'] | '\'\'' | '\\n' | '\\t' ) '\'';

// Grammar

program: scopeExpr EOF;

scopeExpr: definition* expr?;

definition
    : 'var' vars+=varDefItem (',' vars+=varDefItem)* ';'    # varDef
    | 'fun' LIDENT '(' funArgs ')' '{' scopeExpr '}'        # funDef
    ;

varDefItem: LIDENT ('=' basicExpr)?;
funArgs: (args+=LIDENT (',' args+=LIDENT)*)?;

expr: exprs+=basicExpr (';' exprs+=basicExpr)*;
basicExpr: binaryExpr;

binaryExpr
    : binaryOperand                                                             #binaryExpr1
    | lhs=binaryExpr op=('*' | '/' | '%') rhs=binaryExpr                        #binaryExpr2
    | lhs=binaryExpr op=('+' | '-') rhs=binaryExpr                              #binaryExpr2
    | lhs=binaryExpr op=('==' | '!=' | '<' | '<=' | '>' | '>=') rhs=binaryExpr  #binaryExpr2
    | lhs=binaryExpr op='&&' rhs=binaryExpr                                     #binaryExpr2
    | lhs=binaryExpr op='!!' rhs=binaryExpr                                     #binaryExpr2
    | <assoc=right> lhs=binaryExpr op=':' rhs=binaryExpr                        #binaryExpr2
    | <assoc=right> lhs=binaryExpr op=':=' rhs=binaryExpr                       #binaryExpr2
    ;

binaryOperand: '-'? postfixExpr;

postfixExpr
    : primaryExpr                                           #postfixExprPrimary
    | postfixExpr '(' (args+=expr (',' args+=expr)*)? ')'   #postfixExprCall
    | postfixExpr '[' expr ']'                              #postfixExprSubscript
    ;

primaryExpr
    : DECIMAL   #primaryExprDecimal
    | STRING    #primaryExprString
    | CHAR      #primaryExprChar
    | LIDENT    #primaryExprId
    | 'true'    #primaryExprTrue
    | 'false'   #primaryExprFalse
    | 'fun' '(' funArgs ')' '{' scopeExpr '}'           #primaryExprFun
    | 'skip'                                            #primaryExprSkip
    | '(' scopeExpr ')'                                 #primaryExprScope
    | '{' (items+=expr (',' items+=expr)*)? '}'         #primaryExprList
    | '[' (items+=expr (',' items+=expr)*)? ']'         #primaryExprArray
    | UIDENT ('(' args+=expr (',' args+=expr)* ')')?    #primaryExprSexp
    | 'if' conds+=expr 'then' thens+=scopeExpr
     ('elif' conds+=expr 'then' thens+=scopeExpr)*
     ('else' else=expr)
      'fi'                                              #primaryExprIf
    | 'while' cond=expr 'do' body=scopeExpr 'od'        #primaryExprWhile
    | 'do' body=scopeExpr 'while' cond=expr 'od'        #primaryExprDo
    | 'for' init=scopeExpr ',' cond=expr ',' post=expr
      'do' body=scopeExpr 'od'                          #primaryExprFor
    ;
