%{
#include <stdio.h>
%}
cifre [0-9]{2,}
nume [A-Z][a-z]+
litere [a-z]+
%option noyywrap
%%
\<{cifre}\> {printf("[cifre:%s]\n",yytext);}
{litere}"."{litere}"@info.uaic.ro"  {printf ("[email:%s]\n", yytext);}
"- "{nume}(" "{nume})+" -"  {printf ("[Nume:%s]\n", yytext);}
.|\n ;  
%%
int main(int argc, char** argv){
FILE *g;
if(argc>0)
 yyin = fopen(argv[1],"r");

/*if(!(g = fopen("out.txt","w")))
    fprintf(stderr,"eroare creare fisier!");
else
 yyout=g; */

yylex();

}


