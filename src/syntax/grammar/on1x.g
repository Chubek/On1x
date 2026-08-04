/*
 * On1x grammar, transcribed from docs/SPECS.md §17.
 *
 * The current C++ parser remains active until reduction actions construct the
 * full AST and parity tests cover every production.
 */

${declare longest_match program}

program
    : terminator* (statement terminator+)* statement? terminator*
    ;

statement
    : let_statement
    | assign_statement
    | effect_statement
    | if_expression
    | while_statement
    | for_statement
    | match_expression
    | 'return' expression?
    | 'break'
    | 'continue'
    | block
    | expression
    ;

terminator
    : ';'
    | "\n"
    ;

let_statement
    : 'let' identifier '=' expression
    ;

assign_statement
    : lvalue '=' expression
    ;

lvalue
    : identifier
    | postfix '[' expression ']'
    | postfix '.' identifier
    ;

effect_statement
    : '~' expression terminator+ statement
    ;

block
    : '{' terminator* (statement terminator+)* statement? terminator* '}'
    ;

if_expression
    : 'if' expression block ('else' (if_expression | block))?
    ;

while_statement
    : 'while' expression block
    ;

for_statement
    : 'for' identifier 'in' expression block
    ;

match_expression
    : 'match' expression '{' terminator* match_arm* '}'
    ;

match_arm
    : pattern '=>' (expression | block) terminator*
    ;

function_declaration
    : 'fn' identifier '(' parameters? ')' block
    ;

function_literal
    : 'fn' '(' parameters? ')' block
    ;

parameters
    : identifier (',' identifier)* (',' identifier '..')?
    ;

enum_expression
    : 'enum' '{' terminator* enum_member* '}'
    ;

enum_member
    : identifier '=' expression ','? terminator*
    ;

expression
    : or_expression
    ;

or_expression
    : and_expression ('or' and_expression)*
    ;

and_expression
    : equality_expression ('and' equality_expression)*
    ;

equality_expression
    : relational_expression (('==' | '!=') relational_expression)*
    ;

relational_expression
    : range_expression (('<' | '<=' | '>' | '>=') range_expression)*
    ;

range_expression
    : additive_expression ('..' additive_expression)?
    ;

additive_expression
    : multiplicative_expression (('+' | '-') multiplicative_expression)*
    ;

multiplicative_expression
    : unary_expression (('*' | '/' | '%') unary_expression)*
    ;

unary_expression
    : ('not' | '-') unary_expression
    | '?' postfix?
    | '~' unary_expression
    | postfix
    ;

postfix
    : primary postfix_suffix*
    ;

postfix_suffix
    : '(' arguments? ')'
    | '[' expression ']'
    | '.' identifier
    ;

arguments
    : expression (',' expression)*
    ;

primary
    : literal
    | identifier
    | '~'
    | '(' expression? ')'
    | list_literal
    | table_literal
    | tagged_list
    | function_literal
    | function_declaration
    | enum_expression
    | if_expression
    | match_expression
    ;

list_literal
    : '[' (expression (',' expression)* ','?)? ']'
    ;

table_literal
    : '%{' (table_entry (',' table_entry)* ','?)? '}'
    ;

table_entry
    : expression '=>' expression
    ;

tagged_list
    : tag '[' (expression (',' expression)* ','?)? ']'
    ;

literal
    : float_literal
    | integer_literal
    | string_literal
    | 'true'
    | 'false'
    | tag
    ;

tag
    : ':' identifier
    | ':' string_literal
    ;

pattern
    : '_'
    | literal
    | identifier
    | '[' pattern_list? pattern_tail? ']'
    | tag ('[' pattern_list? ']')?
    ;

pattern_list
    : pattern (',' pattern)* ','?
    ;

pattern_tail
    : '..' identifier
    ;

identifier
    : "[A-Za-z_][A-Za-z0-9_]*"
    ;

integer_literal
    : "(0[xX][0-9A-Fa-f]([0-9A-Fa-f_]*[0-9A-Fa-f])?|0[bB][01]([01_]*[01])?|0[oO][0-7]([0-7_]*[0-7])?|[0-9]([0-9_]*[0-9])?)"
    ;

float_literal
    : "([0-9]([0-9_]*[0-9])?\\.[0-9]([0-9_]*[0-9])?([eE][+-]?[0-9]([0-9_]*[0-9])?)?|[0-9]([0-9_]*[0-9])?[eE][+-]?[0-9]([0-9_]*[0-9])?)"
    ;

string_literal
    : "\"([^\"\\\\\n]|\\\\.)*\""
    ;

whitespace
    : ("[ \t\r]+" | line_comment | block_comment)*
    ;

line_comment
    : "//[^\n]*"
    ;

block_comment
    : "/\\*([^*]|\\*+[^*/])*\\*+/"
    ;
