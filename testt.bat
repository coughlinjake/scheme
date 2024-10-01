echo off
cd tests
cls

echo TESTING - TORTURE ON
echo .
echo Testing MISC
..\scheme -s -t < misc.s > temp
diff misc.o temp

echo .
echo Testing NUMBERS
..\scheme -s -t < numbers.s > temp
diff numbers.o temp

echo .
echo Testing PREDS
..\scheme -s -t < preds.s > temp
diff preds.o temp

echo .
echo Testing FORMS
..\scheme -s -t < forms.s > temp
diff forms.o temp

echo .
echo Testing BINDING
..\scheme -s -t < binding.s > temp
diff binding.o temp

rem echo .
rem echo Testing MACROS
rem ..\scheme -s -t < macros.s > temp
rem diff macros.o temp

echo .
echo Testing CHAR
..\scheme -s -t < char.s > temp
diff char.o temp

echo .
echo Testing EVAL
..\scheme -s -t < eval.s > temp
diff eval.o temp

echo .
echo Testing LISTS
..\scheme -s -t < lists.s > temp
diff lists.o temp

echo .
echo Testing STRINGS
..\scheme -s -t < strings.s > temp
diff strings.o temp

echo .
echo Testing CONTINUATIONS
..\scheme -s -t < conts.s > temp
diff conts.o temp

echo .
echo Testing PROLOG
..\scheme -s -t < logic.s > temp
diff logic.o temp
