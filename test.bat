echo off
cd tests
cls

echo TESTING - NO TORTURE
echo .
echo Testing MISC
..\scheme -s < misc.s > temp
diff misc.o temp

echo .
echo Testing NUMBERS
..\scheme -s < numbers.s > temp
diff numbers.o temp

echo .
echo Testing PREDS
..\scheme -s < preds.s > temp
diff preds.o temp

echo .
echo Testing FORMS
..\scheme -s < forms.s > temp
diff forms.o temp

echo .
echo Testing BINDING
..\scheme -s < binding.s > temp
diff binding.o temp

rem echo .
rem echo Testing MACROS
rem ..\scheme -s < macros.s > temp
rem diff macros.o temp

echo .
echo Testing CHAR
..\scheme -s < char.s > temp
diff char.o temp

echo .
echo Testing EVAL
..\scheme -s < eval.s > temp
diff eval.o temp

echo .
echo Testing LISTS
..\scheme -s < lists.s > temp
diff lists.o temp

echo .
echo Testing STRINGS
..\scheme -s < strings.s > temp
diff strings.o temp

echo .
echo Testing CONTINUATIONS
..\scheme -s < conts.s > temp
diff conts.o temp

echo .
echo Testing PROLOG
..\scheme -s < logic.s > temp
diff logic.o temp
