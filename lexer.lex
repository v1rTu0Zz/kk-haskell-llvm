%option noyywrap
%option nounput
%option noinput
%{
#include <cstdlib>
#include <iostream>

  using namespace std;
#include <string>
#include "ast.hpp"
#include "parser.tab.hpp"

%}


%%
"if"                     return if_token;
"then"                   return then_token;
"else"                   return else_token;
"where"                  return where_token;
"case"                   return case_token;
"of"                     return of_token;
"let"                	 return let_token;
"in"                	 return in_token;
"otherwise"              return ow_token;
"=="              		 return eq_token;
 \"(\\.|[^\"])*\"   {
  string tmp = string(yytext + 1);\
  tmp = tmp.substr(0, tmp.size() - 1);
  yylval.s = new string(tmp);
  return string_token;
}
[a-zA-Z_][a-zA-Z_0-9]*   {
  yylval.s = new string(yytext);
  return id_token;
}
[<>(),;+*/=-\[\]|]           return *yytext;
[0-9]+(\.[0-9]*)?        {
  yylval.d = atof(yytext);
  return num_token;
}
[ \t\n]                  {   }
.   {
  cerr << "Leksicka greska: Neprepoznat karakter '" << *yytext << "'" << endl;
  exit(EXIT_FAILURE);
}
%%

