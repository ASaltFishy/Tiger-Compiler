%filenames parser
%scanner tiger/lex/scanner.h
%baseclass-preinclude tiger/absyn/absyn.h

 /*
  * Please don't modify the lines above.
  */

//associate field name(used in %token and %type) with C type 
%union {
  int ival;
  std::string* sval;
  sym::Symbol *sym;
  absyn::Exp *exp;
  absyn::ExpList *explist;
  absyn::Var *var;
  absyn::DecList *declist;
  absyn::Dec *dec;
  absyn::EFieldList *efieldlist;
  absyn::EField *efield;
  absyn::NameAndTyList *tydeclist;
  absyn::NameAndTy *tydec;
  absyn::FieldList *fieldlist;
  absyn::Field *field;
  absyn::FunDecList *fundeclist;
  absyn::FunDec *fundec;
  absyn::Ty *ty;
  }

//specify the token
%token <sym> ID
%token <sval> STRING
%token <ival> INT

%token
  COMMA COLON SEMICOLON LPAREN RPAREN LBRACK RBRACK
  LBRACE RBRACE DOT
  /* PLUS MINUS TIMES DIVIDE EQ NEQ LT LE GT GE
  AND OR ASSIGN */
  ARRAY IF THEN ELSE WHILE FOR TO DO LET IN END OF
  BREAK NIL
  FUNCTION VAR TYPE

 /* token priority */
%nonassoc ASSIGN
%left OR
%left AND
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE

//define semantic value of nonterminal(symbol list)
%type <exp> exp expseq opexp assignexp callexp
%type <explist> actuals nonemptyactuals sequencing sequencing_exps
%type <var> lvalue one oneormore
%type <declist> decs decs_nonempty
%type <dec> decs_nonempty_s vardec
%type <efieldlist> rec rec_nonempty
%type <efield> rec_one
%type <tydeclist> tydec
%type <tydec> tydec_one
%type <fieldlist> tyfields tyfields_nonempty
%type <field> tyfield
%type <ty> ty
%type <fundeclist> fundec
%type <fundec> fundec_one

%start program

%%
program:  exp  {absyn_tree_ = std::make_unique<absyn::AbsynTree>($1);};

 /* TODO: Put your lab3 code here */

 //-----variable----------
lvalue:  
  ID {$$ = new absyn::SimpleVar(scanner_.GetTokPos(), $1);}
  | oneormore 
  ;

 oneormore:
    oneormore DOT ID {$$ = new absyn::FieldVar(scanner_.GetTokPos(), $1, $3); }
  | oneormore LBRACK exp RBRACK {$$ = new absyn:: SubscriptVar(scanner_.GetTokPos(), $1, $3);}
  | one 
  ;

one:
    ID DOT ID {$$ = new absyn::FieldVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1), $3);}
  | ID LBRACK exp RBRACK {$$ = new absyn:: SubscriptVar(scanner_.GetTokPos(), new absyn::SimpleVar(scanner_.GetTokPos(), $1), $3);}
  ;



 //-----expressions--------
exp:
    INT {$$ = new absyn::IntExp(scanner_.GetTokPos(), $1);}
  | STRING {$$ = new absyn::StringExp(scanner_.GetTokPos(), $1);}
  | NIL {$$ = new absyn::NilExp(scanner_.GetTokPos());}
  | lvalue {$$ = new absyn::VarExp(scanner_.GetTokPos(), $1);}
  | LPAREN sequencing_exps RPAREN {$$ = new absyn::SeqExp(scanner_.GetTokPos(), $2);}
  | LPAREN RPAREN {$$ = new absyn::VoidExp(scanner_.GetTokPos());}
  | LPAREN exp RPAREN {$$ = $2;}
  | ID LBRACE rec RBRACE {$$ = new absyn::RecordExp(scanner_.GetTokPos(),$1, $3);} 
  | ID LBRACK exp RBRACK OF exp {$$ = new absyn::ArrayExp(scanner_.GetTokPos(),$1, $3, $6);}
  | IF exp THEN exp {$$ = new absyn::IfExp(scanner_.GetTokPos(),$2, $4, NULL);}
  | IF exp THEN exp ELSE exp {$$ = new absyn::IfExp(scanner_.GetTokPos(),$2, $4, $6);}
  | WHILE exp DO exp {$$ = new absyn::WhileExp(scanner_.GetTokPos(),$2, $4);}
  | FOR ID ASSIGN exp TO exp DO exp {$$ = new absyn::ForExp(scanner_.GetTokPos(),$2, $4, $6, $8);}
  | BREAK {$$ = new absyn::BreakExp(scanner_.GetTokPos());}
  | LET decs IN expseq END {$$ = new absyn::LetExp(scanner_.GetTokPos(),$2, $4);}
  | opexp
  | assignexp
  | callexp
  ;


