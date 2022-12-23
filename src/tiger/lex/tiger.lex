%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE
%%

 /* comment */
"/*" { adjust(); comment_level_ = 1; begin(StartCondition__::COMMENT); }

<COMMENT> {
    "*/" {
        adjust();
        comment_level_--;
        if(!comment_level_)  begin(StartCondition__::INITIAL); 
    }
    "/*" { adjust(); comment_level_++; }
    \n { adjust(); errormsg_->Newline(); }
    <<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in COMMENT"); }
    . { adjust(); }
}

 /* string */
\" { adjust(); string_buf_ = ""; begin(StartCondition__::STR); }
<STR> {
    \\[ \n\t\r] { adjustStr(); begin(StartCondition__::IGNORE); }
    \\n { adjustStr(); string_buf_ += '\n'; }
    \\t { adjustStr(); string_buf_ += '\t'; }
    \\^[A-Z] { adjustStr(); string_buf_ += (char)(matched()[2] - 'A' + 1); }
    \\^\[ { adjustStr(); string_buf_ += (char)0x1B; }
    \\^\\ { adjustStr(); string_buf_ += (char)0x1C; }
    \\^\] { adjustStr(); string_buf_ += (char)0x1D; }
    \\^^ { adjustStr(); string_buf_ += (char)0x1E; }
    \\^_ { adjustStr(); string_buf_ += (char)0x1F; }
    \\[0-9]{3} { adjustStr(); string_buf_ += (char)atoi(matched().substr(1).c_str()); }
    \\\" { adjustStr(); string_buf_ += '\"'; }
    \\\\ { adjustStr(); string_buf_ += '\\'; }
    \" {
        adjustStr();
        setMatched(string_buf_);
        begin(StartCondition__::INITIAL);
        return Parser::STRING;
    }
    ^\\ { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal char sequence in STR"); }
    \n { adjustStr(); errormsg_->Newline(); }
    <<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in STR"); }
    . { adjustStr(); string_buf_ += matched(); }
}

 /* ignore */
<IGNORE> {
    [\n\t\r ] { adjustStr(); }
    \\ { adjustStr(); begin(StartCondition__::STR); }
    \n { adjustStr(); errormsg_->Newline(); }
    <<EOF>> { adjust(); errormsg_->Error(errormsg_->tok_pos_, "EOF in \\f___f\\"); }
    . { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal char in \\f___f\\"); }
}

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /* reserved words */
"array" { adjust(); return Parser::ARRAY; }
"if" { adjust(); return Parser::IF; }
"then" { adjust(); return Parser::THEN; }
"else" { adjust(); return Parser::ELSE; }
"while" { adjust(); return Parser::WHILE; }
"for" { adjust(); return Parser::FOR; }
"to" { adjust(); return Parser::TO; }
"do" { adjust(); return Parser::DO; }
"let" { adjust(); return Parser::LET; }
"in" { adjust(); return Parser::IN; }
"end" { adjust(); return Parser::END; }
"of" { adjust(); return Parser::OF; }
"break" { adjust(); return Parser::BREAK; }
"nil" { adjust(); return Parser::NIL; }
"function" { adjust(); return Parser::FUNCTION; }
"var" { adjust(); return Parser::VAR; }
"type" { adjust(); return Parser::TYPE; }

 /* id */
[A-Za-z][A-Za-z0-9_]* { adjust(); return Parser::ID; }

 /* int */
[0-9]+ { adjust(); return Parser::INT; }

 /* dot */
"." { adjust(); return Parser::DOT; }

 /* COMMA */
"," { adjust(); return Parser::COMMA; }

 /* COLON */
":" { adjust(); return Parser::COLON; }

 /* SEMICOLON */
";" { adjust(); return Parser::SEMICOLON; }

 /* LPAREN */
"(" { adjust(); return Parser::LPAREN; }

 /* RPAREN */
")" { adjust(); return Parser::RPAREN; }

 /* LBRACK */
"[" { adjust(); return Parser::LBRACK; }

 /* RBRACK */
"]" { adjust(); return Parser::RBRACK; }

 /* LBRACE */
"{" { adjust(); return Parser::LBRACE; }

 /* RBRACE */
"}" { adjust(); return Parser::RBRACE; }

 /* PLUS */
"+" { adjust(); return Parser::PLUS; }

 /* MINUS */
"-" { adjust(); return Parser::MINUS; }

 /* TIMES */
"*" { adjust(); return Parser::TIMES; }

 /* DIVIDE */
"/" { adjust(); return Parser::DIVIDE; }

 /* EQ */
"=" { adjust(); return Parser::EQ; }

 /* NEQ */
"<>" { adjust(); return Parser::NEQ; }

 /* LT */
"<" { adjust(); return Parser::LT; }

 /* LE */
"<=" { adjust(); return Parser::LE; }

 /* GT */
">" { adjust(); return Parser::GT; }

 /* GE */
">=" { adjust(); return Parser::GE; }

 /* AND */
"&" { adjust(); return Parser::AND; }

 /* OR */
"|" { adjust(); return Parser::OR; }

 /* ASSIGN */
":=" { adjust(); return Parser::ASSIGN; }

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ { adjust(); }
\n { adjust(); errormsg_->Newline(); }

 /* illegal input */
. { adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token"); }