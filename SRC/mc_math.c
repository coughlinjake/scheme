/* mc_math.c - the C, low-level MATH functions.

   Version 1
	- Lowest level C routines which manipulate the structures directly.

	- See notes in microcode.c.

	- These routines are only used by Scheme operations.  Therefore, to
	optimize, they take their args from the value stack.

   BUGS:

	- Overflow and Underflow in arithmetic for integers is NOT
	handled.  Solutions: convert to FLOAT or use long integers.

	- R(num1) = new_cons(); R(num2) = new_cons() results in two cons
	nodes whose mcKind() == FREE.  Can't figure out why.  Rearranged to
	be R(num1) = new_cons(); mcCopyCons( R(num1), ...);
*/

#include "machine.h"

#include <math.h>

#include "glo.h"
#include "micro.h"
#include "memory.h"
#include "error.h"

/* local prototypes */
void mcCoerce( C_CONS X C_CONS );

/* mcCoerce(num1, num2) - Coerces either num1 or num2 so that they're the
	same type.  Must convert INT's to FLOAT's.
*/
void mcCoerce(num1, num2)
CONS num1, num2;
{
   REAL_NUM tfloat;

   /* coerce the numbers to floating point if necessary */
   if ( mcKind(num1) != mcKind(num2) && mcKind(num1) == FLOAT ) {

	/* num1 = FLOAT, num2 = INT, coerce num2 into float */
	tfloat = (REAL_NUM) mcGet_Int(num2);
	mcCpy_Float(num2, tfloat);
	mcKind(num2) = FLOAT;
  }
  else if ( mcKind(num1) != mcKind(num2) && mcKind(num2) == FLOAT ) {

	/* num1 = INT, num2 = FLOAT, coerce num1 into float */
	tfloat = (REAL_NUM) mcGet_Int(num1);
	mcCpy_Float(num1, tfloat);
	mcKind(num1) = FLOAT;
  }
}

/* mcPlus() - Adds the args together and returns their sum.  If only
	one arg is given, it's added to 0.  If no args are given, 0 is
	returned.  The args are taken off the value stack.
*/
CONS mcPlus()
{
   CONS result, farg;
   int tint;
   REAL_NUM tfloat;

   /* need a node for the result; assume it's an integer, coerce into
    * float only if need be
    */
   result = NewCons( INT, 0, 0 );

   /* (+) => 0 */
   mcCpy_Int(result, 0);

   /* loop thru args, summing them */
   while ( (farg = mcPopVal()) != MARK ) {

	/* make sure it's a number! */
	if ( !mcNumber(farg) )
		RT_ERROR("+ requires numbers.");

	/* coerce numbers to same type */
	mcCoerce(result, farg);

	/* add the numbers */
	if ( mcInteger(result) ) {

		/* add two integers */
		tint = mcGet_Int(result) + mcGet_Int(farg);
		mcCpy_Int(result, tint);
	}
	else if ( mcFloat(result) ) {

		/* add two floats */
		tfloat = mcGet_Float(result) + mcGet_Float(farg);
		mcCpy_Float(result, tfloat);
	}
   }

   return result;
}

/* mcMinus() - With two or more arguments, - repeatedly subtracts it's
	args in LR order.  With one arg, it returns it's negative.
*/
CONS mcMinus()
{
   CONS result, farg;
   int tint;
   REAL_NUM tfloat;

   /* (-) => 0 */
   if ( (farg = mcPopVal()) == MARK ) {
	result = NewCons( INT, 0, 0 );
	mcCpy_Int(result, 0);
	return result;
   }
   else result = mcCopyCons( farg );

   /* legal input? */
   if ( !mcNumber(result) )
	RT_ERROR("- requires numbers.");

   /* (- NUMBER) => -NUMBER */
   if ( (farg = mcPopVal()) == MARK ) {

	/* multiply by -1 */
	if ( mcInteger(result) ) {
		tint = mcGet_Int(result) * -1;
		mcCpy_Int(result, tint);
	} else if ( mcFloat(result) ) {
		tfloat = mcGet_Float(result) * -1;
		mcCpy_Float(result, tfloat);
	}

	return result;
   }

   /* loop thru args, subtracting them */
   while ( farg != MARK ) {

	/* make sure it's a number! */
	if ( !mcNumber(farg) )
		RT_ERROR("- requires numbers.");

	/* coerce numbers to same type */
	mcCoerce(result, farg);

	/* subtract the numbers */
	if ( mcInteger(result) ) {

		/* subtract two integers */
		tint = mcGet_Int(result) - mcGet_Int(farg);
		mcCpy_Int(result, tint);
	}
	else if ( mcFloat(result) ) {

		/* subtract two floats */
		tfloat = mcGet_Float(result) - mcGet_Float(farg);
		mcCpy_Float(result, tfloat);
	}

	farg = mcPopVal();
   }

   return result;
}

