/* microcode.h */

#define MAX_REGSTACK		1500
#define MAX_EXPRSTACK		1500
#define MAX_VALSTACK		1500
#define MAX_FUNCSTACK		1500

/* globals */
extern CONS RegStack[];
extern CONS ExprStack[];		/* the expression stack */
extern CONS ValStack[];			/* the value stack */
extern CONS FuncStack[];		/* the function stack */
extern CONS *Top_RegS;			/* top of reg stack */
extern CONS *Top_Expr;			/* top expr on expr stack */
extern CONS *Top_Val;			/* top val on val stack */
extern CONS *Top_Func;			/* top func on func stack */

/* prototypes */
void InitMicro( C_VOID );

/* stack operations */
void mcClearStacks( C_VOID );
void mcPushExpr( C_CONS );
void mcPushVal( C_CONS );
void mcPushFunc( C_CONS );
CONS mcPopVal( C_VOID );
CONS mcPopExpr( C_VOID );
CONS mcPopFunc( C_VOID );
CONS mcGetExprS( C_VOID );
CONS mcGetValS( C_VOID );
CONS mcGetFuncS( C_VOID );
void mcRestExpr( C_CONS );
void mcRestVal( C_CONS );
void mcRestFunc( C_CONS );

/* environment functions */
CONS mcMkEnv( C_VOID );
int mcDumpEnv( C_FILE C_PTR );
int mcRestEnv( C_FILE C_PTR );

/* hashing functions */
int mcHash( C_CHAR C_PTR X C_INT );
int mcRehash( C_CHAR C_PTR X C_INT X C_INT );

/* control functions */
CONS mcProcedure( C_CONS );

/* primitive list operations */
CONS mcIntToCons( C_INT );
CONS mcCharToCons( C_INT );
CONS mcCadr( C_CONS );
CONS mcCaadr( C_CONS );
CONS mcCaddr( C_CONS );
CONS mcCadddr( C_CONS );
CONS mcCaar( C_CONS );
CONS mcCddr( C_CONS );
CONS mcCdar( C_CONS );
CONS mcCdddr( C_CONS );
CONS mcCddddr( C_CONS );
CONS mcCadar( C_CONS );
CONS mcCons( C_CONS X C_CONS );
CONS mcSetCar( C_CONS X C_CONS );
CONS mcSetCdr( C_CONS X C_CONS );
CONS mcCopyCons( C_CONS );

/* equality prototypes */
CONS mcEq( C_CONS X C_CONS );
CONS mcEqv( C_CONS X C_CONS );
CONS mcEqual( C_CONS X C_CONS );

/* high level list operations */
CONS mcRev( C_CONS );
CONS mcAppend( C_CONS X C_CONS );
CONS mcAssoc( C_CONS X C_CONS );
CONS mcAssq( C_CONS X C_CONS );
CONS mcAssv( C_CONS X C_CONS );
CONS mcMember( C_CONS X C_CONS );
CONS mcMemq( C_CONS X C_CONS );
CONS mcMemv( C_CONS X C_CONS );
int mcLength( C_CONS );
CONS mcQAssoc( C_CONS X C_CONS );
CONS mcTree_Copy( C_CONS );

/* vector ops */
CONS mcArgVector( C_VOID );
CONS mcMakeVector( C_INT X C_CONS );
CONS mcLstVector( C_CONS );
CONS mcVectorLst( C_CONS );
CONS mcVectorCopy( C_CONS );
void mcVectorFill( C_CONS X C_CONS );

/* microcode i/o ops */
CONS mcOpen( C_CHAR C_PTR X C_CHAR C_PTR );
void mcClose( C_CONS );
CONS mcRead( C_FILE C_PTR );
void mcEmit( C_CONS X C_FILE C_PTR X C_INT );
int mcLoad( C_CHAR C_PTR );
int mcResLoad( C_CONS );

/* math prototypes (find these in mc_math.c) */
CONS mcPlus( C_VOID );
CONS mcMinus( C_VOID );
CONS mcMult( C_VOID );
CONS mcDiv( C_VOID );
CONS mcAbs( C_VOID );
CONS mcLT( C_VOID );
CONS mcGT( C_VOID );
CONS mcLTE( C_VOID );
CONS mcGTE( C_VOID );
CONS mcE( C_VOID );
CONS mcNE( C_VOID );
CONS mcPos( C_VOID );
CONS mcNeg( C_VOID );
CONS mcOdd( C_VOID );
CONS mcEven( C_VOID );
CONS mcMax( C_VOID );
CONS mcMin( C_VOID );

