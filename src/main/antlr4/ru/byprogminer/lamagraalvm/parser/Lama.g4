grammar Lama;


// Tokens

WS: [ \t\n\r]+ -> skip;
INT: '-'?[0-9]+;

// Grammar

expr: INT EOF;
