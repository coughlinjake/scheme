/* preds.h */

/* prototypes */

/* predicates */
void opBoolean( C_VOID );
void opNull( C_VOID );
void opAtom( C_VOID );
void opPair( C_VOID );
void opVector( C_VOID );
void opSymbol( C_VOID );
void opNumber( C_VOID );
void opInteger( C_VOID );
void opFloat( C_VOID );
void opZero( C_VOID );

/* equality tests */
void opEq( C_VOID );
void opEqv( C_VOID );
void opEqual( C_VOID );

/* math operations */
void opLT( C_VOID );
void opGT( C_VOID );
void opLTE( C_VOID );
void opGTE( C_VOID );
void opE( C_VOID );
void opNE( C_VOID );

/* character ops */
void opChar( C_VOID );
void opCharE( C_VOID );
void opCharL( C_VOID );
void opCharG( C_VOID );
void opCharLE( C_VOID );
void opCharGE( C_VOID );

/* string ops */
void opString( C_VOID );
void opStrE( C_VOID );
void opStrL( C_VOID );
void opStrG( C_VOID );
void opStrLE( C_VOID );
void opStrGE( C_VOID );

/* port ops */
void opEofObj( C_VOID );
void opInPort( C_VOID );
void opOutPort( C_VOID );

/* control operations */
void opProcedure( C_VOID );
