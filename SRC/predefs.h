/* predef.h - the high-level scheme pre-defined functions and forms.

   NOTES:

	- These are the pre-defined numbers of the Scheme system
	functions and special forms.

	- NUM_FUNCS *MUST* be > the highest predef #.

	- INTERP_CODES *MUST* be == the # of byte-code interpreter ops.
*/

#define NUM_FUNCS	138
#define INTERP_CODES	10

/* byte-code interpreter ops */
#define prNoOp		0
#define prCollectArgs	1
#define prPushConst	2
#define prPushVar	3
#define prReturn	4
#define prNilBranch	5
#define prBranch	6
#define prPopVal	7
#define prMakeClosure	8
#define prPushMark	9
#define prCall		10
#define prPushFunc	11

/* special forms */
#define prDefine	20
#define prDefineForm	21
#define prSet		22
#define prLambda	23
#define prQuote		24
#define prIf		25
#define prOr		26
#define prAnd		27
#define prBegin		29
#define prLet		29
#define prMacro		30
#define prmcExpand	31

/* note missing 32, 33, 34 */

/* interpreter directives */
#define prEnv		35
#define prTorture	36
#define prEvDebug	37
#define prGcDebug	38
#define prExit		39

/* primitive list operations */
#define prCar		40
#define prCdr		41
#define prCons		42
#define prSetCar	43
#define prSetCdr	44

/* predicates */
#define prNull		45
#define prAtom		46
#define prPair		47
#define prSymbol	48
#define prNumber	49
#define prInteger	50
#define prFloat		51
#define prZero		52

/* equality tests */
#define prEq		53
#define prEqv		54
#define prEqual		55

/* math operations */
#define prPlus		56
#define prMinus		57
#define prMult		58
#define prDiv		59
#define prAbs		60
#define prLT		61
#define prGT		62
#define prLTE		63
#define prGTE		64
#define prE		65
#define prNE		66

/* higher-level list functions */
#define prAssoc		67
#define prAssq		68
#define prAssv		69
#define prMember	70
#define prMemq		71
#define prMemv		72
#define prList		73
#define prLength	74
#define prAppend	75
#define prRev		76
#define prTreeCopy	77

/* Misc */
#define prEval		78
#define prApply		79
#define prCallCC	80
#define prProcedure	81
#define prMap		82

/* Boolean functions */
#define prBoolean	83
#define prNot		84

/* character functions */
#define prChar		85
#define prCharE		86
#define prCharL		87
#define prCharG		88
#define prCharLE	89
#define prCharGE	90
#define prCharInt	91
#define prIntChar	92

/* string funtions */
#define prString	93
#define prStrLen	94
#define prStrRef	95
#define prStrE		96
#define prStrL		97
#define prStrG		98
#define prStrLE		99
#define prStrGE		100
#define prSubStr	101
#define prStrLst	102
#define prLstStr	103
#define prSymStr	104
#define prStrSym	105
#define prStrApp	106

/* I/O functions */
#define prRead		107
#define prWrite		108
#define prReadChar	109
#define prWriteChar	110
#define prEofObj	111
#define prDisplay	112
#define prNewLine	113
#define prInPort	114
#define prOutPort	115
#define prCurrIn	116
#define prCurrOut	117
#define prOpenInFile	118
#define prOpenOutFile	119
#define prLoad		120

#define prError		121
#define prGenSym	122

#define prCompile	123
#define prChdir		124
#define prClose		125

#define prVector	126
#define prArgVector	127
#define prMakeVector	128
#define prVectLength	129
#define prVectRef	130
#define prVectSet	131
#define prVectCopy	132
#define prVectFill	133
#define prVectLst	134
#define prLstVect	135

#define prDumpEnv	136
#define prRestEnv	137
