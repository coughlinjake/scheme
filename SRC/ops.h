/* ops.h */

/* prototypes */
void opNoOp( C_VOID );
void opEnv( C_VOID );
void opTorture( C_VOID );
void opGcDebug( C_VOID );
void opEvDebug( C_VOID );
void opExit( C_VOID );
void opMakeClosure( C_VOID );

/* primitive list operations */
void opCar( C_VOID );
void opCdr( C_VOID );
void opCons( C_VOID );
void opSetCar( C_VOID );
void opSetCdr( C_VOID );

/* math operations */
void opPlus( C_VOID );
void opMinus( C_VOID );
void opMult( C_VOID );
void opDiv( C_VOID );
void opAbs( C_VOID );

/* higher-level list functions */
void opAssoc( C_VOID );
void opAssq( C_VOID );
void opAssv( C_VOID );
void opMember( C_VOID );
void opMemq( C_VOID );
void opMemv( C_VOID );
void opList( C_VOID );
void opLength( C_VOID );
void opAppend( C_VOID );
void opRev( C_VOID );
void opTree_Copy( C_VOID );

/* vector ops */
void opArgVector( C_VOID );
void opMakeVector( C_VOID );
void opVectLength( C_VOID );
void opVectRef( C_VOID );
void opVectSet( C_VOID );
void opVectCopy( C_VOID );
void opVectFill( C_VOID );
void opVectLst( C_VOID );
void opLstVect( C_VOID );

/* Boolean functions */
void opNot( C_VOID );

/* evaluation functions */
void opCallCC( C_VOID );
void opEval( C_VOID );
void opApply( C_VOID );
void opMap( C_VOID );
void opResMap( C_VOID );

/* environment functions */
void opDumpEnv( C_VOID );
void opRestEnv( C_VOID );

/* Character functions */
void opIntChar( C_VOID );
void opCharInt( C_VOID );

void opGenSym( C_VOID );

/* string routines */
void opStrLen( C_VOID );
void opStrRef( C_VOID );
void opSubStr( C_VOID );
void opStrLst( C_VOID );
void opLstStr( C_VOID );
void opSymStr( C_VOID );
void opStrSym( C_VOID );
void opStrApp( C_VOID );

/* I/O Routines */
void opRead( C_VOID );
void opWrite( C_VOID );
void opReadChar( C_VOID );
void opWriteChar( C_VOID );
void opDisplay( C_VOID );
void opNewLine( C_VOID );
void opCurrIn( C_VOID );
void opCurrOut( C_VOID );
void opLoad( C_VOID );
void opOpenInFile( C_VOID );
void opOpenOutFile( C_VOID );
void opClose( C_VOID );

void opError( C_VOID );
void opChdir( C_VOID );
void opCompile( C_VOID );
