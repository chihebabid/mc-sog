%{
/************* INCLUDES AND EXTERNALS *************************************/
extern "C" int yylex();

#include <stdio.h>
#include <string.h>
#include "RdPMonteur.tab.hpp"

%}

%%
"("			                  { return('('); }
")"			                  { return(')'); }
"{"			                  { return('{'); }
"}"			                  { return('}'); }
":"			                  { return(':'); }
";"			                  { return(';'); }
"<."[ ]*".>"		          { return(TOKEN);}
"<."			                { return('['); }
".>"			                { return(']'); }
"<"			                  { return(INHIBITOR); }
"#trans"		              { return(TRANS);}
"#place"		              { return(PLACE);}
"#queue"		              { return(QUEUE);}
"#endtr"		              { return(ENDTR);}
"reset"		                { return(RESET);}
"loss"		                { return(LOSS);}
mk/[ ]*"("		            { return(MK);}
cp/[ ]*"("		            { return(CP);}
^in			                  { return(IN);}
^out			                { return(OUT);}
[0-9]+		                { sscanf(yytext,"%d",&yylval.i); return(ENTIER);}
[a-zA-Z_][a-zA-Z0-9_]*	  { yylval.s = strdup(yytext); return(VARIABLE);}
[ \t\n]			              {}
%%