opexp:
    MINUS exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::MINUS_OP, new absyn::IntExp(scanner_.GetTokPos(), 0),$2);}
  | exp PLUS exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::PLUS_OP, $1, $3);}
  | exp MINUS exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::MINUS_OP, $1, $3);}
  | exp TIMES exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::TIMES_OP, $1, $3);}
  | exp DIVIDE exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::DIVIDE_OP, $1, $3);}
  | exp EQ exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::EQ_OP, $1, $3);}
  | exp NEQ exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::NEQ_OP, $1, $3);}
  | exp LT exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::LT_OP, $1, $3);}
  | exp LE exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::LE_OP, $1, $3);}
  | exp GT exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::GT_OP, $1, $3);}
  | exp GE exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::GE_OP, $1, $3);}
  | exp AND exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::AND_OP, $1, $3);}
  | exp OR exp {$$ = new absyn::OpExp(scanner_.GetTokPos(),absyn::OR_OP, $1, $3);}
  ;

assignexp:
    lvalue ASSIGN exp {$$ = new absyn::AssignExp(scanner_.GetTokPos(), $1, $3);}
  ;

callexp:
    ID LPAREN actuals RPAREN {$$ = new absyn::CallExp(scanner_.GetTokPos(), $1, $3);}
  ;

//return class SepExp, for let-in
expseq:
    exp {$$ = new absyn::SeqExp(scanner_.GetTokPos(), new absyn::ExpList($1));}
  | sequencing_exps {$$ = new absyn::SeqExp(scanner_.GetTokPos(), $1);}
  | {$$ = new absyn::VoidExp(scanner_.GetTokPos());}
  ;

//return class ExpList, for (seq_list)
// when there's only one exp in a seq_list, do not retrun ExPlist and SeqExp
sequencing_exps:
  sequencing
  ;

sequencing:
    exp SEMICOLON exp {$$ = new absyn::ExpList($3);$$->Prepend($1);}
  | exp SEMICOLON sequencing { $3->Prepend($1);$$ = $3;}
  ;

//return class ExpList, for function call
actuals:
    nonemptyactuals
  | {$$ = new absyn::ExpList();}
  ;

nonemptyactuals:
    exp {$$ = new absyn::ExpList($1);}
  | exp COMMA nonemptyactuals   {$3->Prepend($1);$$ = $3;}
  ;

//return class EFieldList, for record
rec:
    rec_nonempty
  | {$$ = new absyn::EFieldList();}
  ;

rec_nonempty:
    rec_one {$$ = new absyn::EFieldList($1);}
  | rec_one COMMA rec_nonempty  {$3->Prepend($1);$$ = $3;}
  ;

rec_one:
    ID EQ exp {$$ = new absyn::EField($1,$3);}
  ;

//---------Declarations------------

//all kinds of Declaration
decs:
    decs_nonempty
  | {$$ = new absyn::DecList();}
  ;

decs_nonempty:
    decs_nonempty_s {$$ = new absyn::DecList($1);}
  | decs_nonempty_s decs_nonempty {$2->Prepend($1);$$ = $2;}
  ;

decs_nonempty_s:
  fundec {$$ = new absyn::FunctionDec(scanner_.GetTokPos(),$1);}
  | tydec {$$ = new absyn::TypeDec(scanner_.GetTokPos(),$1);}
  | vardec
  ;

//variable Declaration
vardec:
    VAR ID ASSIGN exp {
      $$ = new absyn::VarDec(scanner_.GetTokPos(),$2,NULL,$4);
      // std::cout<<$2->Name();
      }
  | VAR ID COLON ID ASSIGN exp {
    $$ = new absyn::VarDec(scanner_.GetTokPos(),$2,$4,$6);
    // std::cout<<$2->Name();
    }
  ;

//function Declaration
fundec:
    fundec_one {$$ = new absyn::FunDecList($1);}
  | fundec_one fundec {$2->Prepend($1);$$ = $2;}
  ;

fundec_one:
    FUNCTION ID LPAREN tyfields RPAREN EQ exp {
      $$ = new absyn::FunDec(scanner_.GetTokPos(),$2,$4,NULL,$7);
      // std::cout<<$2->Name();
    }
  | FUNCTION ID LPAREN tyfields RPAREN COLON ID EQ exp {
    $$ = new absyn::FunDec(scanner_.GetTokPos(),$2,$4,$7,$9);
    // std::cout<<$2->Name();
    }
  ;


//type Declaration
tydec:
    TYPE tydec_one {$$ = new absyn::NameAndTyList($2);}
  | TYPE tydec_one tydec { $3->Prepend($2);$$ = $3;}
  ;

tydec_one:
    ID EQ ty {$$ = new absyn::NameAndTy($1,$3);}
  ;

ty:
    ID {$$ = new absyn::NameTy(scanner_.GetTokPos(),$1);}
  | LBRACE tyfields RBRACE {$$ = new absyn::RecordTy(scanner_.GetTokPos(), $2);}
  | ARRAY OF ID {$$ = new absyn::ArrayTy(scanner_.GetTokPos(), $3);}
  ;

tyfields:
    tyfields_nonempty 
  | {$$ = new absyn::FieldList();}
  ;

tyfields_nonempty:
    tyfield {$$ = new absyn::FieldList($1);}
  | tyfield COMMA tyfields_nonempty {$3->Prepend($1);$$ = $3;}
  ;

tyfield:
    ID COLON ID {$$ = new absyn::Field(scanner_.GetTokPos(),$1,$3);}
  ;