CONS mcGenSym( C_VOID );

/* string prototypes */
CONS mcSubStr( C_CHAR C_PTR X C_INT X C_INT );
CONS mcSymStr( C_CONS );
CONS mcStrSym( C_CONS );
CONS mcStrApp( C_CHAR C_PTR X C_CHAR C_PTR );
CONS mcStrLst( C_CHAR C_PTR );
CONS mcLstStr( C_CONS );

/* Stack macros */
#define mcRegPush(x)	( R(++Top_RegS) = (x))
#define mcHaveExprs()	( Top_Expr > ExprStack )
#define mcHaveVals()	( Top_Val > ValStack )
#define mcExprStackTop()	( Top_Expr > ExprStack ? R(Top_Expr) : NIL )
#define mcValStackTop()		( Top_Val > ValStack ? R(Top_Val) : NIL )

/* MM macros */
#define ENTER		CONS *Old_Top = Top_RegS
#define REG(x)		CONS *x = (mcRegPush( NIL ), Top_RegS)
#define R(x)		(*(x))
#define MCLEAVE		Top_RegS = Old_Top; return
#define OPVOIDLEAVE	Top_RegS = Old_Top; return
#define OPLEAVE(x)	Top_RegS = Old_Top; mcPushVal( (x) ); return

/* macros to manipulate a CONS node */
#define mcKind(p)	((p) -> cell_type)

#define mcGet_Car(p)	((p) -> data.cons_node.car)	/* the car's cons node */
#define mcGet_Cdr(p)	((p) -> data.cons_node.cdr)	/* the cdr's cons node */

/* macros for handling funcs */
#define mcPrim_Name(n)	((n) -> data.func.name)
#define mcPrim_RA(n)	((n) -> data.func.rargs)
#define mcPrim_AA(n)	((n) -> data.func.aargs)
#define mcPrim_PR(n)	((n) -> data.func.prnum)
#define mcPrim_Op(n)	((n) -> data.func.op)

/* macros for handling bytecode nodes */
#define mcBC_Code(n)	( (n)->data.bcode.code )
#define mcBC_Const(n)	( (n)->data.bcode.const_table )
#define mcBC_CSize(n)	( (n)->data.bcode.lcode )
#define mcBC_CCSize(n)	( (n)->data.bcode.lconst )

/* macros for execution points */
#define mcExe_BC(n)	( (n)->data.exepnt.bcode )
#define mcExe_PC(n)	( (n)->data.exepnt.pc )
#define mcExe_Env(n)	( (n)->data.exepnt.env )

/* macros for closures */
#define mcCl_Env(n)	( (n)->data.closure.env )
#define mcCl_Parms(n)	( (n)->data.closure.parms )
#define mcCl_Body(n)	( (n)->data.closure.body )

/* macros for user-defined special forms */
#define mcForm_Parms(n)	( (n)->data.closure.parms )
#define mcForm_Body(n)	( (n)->data.closure.body )

/* macros for continuations */
#define mcCont_Env(n)	( (n)->data.cont.env )
#define mcCont_Val(n)	( (n)->data.cont.vals )
#define mcCont_Exp(n)	( (n)->data.cont.exps )
#define mcCont_Fnc(n)	( (n)->data.cont.fncs )

#define mcVect_Size(n)	( (n)->data.vector.size )
#define mcVect_Ref(n,r)	( ((n)->data.vector.elems+(r)) )

/* macros for environments */
#define mcGet_Global(e)	( (e)->data.env.global )
#define mcGet_Nested(e)	( (e)->data.env.nested )

/* macros for other Scheme data-types */
#define mcCpy_Sym(p, s)	( ((p) -> data.int_data) = ssAddSymbol(s) )  /* put symbol in node */
#define mcCpy_Str(p, s) ( ((p) -> data.string) = ssAddString(s) )  /* put string in node */
#define mcCpy_Char(p, c)  ( ((p) -> data.character) = c )	  /* put character in node */
#define mcCpy_Int(p, i)   ((p) -> data.int_data = i)	/* put INT in atom node */
#define mcCpy_Float(p, f) ((p) -> data.float_data = f)	/* put FLOAT in atom node */
#define mcCpy_Port(c, p)  ((c) -> data.port.fp = p)	/* put FILE handle in atom node */
#define mcCpy_PortType(c, t)  ((c) -> data.port.type = t )	  /* put port type in node */

