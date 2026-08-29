{
#include "dparse.h"
}

program: top_level*;
top_level: definition ';'? | expression ';'?;

definition: function_definition | type_definition;

function_definition
  : 'fn' identifier type_params? '(' parameter_list? ')' return_type? ('=' expression | block_expression)
  ;

type_definition
  : 'type' identifier type_params? '=' type_expression
  ;

expression
  : let_expression
  | if_expression
  | fn_expression
  | block_expression
  | binary_expression
  | primary_expression
  ;

let_expression
  : 'let' pattern '=' expression 'in' expression
  | 'let' pattern '=' expression
  ;

if_expression
  : 'if' expression block_expression 'else' (if_expression | block_expression)
  | 'if' expression block_expression
  ;

fn_expression
  : 'fn' identifier? '(' parameter_list? ')' return_type? ('=' expression | block_expression)
  ;

block_expression: '{' statement* expression? '}';
statement: definition | expression ';' | let_expression ';';

binary_expression: unary_expression (binary_operator unary_expression)*;
unary_expression: unary_operator unary_expression | primary_expression;

primary_expression
  : literal
  | identifier
  | '(' expression ')'
  | list_expression
  | tuple_expression
  | record_expression
  ;

list_expression: '[' expression_list? ']';
tuple_expression: '(' expression ',' expression (',' expression)* ')';
record_expression: '{' field_init (',' field_init)* '}';

field_init: identifier ':' expression | identifier;

parameter_list: parameter (',' parameter)*;
parameter: identifier type_annotation?;
type_params: '<' identifier (',' identifier)* '>';
type_annotation: ':' type_expression;
return_type: '->' type_expression;

type_expression
  : type_primary
  | '(' type_expression (',' type_expression)+ ')'
  | '[' type_expression ']'
  | '&' 'mut'? type_expression
  | type_expression '[' type_expression ']'
  ;

type_primary: identifier | primitive_type | '(' type_expression ')';

primitive_type: 'i32' | 'i64' | 'f32' | 'f64' | 'bool' | 'char' | 'str' | 'unit';

pattern
  : identifier
  | '_'
  | '(' pattern (',' pattern)+ ')'
  | '[' pattern_list? ']'
  ;

pattern_list: pattern (',' pattern)*;

literal
  : number
  | string
  | 'true'
  | 'false'
  | '()'
  ;

binary_operator
  : '==' | '!=' | '<=' | '>=' | '<' | '>'
  | '&&' | '||'
  | '+' | '-' | '*' | '/' | '%' | '|' | '^' | '<<' | '>>' | '|>'
  | '='
  ;

unary_operator: '-' | '!' | '&';

expression_list: expression (',' expression)*;

identifier: "[A-Za-z_][A-Za-z0-9_]*" $term 0;
number: "-?[0-9]+(\\.[0-9]+)?";
string: "\"([^\"\\\\]|\\\\.)*\"";

whitespace: "([ \t\r\n]|--[^\n\r]*)*";