/* mcMult(args) - Multiplies it's args and returns the product.  If only one
	arg, multiplies it by 1 and returns it.  If no args, returns 1.
*/
CONS mcMult()
{
   CONS result, farg;
   int tint;
   REAL_NUM tfloat;

   /* need a node for the result; assume it's an integer, coerce into
    * float only if need be
    */
   result = NewCons( INT, 0, 0 );

   /* (*) => 1 */
   mcKind(result) = INT;
   mcCpy_Int(result, 1);

   /* loop thru args, multiplying them */
   while ( (farg = mcPopVal()) != MARK ) {

	/* make sure it's a number! */
	if ( !mcNumber(farg) )
		RT_ERROR("* requires numbers.");

	/* coerce numbers to same type */
	mcCoerce(result, farg);

	/* multiply the numbers */
	if ( mcInteger(result) ) {

		/* multiply two integers */
		tint = mcGet_Int(result) * mcGet_Int(farg);
		mcCpy_Int(result, tint);
	}
	else if ( mcFloat(result) ) {

		/* multiply two floats */
		tfloat = mcGet_Float(result) * mcGet_Float(farg);
		mcCpy_Float(result, tfloat);
	}
   }

   return result;
}

/* mcDiv() - With two or more args, repeatedly divides them in LR order.
	If a single arg, returns it's reciprocal.

	NOTE:  Division by zero is trapped.
*/
CONS mcDiv()
{
   CONS result, farg;
   int tint;
   REAL_NUM tfloat;

   /* have args? */
   if ( (farg = mcPopVal()) == MARK ) {
	RT_ERROR("/ requires numbers.");
   } else result = mcCopyCons( farg );

   /* is it a number? */
   if ( !mcNumber(result) )
	RT_ERROR("/ requires numbers.");

   /* better not be zero! */
   if ( mcZero(result) )
	RT_ERROR("Division by zero.");

   /* (/ NUMBER) => 1/NUMBER */
   if ( (farg = mcPopVal()) == MARK ) {
	if ( mcInteger(result) )
		tfloat = (REAL_NUM) mcGet_Int(result);
	else tfloat = mcGet_Float(result);

	tfloat = 1 / tfloat;

	mcKind(result) = FLOAT;
	mcCpy_Float(result, tfloat);
	return result;
   }

   /* loop thru args, dividing them */
   while ( farg != MARK ) {

	/* make sure it's a number! */
	if ( !mcNumber(farg) )
		RT_ERROR("- requires numbers.");

	/* make sure not zero! */
	if ( mcZero(farg) )
		RT_ERROR("Division by zero.");

	/* coerce numbers to same type */
	mcCoerce(result, farg);

	/* divide the numbers */
	if ( mcInteger(result) ) {

		/* divide two integers */
		tint = mcGet_Int(result) / mcGet_Int(farg);
		mcCpy_Int(result, tint);
	}
	else if ( mcFloat(result) ) {

		/* divide two floats */
		tfloat = mcGet_Float(result) / mcGet_Float(farg);
		mcCpy_Float(result, tfloat);
	}

	farg = mcPopVal();
   }

   return result;
}

/* mcAbs() - 1 arg.  Returns it's absolute value. */
CONS mcAbs()
{
   CONS arg, result;
   int tint;
   REAL_NUM tfloat;

   /* need node for result; use type INT just temporarily since
    * rearranging these expressions requires using registers so
    * the arg won't disappear.
    */
   result = NewCons( INT, 0, 0 );

   /* pop the argument */
   arg = mcPopVal();

   /* legal input? */
   if ( !mcNumber(arg) )
	RT_ERROR("ABS requires a number.");

   mcKind(result) = mcKind(arg);

   /* take the absolute value */
   if ( mcInteger(arg) ) {
	tint = (int) abs( mcGet_Int(arg) );
	mcCpy_Int(result, tint);
   } else {
	tfloat = mcGet_Float(arg);
	if ( tfloat < 0 ) tfloat = tfloat * -1;
	mcCpy_Float(result, tfloat);
   }

   return result;
}

