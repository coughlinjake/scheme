#\a
#\A
#\space
#\newline
#\(
(char? #\space)
(char? #\newline)
(char? 'a)
(char? #\a)
(char=? #\a #\a)
(char=? #\A #\A)
(char<? #\a #\b)
(char>? #\a #\b)
(char<=? #\a #\b)
(char>=? #\a #\b)
(char<=? #\a #\a)
(char>=? #\b #\b)
(number? (char->integer #\a))
(char->integer #\A)
(char? (integer->char (char->integer #\a)))
(char=? #\a (integer->char (char->integer #\a)))
(exit)
