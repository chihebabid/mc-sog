%{

/* .......... Declarations C ........*/

#include <string>
#include <stdio.h>
#include "RdPMonteur.hpp"

static	RdPMonteur *R;
static  string t;

extern	FILE *yyin;
extern char yytext[];

/*........... Code C additionnel ....*/
extern "C" {
  int yylex();

  int yyerror(const char* s){
    printf("parsing error\n");
    return (1);
  }

  int yywrap(){
    return (1);
  }
}

%}

/*........... Declarations Yacc .....*/
%union {
  int i;
  char *s;
}

%token IN, OUT, MK, CP, PLACE, QUEUE, TRANS, ENDTR, TOKEN, RESET, LOSS, INHIBITOR
%token <i> ENTIER
%token <s> VARIABLE

%type <i> marquage

%start reseau

%%
/*........... Regles de grammaire ...*/

	/*********************/
	/* lecture du reseau */
	/*********************/

reseau	        : noeud | reseau noeud
		;

noeud		: place | queue | transition
		;

 	/***********************/
	/* lecture d'une place */
	/***********************/

place		: PLACE VARIABLE
		{
		  if(!R->addPlace($2))
		  {
		    yyerror("");return(1);
		  }
		}
                | PLACE VARIABLE MK '(' marquage ')'
		{
		  if(!R->addPlace($2,$5))
		  {
		    yyerror("");return(1);
		  }
		}
                | PLACE VARIABLE CP '(' ENTIER ')'
		{
		  if(!R->addPlace($2,0,$5))
		  {
		    yyerror("");return(1);
		  }
		}
                | PLACE VARIABLE MK '(' marquage ')' CP '(' ENTIER ')'
		{
		  if(!R->addPlace($2,$5,$9))
		  {
		    yyerror("");return(1);
		  }
		}
                | PLACE VARIABLE CP '(' ENTIER ')' MK '(' marquage ')'
		{
		  if(!R->addPlace($2,$9,$5))
		  {
		    yyerror("");return(1);
		  }
		}
		;

 	/***********************/
	/* lecture d'une queue */
	/***********************/

queue		: QUEUE VARIABLE
		{
		  if(!R->addQueue($2))
		  {
		    yyerror("");return(1);
		  }
		}
        | QUEUE VARIABLE CP '(' ENTIER ')'
		{
		  if(!R->addQueue($2,$5))
		  {
		    yyerror("");return(1);
		  }
		}
        | QUEUE VARIABLE LOSS
		{
		  if(!R->addLossQueue($2))
		  {
		    yyerror("");return(1);
		  }
		}
        | QUEUE VARIABLE LOSS CP '(' ENTIER ')'
		{
		  if(!R->addLossQueue($2,$6))
		  {
		    yyerror("");return(1);
		  }
		}
        | QUEUE VARIABLE CP '(' ENTIER ')' LOSS
		{
		  if(!R->addLossQueue($2,$5))
		  {
		    yyerror("");return(1);
		  }
		}
		;
	/***********************/
	/* lecture d'une trans */
	/***********************/

nomtransition   : TRANS VARIABLE
                {
		  if(!R->addTrans($2))
		  {
		    yyerror("");return(1);
		  }
		  t=$2;
		}
                ;

transition	: nomtransition entree sortie ENDTR
		;
	/***************************************/
	/* lecture des entrees d'une transition*/
	/***************************************/

entree		: IN '{' listearcin '}'
		|
		;
	/***************************************/
	/* lecture des sorties d'une transition*/
	/***************************************/

sortie		: OUT '{' listearcout '}'
		|
		;

	/***************************************/
	/* lecture d'un arc			*/
	/***************************************/

listearcin	: listearcin VARIABLE ':' marquage ';'
		{
		  if(!R->addPre($2,t,$4))
		  {
		    yyerror("");return(1);
		  }
		}
		| listearcin VARIABLE ':' '[' ENTIER ']' ';'
		{
		  if(!R->addPreQueue($2,t,$5))
		  {
		    yyerror("");return(1);
		  }
		}
		| listearcin VARIABLE ':' RESET ';'
		{
		  if(!R->addReset($2,t))
		  {
		    yyerror("");return(1);
		  }
		}
		| listearcin VARIABLE INHIBITOR ENTIER ';'
		{
		  if(!R->addInhibitor($2,t,$4))
		  {
		    yyerror("");return(1);
		  }
		}
		| listearcin VARIABLE ':' VARIABLE ';'
		{
		  if(!R->addPreAuto($2,t,$4))
		  {
		    yyerror("");return(1);
		  }
		}
		|
		;

listearcout	: listearcout VARIABLE ':' marquage ';'
		{
		  if(!R->addPost($2,t,$4))
		  {
		    yyerror("");return(1);
		  }
		}
		| listearcout VARIABLE ':' '[' ENTIER ']' ';'
		{
		  if(!R->addPostQueue($2,t,$5))
		  {
		    yyerror("");return(1);
		  }
		}
		| listearcout VARIABLE ':' VARIABLE ';'
		{
		  if(!R->addPostAuto($2,t,$4))
		  {
		    yyerror("");return(1);
		  }
		}
		|
		;

marquage	: TOKEN {$$=1;}
		|
		ENTIER TOKEN {$$=$1;}
		;
%%

bool RdPMonteur::create(const char *f){
  int i;
  if ((yyin=fopen(f,"r"))==NULL)
    return false;
  R=this;
  i=(int) yyparse();
  fclose(yyin);
  return (i==0);
}
