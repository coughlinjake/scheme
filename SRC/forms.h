/* forms.h */

/* byte-code interpreter forms */
void bcDefine( C_VOID );
void bcSet( C_VOID );
void bcMacro( C_VOID );

/* special forms */
void opQuote( C_VOID );
void opLambda( C_VOID );
void opDefineForm( C_CONS );

void opDefine( C_VOID );
void opResDefine( C_VOID );

void opSet( C_VOID );
void opResSet( C_VOID );

void opBegin( C_VOID );
void opResBegin( C_CONS );

void opIf( C_VOID );
void opResIf( C_VOID );

void opOr( C_VOID );
void opResOr( C_CONS );

void opAnd( C_VOID );
void opResAnd( C_CONS );

void opMacro( C_VOID );
void opResMacro( C_VOID );
