/* eval.h */

void InitEval( C_INT X C_CHAR C_PTR C_ARRAY );
void evDefGlobal( C_CONS X C_CONS );
CONS evAccNested( C_CONS X C_CONS );
CONS evAccGlobal( C_CONS X C_CONS );
CONS evMkResume( C_INT );
void evAddFunc( C_INT X C_VOID_F_PTR );
void evSaveEnv( C_VOID );
void evCallFunc( C_CONS X C_CONS );
CONS evGatherVal( C_VOID );
CONS evGatherExpr( C_VOID );
void evEval( C_VOID );
void evApply( C_CONS );