/* mcLT() - 2 args */
CONS mcLT()
{
   ENTER;
   REG(num1);
   REG(num2);

   /* get temporary CONS nodes */
   R(num1) = mcCopyCons( mcPopVal() );
   R(num2) = mcCopyCons( mcPopVal() );

   /* make sure they're numbers */
   if ( ! (mcNumber( R(num1) ) && mcNumber( R(num2) )) )
	RT_ERROR("< requires numbers.");

   /* coerce to same type */
   mcCoerce( R(num1), R(num2) );

   if ( mcInteger( R(num1) ) ) {
	if ( mcGet_Int( R(num1) ) < mcGet_Int( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }
   else if ( mcFloat( R(num1) ) ) {
	if ( mcGet_Float( R(num1) ) < mcGet_Float( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }

   MCLEAVE F;
}

/* mcGT() - 2 args */
CONS mcGT()
{
   ENTER;
   REG(num1);
   REG(num2);

   /* get temporary CONS nodes */
   R(num1) = mcCopyCons( mcPopVal() );
   R(num2) = mcCopyCons( mcPopVal() );

   /* make sure they're numbers */
   if ( ! (mcNumber( R(num1) ) && mcNumber( R(num2) )) )
	RT_ERROR("> requires numbers.");

   /* coerce to same type */
   mcCoerce( R(num1), R(num2) );

   if ( mcInteger( R(num1) ) ) {
	if ( mcGet_Int( R(num1) ) > mcGet_Int( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }
   else if ( mcFloat( R(num1) ) ) {
	if ( mcGet_Float( R(num1) ) > mcGet_Float( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }

   MCLEAVE F;
}

/* mcLTE() - 2 args */
CONS mcLTE()
{
   ENTER;
   REG(num1);
   REG(num2);

   /* get temporary CONS nodes */
   R(num1) = mcCopyCons( mcPopVal() );
   R(num2) = mcCopyCons( mcPopVal() );

   /* make sure they're numbers */
   if ( ! (mcNumber( R(num1) ) && mcNumber( R(num2) )) )
	RT_ERROR("<= requires numbers.");

   /* coerce to same type */
   mcCoerce( R(num1), R(num2) );

   if ( mcInteger( R(num1) ) ) {
	if ( mcGet_Int( R(num1) ) <= mcGet_Int( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }
   else if ( mcFloat( R(num1) ) ) {
	if ( mcGet_Float( R(num1) ) <= mcGet_Float( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }

   MCLEAVE F;
}

/* mcGTE() - 2 args */
CONS mcGTE()
{
   ENTER;
   REG(num1);
   REG(num2);

   /* get temporary CONS nodes */
   R(num1) = mcCopyCons( mcPopVal() );
   R(num2) = mcCopyCons( mcPopVal() );

   /* make sure they're numbers */
   if ( ! (mcNumber( R(num1) ) && mcNumber( R(num2) )) )
	RT_ERROR(">= requires numbers.");

   /* coerce to same type */
   mcCoerce( R(num1), R(num2) );

   if ( mcInteger( R(num1) ) ) {
	if ( mcGet_Int( R(num1) ) >= mcGet_Int( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }
   else if ( mcFloat( R(num1) ) ) {
	if ( mcGet_Float( R(num1) ) >= mcGet_Float( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }

   MCLEAVE F;
}

/* mcE() - 2 args */
CONS mcE()
{
   ENTER;
   REG(num1);
   REG(num2);

   /* get temporary CONS nodes */
   R(num1) = mcCopyCons( mcPopVal() );
   R(num2) = mcCopyCons( mcPopVal() );

   /* make sure they're numbers */
   if ( ! (mcNumber( R(num1) ) && mcNumber( R(num2) )) )
	RT_ERROR("= requires numbers.");

   /* coerce to same type */
   mcCoerce( R(num1), R(num2) );

   if ( mcInteger( R(num1) ) ) {
	if ( mcGet_Int( R(num1) ) == mcGet_Int( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }
   else if ( mcFloat( R(num1) ) ) {
	if ( mcGet_Float( R(num1) ) == mcGet_Float( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }

   MCLEAVE F;
}

/* mcNE() - 2 args */
CONS mcNE()
{
   ENTER;
   REG(num1);
   REG(num2);

   /* get temporary CONS nodes */
   R(num1) = mcCopyCons( mcPopVal() );
   R(num2) = mcCopyCons( mcPopVal() );

   /* make sure they're numbers */
   if ( ! (mcNumber( R(num1) ) && mcNumber( R(num2) )) )
	RT_ERROR("<> requires numbers.");

   /* coerce to same type */
   mcCoerce( R(num1), R(num2) );

   if ( mcInteger( R(num1) ) ) {
	if ( mcGet_Int( R(num1) ) != mcGet_Int( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }
   else if ( mcFloat( R(num1) ) ) {
	if ( mcGet_Float( R(num1) ) != mcGet_Float( R(num2) ) ) {
		MCLEAVE T;
	}
	else {
		MCLEAVE F;
	}
   }

   MCLEAVE F;
}

/* mcPos() */
CONS mcPos()
{
   CONS n;

   n = mcPopVal();

   if ( !mcNumber(n) )
	RT_LERROR("POSITIVE?: Requires a number: ", n);

   if ( mcInteger(n) ) {
	if ( mcGet_Int(n) <= 0 )
		return F;
	else return T;
   }

   if ( mcGet_Float(n) <= 0 )
	return F;

   return T;
}

/* mcNeg() */
CONS mcNeg()
{
   CONS n;

   n = mcPopVal();

   if ( !mcNumber(n) )
	RT_LERROR("NEGATIVE: Requires a number: ", n);

   if ( mcInteger(n) ) {
	if ( mcGet_Int(n) >= 0 )
		return F;
	else return T;
   }

   if ( mcGet_Float(n) >= 0 )
	return F;

   return T;
}

/* mcOdd(n) */
CONS mcOdd()
{
   CONS n;

   n = mcPopVal();

   if ( !mcNumber(n) || !mcInteger(n) )
	RT_LERROR("ODD?: Requires an integer: ", n);

   if ( (mcGet_Int(n) % 2) )
	return F;

   return T;
}

/* mcEven() */
CONS mcEven()
{
   CONS n;

   n = mcPopVal();

   if ( !mcNumber(n) || !mcInteger(n) )
	RT_LERROR("EVEN?: Requires an integer: ", n);

   if ( (mcGet_Int(n) % 2) )
	return T;

   return F;
}

/* mcExact() */
CONS mcExact()
{
   CONS n;

   n = mcPopVal();

   if ( !mcNumber(n) )
	RT_LERROR("EXACT?: Requires a number: ", n);

   return F;
}

/* mcInExact(n) */
CONS mcInExact()
{
   CONS n;

   n = mcPopVal();

   if ( !mcNumber(n) )
	RT_LERROR("INEXACT?: Requires a number: ", n);

   return T;
}

/* mcMax() */
CONS mcMax()
{
   CONS head, max;

   max = mcPopVal();
   head = mcPopVal();

   while ( head != MARK ) {

	if ( !mcNumber(head) )
		RT_LERROR("MAX: Requires numbers: ", head);

	if ( mcInteger(max) ) {
		if ( mcInteger(head) ) {
			/* new max? */
			if ( mcGet_Int(max) < mcGet_Int(head) )
				max = head;
		} else {
			if ( mcGet_Int(max) < mcGet_Float(head) )
				max = head;
		}
	} else {
		if ( mcInteger(head) ) {
			/* new max? */
			if ( mcGet_Float(max) < mcGet_Int(head) )
				max = head;
		} else {
			if ( mcGet_Float(max) < mcGet_Float(head) )
				max = head;
		}
	}

	head = mcPopVal();
   }

   return max;
}

/* mcMin() */
CONS mcMin()
{
   CONS head, min;

   min = mcPopVal();
   head = mcPopVal();

   while ( head != MARK ) {

	if ( !mcNumber(head) )
		RT_LERROR("MIN: Requires numbers: ", head);

	if ( mcInteger(min) ) {
		if ( mcInteger(head) ) {
			/* new max? */
			if ( mcGet_Int(min) > mcGet_Int(head) )
				min = head;
		} else {
			if ( mcGet_Int(min) > mcGet_Float(head) )
				min = head;
		}
	} else {
		if ( mcInteger(head) ) {
			/* new min? */
			if ( mcGet_Float(min) > mcGet_Int(head) )
				min = head;
		} else {
			if ( mcGet_Float(min) > mcGet_Float(head) )
				min = head;
		}
	}

	head = mcPopVal();
   }

   return min;
}