#define mcGet_Sym(p)	( SymTable[(p)->data.int_data] )	/* returns the symbol */
#define mcGet_Str(p)	((p) -> data.string)		/* returns the string */
#define mcGet_Char(p)	((p) -> data.character)		/* returns the char */
#define mcGet_Int(p)	((p) -> data.int_data)		/* returns the integer */
#define mcGet_Float(p)	((p) -> data.float_data)	/* returns float */
#define mcGet_Port(p)	((p) -> data.port.fp)		/* returns the FILE * */
#define mcGet_PortType(p)  ((p) -> data.port.type)	/* returns the port's type */
#define mcGet_Vector(p)	((p)-> data.vector.elems )	/* returns the base of the array */

/* predicate macros used by the system */
#define mcCode(n)	( mcKind((n)) == BCODES )
#define mcClosure(n)	( mcKind((n)) == CLOSURE )
#define mcUserForm(n)	( mcKind((n)) == FORM )
#define mcExe(n)	( mcKind((n)) == EXEPOINT )
#define mcFunc(n)	( mcKind((n)) == CFUNC )
#define mcForm(n)	( mcKind((n)) == CFORM )
#define mcFunc(n)	( mcKind((n)) == CFUNC )
#define mcCont(n)	( mcKind((n)) == CONT )
#define mcVector(n)	( mcKind((n)) == VECTOR )
#define mcEnvironment(n)	( mcKind((n)) == ENVMNT )
#define mcResume(n)	( mcKind((n)) == RESUME )

/* predicate macros used by users */
#define mcNull(l)	( (l) == NIL )
#define mcPair(p)	( mcKind((p)) == PAIR )
#define mcAtom(p)	( !mcPair(p) )
#define mcSymbol(p)	( mcKind((p)) == SYMBOL )
#define mcString(p)	( mcKind((p)) == STRING )
#define mcChar(p)	( mcKind((p)) == CHAR )
#define mcNumber(p)	( mcKind((p)) == INT || mcKind((p)) == FLOAT )
#define mcInteger(p)	( mcKind((p)) == INT )
#define mcFloat(p)	( mcKind((p)) == FLOAT )
#define mcPort(p)	( mcKind((p)) == PORT )
#define mcInPort(p)	( mcKind((p)) == PORT && mcGet_PortType((p)) == INPUT )
#define mcOutPort(p)	( mcKind((p)) == PORT && mcGet_PortType((p)) == OUTPUT )
#define mcClosedPort(p) ( mcKind((p)) == PORT && mcGet_PortType((p)) == CLOSED )

#define mcZero(p)	( (mcInteger((p)) && mcGet_Int((p)) == 0) ||\
			  (mcFloat((p)) && mcGet_Float((p)) == 0.0) )

#define mcCharE(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == CHAR && mcGet_Char(p1) == mcGet_Char(p2) )
#define mcCharL(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == CHAR && mcGet_Char(p1) < mcGet_Char(p2) )
#define mcCharG(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == CHAR && mcGet_Char(p1) > mcGet_Char(p2) )
#define mcCharLE(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == CHAR && mcGet_Char(p1) <= mcGet_Char(p2) )
#define mcCharGE(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == CHAR && mcGet_Char(p1) >= mcGet_Char(p2) )

#define mcStrE(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == STRING && strcmp( mcGet_Str(p1), mcGet_Str(p2) ) == 0 )
#define mcStrL(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == STRING && strcmp( mcGet_Str(p1), mcGet_Str(p2) ) < 0 )
#define mcStrG(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == STRING && strcmp( mcGet_Str(p1), mcGet_Str(p2) ) > 0 )
#define mcStrLE(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == STRING && strcmp( mcGet_Str(p1), mcGet_Str(p2) ) <= 0 )
#define mcStrGE(p1, p2) ( mcKind(p1) == mcKind(p2) && mcKind(p1) == STRING && strcmp( mcGet_Str(p1), mcGet_Str(p2) ) >= 0 )

/* Higher level list macros */
#define mcCar(p)	( mcPair((p)) ? mcGet_Car((p)) : NIL )
#define mcCdr(p)	( mcPair((p)) ? mcGet_Cdr((p)) : NIL )

/* Emitting functions */
#define mcWrite(l,p)	( mcEmit( (l), (p), FALSE ) )
#define mcDisplay(l,p)	( mcEmit( (l), (p), TRUE ) )
