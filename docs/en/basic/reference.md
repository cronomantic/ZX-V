**English** · [Español](../../es/basic/reference.md)

# BASIC reference

This page describes every statement, function and operator of ZX-V BASIC,
plus the error messages, the character set and the limits of the machine. It
is a reference: to learn the language step by step, start with the
[BASIC course](README.md); to learn how to type and change programs, read
[the editor](editor.md).

## How to read this reference

Each entry starts with its **syntax**, in a box:

```
PRINT [AT row, column;] [item] [; item]...
```

- Words in UPPER CASE are keywords: type them as they are (in upper or lower
  case, it makes no difference).
- Words in lower case are things you supply: `n`, `x`, `row` are numbers or
  numeric expressions; names ending in `$` (`a$`, `name$`) are strings;
  `line` is a line number; `var` is a variable name.
- `[` `]` mark an optional part; `...` means the part before can be repeated;
  `|` separates alternatives (`IN | OUT` means "IN or OUT").
- Punctuation such as `,`, `;`, `(` and `#` is typed exactly as shown.

Then comes what the keyword does, its parameters and their ranges, a short
example you can type, and related entries (**See also**). When an argument is
out of range you get an error report, written as its code and message, for
example `B Integer out of range` (see [Error messages](#error-messages)).

A **statement** (also called a command) does something: `PRINT`, `GO TO`,
`BEEP`. A **function** gives back a value and is used inside expressions:
`SQR 16`, `LEN a$`, `INKEY$`. Most statements can be typed either in a
program line (with a line number, to be run later) or as a **direct command**
(without a number, run as soon as you press Enter).

> **If you know the Spectrum:** ZX-V BASIC runs Sinclair BASIC programs from
> the 48K, plus `PLAY` from the 128K, and adds many extensions (structured
> loops, procedures, labels, error trapping, sprites, files...). Keywords are
> typed letter by letter, as on the 128K, not with single keys.

## Contents

- [The language](#the-language): lines, numbers, strings, variables, arrays,
  operators
- [Program flow](#program-flow): `GO TO`, `GO SUB`, `IF`, `SELECT CASE`,
  `FOR`, `WHILE`, `REPEAT`, procedures, labels, `RUN`, `STOP`, `CONTINUE`,
  `TRACE`, `PAUSE`, `WAIT`
- [Variables and data](#variables-and-data): `LET`, `DIM`, `READ`, `DATA`,
  `CLEAR`, `DEF FN`
- [Maths](#maths): `SIN`, `SQR`, `INT`, `RND`...
- [Text functions](#text-functions): `LEN`, `CHR$`, `CODE`, `STR$`, `VAL`
- [The screen and printing](#the-screen-and-printing): `PRINT`, `CLS`,
  `SCREEN$`, `ATTR`, `LPRINT`
- [Colours](#colours): `INK`, `PAPER`, `BORDER`, `BRIGHT`, `FLASH`,
  `INVERSE`, `OVER`
- [Drawing with PLOT and DRAW](#drawing-with-plot-and-draw): `PLOT`, `DRAW`,
  `CIRCLE`, `POINT`
- [Video modes and screen effects](#video-modes-and-screen-effects): `MODE`,
  `PAGE`, `SCREEN`, `SCROLL`, `REMAP`, `RASTER`
- [Sprites, tile maps and fast graphics](#sprites-tile-maps-and-fast-graphics):
  `GRAB`, `SPRITE`, `TILE`, `MAP`, `FLOOR`, `BOX`, `DISC`, `POLY`...
- [Sound and music](#sound-and-music): `BEEP`, `PLAY`, `SOUND`, `MUSIC`,
  `SFX`
- [Keyboard, joystick and mouse](#keyboard-joystick-and-mouse): `INPUT`,
  `INKEY$`, `KEY`, `JOY`, `MOUSE`
- [Files and the SD card](#files-and-the-sd-card): `SAVE`, `LOAD`, `CAT`,
  `CD`, `COPY`, `ERASE`...
- [Tape files](#tape-files): `.tap` and `.tzx`
- [Streams and channels](#streams-and-channels): `OPEN #`, `CLOSE #`,
  text files
- [Memory and system](#memory-and-system): `PEEK`, `POKE`, `USR`, `IN`,
  `OUT`, `TIME`, `CLOCK`, `SPEED`, `LANGUAGE`
- [Editing commands](#editing-commands): `LIST`, `EDIT`, `AUTO`, `RENUM`,
  `DELETE`, `FIND`, `REPLACE`, `NEW`
- [Error handling](#error-handling): reports, `ON ERROR`, `ERR`, `ERL`
- [Error messages](#error-messages)
- [Character set](#character-set)
- [Limits](#limits)
- [Keyword index](#keyword-index)

## The language

### Program lines and statements

A program is a list of **lines**. Each line starts with a **line number**
from 1 to 9999; the lines are kept in order of their numbers, whatever order
you type them in. A line can hold several statements separated by colons
(`:`):

```
10 PRINT "Hello": PRINT "again"
20 GO TO 10
```

Line numbers usually go in tens (10, 20, 30...) so there is room to add lines
in between later; [`RENUM`](#renum) tidies them up again. A line typed
without a number is a direct command and runs at once.

When something goes wrong, the error report says where: `2 Variable not
found, 20:1` means statement 1 of line 20 (see [Reports](#reports)).

### Numbers

Numbers can be written as `42`, `-7`, `3.25`, `.5` or in scientific notation:
`1E6` is 1 000 000 and `2.5E-3` is 0.0025. They go from about
±2.9E-39 to ±1.7E38; a result beyond that gives `6 Number too big`, and a
smaller one becomes 0. [`BIN`](#bin) writes a number in binary.

`PRINT` shows up to 8 significant digits, and switches to scientific
notation for very large or very small numbers:

```
PRINT 1/3, 2^31, 1E10, 0.000001
```

shows `0.33333333`, `2.1474836E+9`, `1E+10` and `1E-6`.

Comparisons (`=`, `<`, `>`...) are done with the same precision as the
Spectrum, so `0.1*3=0.3` is true.

### Strings

A string is a piece of text between double quotes: `"Hello"`. To put a quote
inside a string, write it twice: `PRINT "say ""hi"""` shows `say "hi"`. The
empty string is `""`. Strings can be as long as memory allows.

`+` joins strings: `"ZX" + "-V"` is `"ZX-V"`.

**Slicing** takes part of a string. Character positions start at 1:

| Expression | Result | Meaning |
|---|---|---|
| `"hello"(2 TO 4)` | `"ell"` | characters 2 to 4 |
| `"hello"(3)` | `"l"` | character 3 |
| `"hello"(TO 2)` | `"he"` | from the start to 2 |
| `"hello"(4 TO)` | `"lo"` | from 4 to the end |

A slice where the start is after the end is the empty string; a position
outside the string gives `3 Subscript wrong`. Slices work on string variables,
literals and any expression in brackets: `(a$+b$)(2 TO)`. You can also assign
to a slice: `LET a$(1)="J"` changes the first character (the length of `a$`
does not change).

Strings are compared character by character using their codes (see
[Character set](#character-set)), so `"B"<"a"` is true and `"ab"<"abc"` is
true.

### Variables

A variable holds a value under a name.

- **Numeric variables** have names made of letters and digits, starting with
  a letter: `x`, `score`, `level2`. Names can be long (up to 63 characters),
  upper and lower case are the same (`Score` and `SCORE` are one variable),
  and spaces inside the name are ignored (`my score` is `myscore`).
- **String variables** have a single letter followed by `$`: `a$`, `n$`.
  There are 26 of them.
- The control variable of [`FOR`](#for-next) is a single letter.
- A name cannot be a keyword: `LET print=1` is an error. A keyword inside a
  longer name is fine (`total`, `ink2`).

A variable is created the first time it is given a value (with `LET`,
`INPUT`, `READ` or `FOR`). Reading a variable that does not exist gives
`2 Variable not found`. [`RUN`](#run) and [`CLEAR`](#clear) delete all the
variables.

```
10 LET my score=10
20 LET a$="Ann"
30 PRINT a$; " has "; MyScore; " points"
```

### Arrays

An array is a numbered list of values (or a table, with several dimensions).
It has a single-letter name and is created with [`DIM`](#dim):

```
10 DIM t(5): DIM g(3,3)
20 LET t(1)=100: LET g(2,3)=7
30 PRINT t(1); " "; g(2,3); " "; t(5)
```

Subscripts start at 1. Numeric arrays start full of zeros. A numeric array
and a numeric variable can share a letter (`a` and `a(1)` are different). A
string array `a$` replaces the string variable `a$`.

**String arrays** are arrays of characters: the last dimension is the fixed
length of each string. `DIM n$(3,10)` makes 3 strings of 10 characters,
filled with spaces. `n$(2)` is the second string, and `n$(2,1)` its first
character. Assigning a shorter text fills the rest with spaces; a longer one
is cut:

```
10 DIM n$(2,5)
20 LET n$(1)="hello world"
30 PRINT n$(1); "|"; n$(2); "|"
```

shows `hello|     |`.

### Expressions and operators

An expression combines values with operators. When there are several
operators, the ones with higher priority are worked out first:

| Priority | Operators | Meaning |
|---|---|---|
| highest | functions | `SIN x`, `LEN a$`... take only the value right after them |
| 10 | `^` | power |
| 9 | `-` (in front) | change of sign |
| 8 | `*` `/` | multiplication, division |
| 6 | `+` `-` | addition (and joining strings), subtraction |
| 5 | `=` `<` `>` `<=` `>=` `<>` | comparisons |
| 4 | `NOT` | logical not |
| 3 | `AND` | logical and |
| lowest | `OR` | logical or |

Operators with the same priority are worked out from left to right, so
`2^3^2` is `(2^3)^2` = 64. Change of sign goes after the power: `-2^2` is
-4. Brackets change the order: `(1+2)*3` is 9.

Functions take only the value that follows them: `SQR 16+9` is `(SQR 16)+9`
= 13, and `SQR (16+9)` is 5. When in doubt, use brackets.

Dividing by zero gives `6 Number too big`. A power of a negative number
gives `A Invalid argument`.

### True and false

Comparisons give 1 when true and 0 when false: `PRINT 3>2` shows 1. `IF`,
`WHILE` and `UNTIL` take any number, and treat 0 as false and anything else as
true.

| Expression | Value |
|---|---|
| `NOT x` | 1 if `x` is 0, otherwise 0 |
| `x AND y` | `x` if `y` is not 0, otherwise 0 |
| `x OR y` | 1 if `y` is not 0, otherwise `x` |
| `a$ AND y` | `a$` if `y` is not 0, otherwise `""` |

So `IF a>0 AND a<10 THEN ...` works as you expect, and a trick like
`PRINT ("yes" AND ok); ("no" AND NOT ok)` prints one word or the other.

### Whole-number arguments

Many statements need whole numbers (a line number, a colour, a coordinate).
If you give a number with decimals it is rounded to the nearest whole number
(2.5 becomes 3). If the result is out of the allowed range, you get
`B Integer out of range` (or `K Invalid colour` for colours).

## Program flow

### GO TO

```
GO TO line
GO TO @label
```

Jumps to a line and carries on from there. `line` can be any expression
(`GO TO n*100`). If that line does not exist, it goes to the next one after
it; if there is none, the program ends. With a [label](#label) it jumps to
that exact statement.

```
10 PRINT "forever"
20 GO TO 10
```

Press BREAK (Esc) to stop it.

**See also:** [GO SUB](#go-sub-return), [@label](#label), [RUN](#run).

### GO SUB, RETURN

```
GO SUB line
GO SUB @label
RETURN
```

`GO SUB` jumps to a subroutine, like `GO TO`, but remembers where it came
from; `RETURN` goes back to the statement after the `GO SUB`. Subroutines can
call other subroutines (up to 512 levels). `RETURN` without a `GO SUB` gives
`7 RETURN without GOSUB`.

```
10 GO SUB 100: GO SUB 100
20 STOP
100 PRINT "in the subroutine"
110 RETURN
```

**See also:** [DEF PROC](#def-proc-proc-end-proc), which is usually clearer.

### IF, THEN, ELSE

```
IF condition THEN statements
IF condition THEN statements ELSE statements

IF condition THEN
  lines
ELSE IF condition THEN
  lines
ELSE
  lines
END IF
```

If the condition is true (not 0), the statements after `THEN` run. If it is
false, they are skipped: the program carries on at the statements after
`ELSE`, if there is one on the same line, or at the next line. A number after
`THEN` is an error (`IF a THEN 100`; write `IF a THEN GO TO 100`).

```
10 LET n=-5
20 IF n<0 THEN PRINT "negative" ELSE PRINT "zero or positive"
30 IF n<0 THEN PRINT "a": PRINT "b" ELSE PRINT "c": PRINT "d"
```

Line 30 prints `a` and `b`. Everything after `THEN` up to `ELSE` belongs to
the true part, and everything after `ELSE` up to the end of the line to the
false part. With several `IF` on one line, each `ELSE` goes with the nearest
`IF`.

**The block IF.** If `THEN` is the last thing on the line, the `IF` opens a
block of several lines, which ends at `END IF`. Inside it there can be
`ELSE IF condition THEN` parts (also at the end of their line), tried in
order if nothing before was true, and an `ELSE` part for when none is. Only
one part runs; then the program carries on after `END IF`.

```
10 INPUT "Age? ";a
20 IF a<13 THEN
30 PRINT "Child"
40 ELSE IF a<18 THEN
50 PRINT "Teenager"
60 ELSE
70 PRINT "Adult"
80 IF a>=65 THEN PRINT "Retired"
90 END IF
```

- `ELSE`, `ELSE IF` and `END IF` start their line. `ELSE` can be followed by
  statements on the same line (`ELSE PRINT "Adult"`), and `END IF` by others
  separated by `:`. An `ELSE` that does not start the line belongs to a
  one-line IF.
- Blocks can go one inside another, with no limit, and contain one-line IFs,
  loops or `GO SUB`. You can leave a block with `GO TO`.
- If a block has no `END IF`, `Y Structure error` when it is looked for. It
  only works in a program: as a direct command, `IF a THEN` does nothing.

**See also:** [SELECT CASE](#select-case), [WHILE](#while-wend),
[Expressions](#expressions-and-operators).

### SELECT CASE

```
SELECT CASE value
CASE values
  lines
CASE ELSE
  lines
END SELECT
```

Chooses a part by `value`, a number or a string: the part of the first `CASE`
that matches runs (or the `CASE ELSE` part if none does), and the program
carries on after `END SELECT`. If none matches and there is no `CASE ELSE`,
nothing runs. Each `CASE` has a list of values separated by commas, which can
be:

| Value | Matches if |
|---|---|
| `5`, `"yes"`, `a+1` | `value` is equal |
| `1 TO 9`, `"a" TO "m"` | `value` is between the two (both included) |
| `< 0`, `>= 100`, `<> 7` | `value` passes the comparison |

```
10 INPUT "A key? ";k$
20 SELECT CASE k$
30 CASE "q", "Q": PRINT "Up"
40 CASE "a", "A": PRINT "Down"
50 CASE "0" TO "9"
60 PRINT "A digit"
70 CASE ELSE: PRINT "Something else"
80 END SELECT
```

`CASE` can go on its own line or after `:`, and the statements of each part on
the same line or the following ones; it can all even go on one line
(`SELECT CASE n: CASE 1: PRINT "one": CASE ELSE: PRINT "other": END SELECT`).
`SELECT CASE` can go one inside another. If the value and those of a `CASE`
are not the same type, `C Nonsense in BASIC`; if `END SELECT` is missing,
`Y Structure error`.

**See also:** [IF](#if-then-else).

### FOR, NEXT

```
FOR var = first TO last [STEP step]
NEXT var
```

Repeats the statements between `FOR` and `NEXT`, with the variable going
from `first` to `last`, adding `step` each time (1 if there is no `STEP`).
The variable is a single letter. With a negative step it counts down. If the
loop should not run at all (`FOR i=1 TO 0`), the program jumps to after the
matching `NEXT`; if there is none, `I FOR without NEXT`.

```
10 FOR i=1 TO 10 STEP 3: PRINT i;" ";: NEXT i
20 PRINT
30 FOR i=3 TO 1 STEP -1: PRINT i: NEXT i
```

After the loop the variable keeps the first value past the end (here, 0).
`NEXT` with a variable that is not a `FOR` variable gives `1 NEXT without
FOR`.

**See also:** [WHILE](#while-wend), [REPEAT](#repeat-until).

### WHILE, WEND

```
WHILE condition
WEND
```

Repeats the statements between `WHILE` and `WEND` while the condition is true.
The condition is checked at the start, so if it is false the first time, the
loop is skipped entirely.

```
10 LET n=1
20 WHILE n<100: LET n=n*2: WEND
30 PRINT n
```

shows 128. Loops can be nested. `WEND` without its `WHILE`, or a `WHILE`
without its `WEND`, gives `Y Structure error`.

**See also:** [REPEAT](#repeat-until), [FOR](#for-next).

### REPEAT, UNTIL

```
REPEAT
UNTIL condition
```

Repeats the statements between `REPEAT` and `UNTIL` until the condition is
true. The condition is checked at the end, so the loop runs at least once.

```
10 REPEAT
20 LET d=INT (RND*6)+1: PRINT d
30 UNTIL d=6
```

**See also:** [WHILE](#while-wend).

### DEF PROC, PROC, END PROC

```
DEF PROC name [(param [, param]...)]
END PROC
PROC name [(value [, value]...)]
```

A **procedure** is a named piece of program. `DEF PROC` starts it and
`END PROC` ends it; `PROC` runs it and then comes back. The parameters
receive the values given in `PROC`; they are local: their old values are
restored at `END PROC`. A numeric parameter is a numeric variable name; a
string parameter is a letter with `$`. Procedures can call themselves.

When the program reaches a `DEF PROC` while running normally, it skips the
whole procedure, so you can put procedures anywhere.

```
10 PROC greet("Ann", 2)
20 PROC greet("Bob", 1)
30 STOP
100 DEF PROC greet(n$, times)
110 FOR i=1 TO times: PRINT "Hello, "; n$: NEXT i
120 END PROC
```

Names follow the rules of numeric variables (letters and digits, case and
spaces ignored). Up to 16 parameters. Errors: `Z PROC not found` (no
`DEF PROC` with that name), `Q Parameter error` (wrong number or type of
values), `Y Structure error` (`END PROC` outside a procedure, or a
`DEF PROC` without `END PROC`).

A procedure does not return a value: to give a result back, set a variable.

**See also:** [LOCAL](#local), [GO SUB](#go-sub-return).

### LOCAL

```
LOCAL var [, var]...
```

Inside a procedure, makes the variables local: they start as 0 (or `""`) and
get their old values back at `END PROC`. Outside a procedure,
`Y Structure error`.

```
10 LET i=99: PROC count: PRINT "i is still "; i
20 STOP
100 DEF PROC count
110 LOCAL i
120 FOR i=1 TO 3: PRINT i: NEXT i
130 END PROC
```

**See also:** [DEF PROC](#def-proc-proc-end-proc).

### @label

```
@name
```

A statement made of `@` and a name marks a place in the program. `GO TO`,
`GO SUB`, `RUN`, `RESTORE` and `ON ERROR GO TO` can jump to it by name
instead of by line number, to the exact statement (it does not need to be
the first one on its line). Labels do not change when you renumber. A label
that does not exist gives `X Label not found`.

```
10 GO SUB @hello: GO SUB @hello
20 STOP
30 @hello: PRINT "hello": RETURN
```

**See also:** [GO TO](#go-to), [RENUM](#renum).

### RUN

```
RUN [line | @label]
```

Starts the program from the beginning, or from that line or label. Before
starting it clears everything like [`CLEAR`](#clear): deletes the
variables, clears the screen, closes open files and turns off
`ON ERROR`. To start without losing the variables, use `GO TO`.

**See also:** [CONTINUE](#continue), [STOP](#stop).

### STOP

```
STOP
```

Stops the program with the report `9 STOP statement`. [`CONTINUE`](#continue)
carries on from the next statement. `ON ERROR` does not catch `STOP`.

### CONTINUE

```
CONTINUE
```

Carries on after a stop: after `STOP` or BREAK, from the next statement;
after an error, it repeats the statement that failed (useful once you have
fixed the cause, for example by giving a missing variable a value).

### TRACE

```
TRACE ON
TRACE #stream
TRACE OFF
```

To follow where a program goes: each time a line starts running (also when
it comes back to it with `GO TO`, `NEXT` or `RETURN`), its number is printed
in brackets, like `[10][20][30]`. With `TRACE ON` it goes to the program
screen, mixed with what the program prints; with `TRACE #3`, to the printer
(the file `printer.txt`), or to the file of a stream opened with `OPEN #`, so
that the screen is not spoilt. `TRACE OFF` turns it off; it can also go inside
the program, to follow only one part. `NEW` turns it off. With a stream that
is not open, `O Invalid stream`.

```
TRACE ON: RUN
```

### REM

```
REM any text
```

A remark: everything after `REM` up to the end of the line is ignored, so
it is the place for comments.

```
10 REM Space game, by Ann
```

### PAUSE

```
PAUSE n
```

Waits `n` fiftieths of a second (`PAUSE 50` is one second, as on the
Spectrum), or until a key is pressed, whichever comes first. `PAUSE 0` waits for a key for ever. Only keys pressed
after the `PAUSE` starts count. `n` goes from 0 to 65535.

```
10 PRINT "Press a key": PAUSE 0: PRINT "Thanks"
```

**See also:** [WAIT](#wait), [INKEY$](#inkey).

### WAIT

```
WAIT [n]
```

Waits for the next frame, and until `n` frames (1 if omitted) have passed
since the previous `WAIT`. There are 60 frames a second. This keeps a game
loop at a steady speed even if some turns of the loop take longer than
others: `WAIT 2` gives 30 turns a second. `n` goes from 0 to 65535.

```
10 FOR x=0 TO 200: BOX 0,x,80,8,8: WAIT: NEXT x
```

**See also:** [PAUSE](#pause), [TIME](#time).

## Variables and data

### LET

```
LET var = value
```

Gives a value to a variable, creating it if needed. `LET` is needed
(`a=1` on its own is an error). The value must match the type: a number for
numeric variables, a string for `$` variables.

```
10 LET a=5: LET a=a+1: LET b$="ZX"
20 LET b$(2)="-": PRINT a; b$
```

**See also:** [Variables](#variables), [DIM](#dim).

### DIM

```
DIM letter(size [, size]...)
DIM letter$(size [, size]...)
```

Creates an array (see [Arrays](#arrays)), replacing any array with the same
name. Up to 16 dimensions; each size goes from 1 to 65535. Numeric arrays
start full of zeros and string arrays full of spaces. A subscript out of
range gives `3 Subscript wrong`.

```
10 DIM s(10): LET s(3)=42: PRINT s(3), s(4)
```

### READ, DATA, RESTORE

```
DATA value [, value]...
READ var [, var]...
RESTORE [line | @label]
```

`DATA` lines hold a list of values inside the program; `READ` takes them one
by one, in order, into variables. The values can be any expression of the
right type. `RESTORE` makes the next `READ` start again from the first `DATA`
(or from the first `DATA` at or after that line or label). Reading past the
last value gives `E Out of DATA`.

```
10 FOR i=1 TO 3
20 READ n$, age: PRINT n$; " is "; age
30 NEXT i
40 DATA "Ann", 12, "Bob", 9, "Eve", 10+1
```

`DATA` does nothing when the program reaches it.

### CLEAR

```
CLEAR [address]
```

Deletes all variables, clears the screen, closes the files opened with
`OPEN #`, forgets the `GO SUB` and loop returns, turns off `ON ERROR` and
`CLIP`, and makes `READ` start from the first `DATA`. Sprites and other
graphics are kept. [`RUN`](#run) does a `CLEAR` first.

With an address, it also moves a memory limit (useful only for advanced
programs): below 65536 it sets the "RAMTOP" of the [virtual Spectrum
memory](#the-virtual-spectrum-memory) (at least 23821, otherwise
`M RAMTOP no good`); from 65536 up, it is a real address, and the graphics
store is moved just below it, leaving the memory above it free for your own
use.

### DEF FN, FN

```
DEF FN letter([param [, param]...]) = expression
DEF FN letter$([param [, param]...]) = string expression
FN letter(values)
```

Defines your own function with a one-letter name and one-letter parameters
(`x`, or `a$` for strings). `FN` calls it. `DEF FN` can be anywhere in the
program and does nothing when reached. Calling a function that is not
defined gives `P FN without DEF`; the wrong number or type of values gives
`Q Parameter error`.

```
10 DEF FN h(x,y)=SQR (x*x+y*y)
20 DEF FN u$(a$)=a$+"!"
30 PRINT FN h(3,4); " "; FN u$("hi")
```

shows `5 hi!`.

## Maths

### SIN, COS, TAN

```
SIN x     COS x     TAN x
```

Sine, cosine and tangent of an angle `x` in **radians** (a full turn is
`2*PI`). To use degrees, multiply by `PI/180`.

```
PRINT SIN (PI/2); " "; COS (60*PI/180)
```

### ASN, ACS, ATN

```
ASN x     ACS x     ATN x
```

Arcsine, arccosine and arctangent: the angle, in radians, whose sine, cosine
or tangent is `x`. `ASN` and `ACS` need `x` from -1 to 1 (otherwise
`A Invalid argument`).

```
PRINT 4*ATN 1
```

### EXP, LN

```
EXP x     LN x
```

`EXP x` is e (2.7182818...) to the power `x`. `LN x` is the natural
logarithm; `x` must be greater than 0 (otherwise `A Invalid argument`). For a
logarithm in base 10 use `LN x/LN 10`.

### SQR

```
SQR x
```

Square root. `x` must not be negative (`A Invalid argument`).

```
PRINT SQR 2
```

### ABS, SGN, INT

```
ABS x     SGN x     INT x
```

- `ABS x`: `x` without its sign (`ABS -3` is 3).
- `SGN x`: -1, 0 or 1 depending on whether `x` is negative, zero or positive.
- `INT x`: the whole number at or below `x`: `INT 2.7` is 2, and `INT -2.5`
  is -3. To round to the nearest, use `INT (x+0.5)`.

### PI

```
PI
```

The number π, 3.1415927.

### RND, RANDOMIZE

```
RND
RANDOMIZE [n]
```

`RND` gives a random number from 0 up to (not including) 1. For a whole
number from 1 to 6, use `INT (RND*6)+1`.

`RANDOMIZE n` (0 to 65535) starts the sequence at a fixed point, so the same
"random" numbers come out again. `RANDOMIZE` or `RANDOMIZE 0` starts it from
the time since the machine was switched on.

```
10 RANDOMIZE
20 FOR i=1 TO 5: PRINT INT (RND*6)+1;" ";: NEXT i
```

> **If you know the Spectrum:** `RND` gives exactly the same sequence as on
> the 48K.

### BIN

```
BIN digits
```

Writes a number in binary: `BIN 1010` is 10. Handy for designing graphics
with [`POKE USR`](#user-defined-graphics-udg).

## Text functions

### LEN

```
LEN a$
```

The number of characters in a string. `LEN ""` is 0.

```
PRINT LEN "ZX-V"
```

### CHR$, CODE

```
CHR$ n     CODE a$
```

`CHR$ n` is the character with code `n` (0 to 255), and `CODE a$` the code of
the first character of `a$` (0 if it is empty). See the
[Character set](#character-set).

```
PRINT CHR$ 65; " "; CODE "a"
```

shows `A 97`.

### STR$, VAL, VAL$

```
STR$ x     VAL a$     VAL$ a$
```

- `STR$ x` turns a number into a string, as `PRINT` would show it:
  `STR$ 3.5` is `"3.5"`.
- `VAL a$` works out the string as a numeric expression: `VAL "2*3"` is 6.
  If the text is not a valid expression, `C Nonsense in BASIC`.
- `VAL$ a$` works out the string as a string expression: `VAL$ """x"""` is
  `"x"`.

```
10 LET a$="12": LET b$="30"
20 PRINT a$+b$; " "; VAL a$+VAL b$
```

shows `1230 42`.

## The screen and printing

The program screen has an **upper part**, where `PRINT` and the drawing
commands work, and a **lower part** of two lines at the bottom, used by
`INPUT` and by the reports. In the normal mode 0 the screen has 24 rows of 32
characters (256 × 192 pixels), and `PRINT` can use rows 0 to 21; in mode 1
it has 48 rows of 64 characters (see [MODE](#mode)).

When printing goes past the last row, the screen scrolls up. Each time a
screenful has scrolled, it stops and asks `scroll?`: press N, Space or BREAK
to stop the program (`D BREAK - CONT repeats`), or any other key to go on.

### PRINT

```
PRINT [item] [separator] [item]...
```

Shows numbers and text on the screen. The items can be:

| Item | Effect |
|---|---|
| a number or string expression | prints its value |
| `AT row, column` | moves the print position (row 0 is at the top) |
| `TAB column` | moves right to that column, filling with spaces |
| `INK n`, `PAPER n`, `FLASH n`, `BRIGHT n`, `INVERSE n`, `OVER n` | changes the colours for the rest of this `PRINT` only (see [Colours](#colours)) |
| `#stream` | prints through that stream (see [Streams](#streams-and-channels)) |

And between them, the separators:

| Separator | Effect |
|---|---|
| `;` | nothing: the next item goes right after |
| `,` | moves to the next half of the line (to the next column 0 or 16; in mode 1, every 16 columns) |
| `'` | starts a new line |

If the `PRINT` does not end with a separator, it moves to a new line at the
end. `PRINT` on its own prints an empty line.

```
10 PRINT "Name", "Score"
20 PRINT "Ann", 120
30 PRINT AT 10,5; "at row 10"; TAB 25; "tab"
40 PRINT "one"'"two"; " and three"
50 PRINT INK 2; "red"; INK 0; " black"
```

`AT` outside the screen gives `5 Out of screen`. `TAB` wraps round the line
width.

**See also:** [LPRINT](#lprint-llist), [INPUT](#input), [Colours](#colours).

### CLS

```
CLS
```

Clears the screen with the current `PAPER` colour and puts the print position
back at the top left corner.

### SCREEN$, ATTR

```
SCREEN$ (row, column)
ATTR (row, column)
```

`SCREEN$` gives the character printed at that position (`""` if it does not
recognise it, for example a drawing). `ATTR` gives the colours of that
character cell as one number: `FLASH`×128 + `BRIGHT`×64 + `PAPER`×8 + `INK`.
Positions outside the screen give `B Integer out of range`.

```
10 PRINT AT 0,0; INK 2; PAPER 6; "A"
20 PRINT SCREEN$ (0,0); " "; ATTR (0,0)
```

shows `A 50` (6×8 + 2).

**See also:** [POINT](#point), [PIXEL](#pixel).

### LPRINT, LLIST

```
LPRINT [items]
LLIST [line]
```

Like `PRINT` and `LIST`, but to the **printer**. ZX-V has no paper printer:
the printer is the text file `printer.txt` at the root of the SD card, and
everything printed is added to its end. Colours and `AT` (except its column)
are ignored. Lines are up to 64 characters long (32 in mode 0).

```
LPRINT "Shopping list": LPRINT "- milk"
```

**See also:** [COPY](#copy), [Streams](#streams-and-channels).

## Colours

Each character cell of the screen has an ink (the colour of the character)
and a paper (the background). Colours are numbered from 0 to 7, and
`BRIGHT 1` makes them brighter:

| Number | Colour | With `BRIGHT 1` |
|---|---|---|
| 0 | black | black |
| 1 | blue | bright blue |
| 2 | red | bright red |
| 3 | magenta | bright magenta |
| 4 | green | bright green |
| 5 | cyan | bright cyan |
| 6 | yellow | bright yellow |
| 7 | white (light grey) | bright white |

The [sprite and fast graphics commands](#sprites-tile-maps-and-fast-graphics),
[`RASTER BORDER`](#raster) and [`REMAP`](#remap) use the 16 colours directly,
from 0 to 15: 0–7 are the colours above and 8–15 their bright versions (10
is bright red). Colour 8 is black too, and is used as "transparent" in
sprites.

On ZX-V, each pixel has its own colour: drawing a pixel only changes the
colour of that pixel, never its neighbours.

> **If you know the Spectrum:** there is no colour clash. `PLOT`, `DRAW` and
> `CIRCLE` paint with the ink only, pixel by pixel; `ATTR` still reports the
> last colours written in each cell.

`INK`, `PAPER`, `FLASH`, `BRIGHT`, `INVERSE` and `OVER` can be used in two
ways:

- As statements, they change the colours for everything printed and drawn
  from then on (permanent colours).
- As items inside `PRINT`, `INPUT`, `PLOT`, `DRAW` and `CIRCLE`, followed by
  `;`, they apply to that statement only (temporary colours).

```
10 PAPER 1: INK 6: CLS
20 PRINT "yellow on blue"
30 PRINT PAPER 2; INK 7; "white on red, only here"
40 PRINT "yellow on blue again"
```

### INK, PAPER

```
INK n     PAPER n
```

Ink and paper colour, from 0 to 7 (see the table), or:

- **8**, transparent: keeps the colour that each cell already has;
- **9**, contrast: white or black, whichever shows best against the other
  colour.

Other values give `K Invalid colour`. `PAPER` takes effect on the whole
screen with the next `CLS`.

### BORDER

```
BORDER n
```

Colour of the border around the screen, 0 to 7. The lower part of the
screen (the two lines of `INPUT` and the reports) uses the border colour as
its paper.

```
BORDER 0: PAPER 0: INK 4: CLS
```

**See also:** [RASTER BORDER](#raster), for stripes of colour.

### BRIGHT, FLASH

```
BRIGHT n     FLASH n
```

`BRIGHT 1` uses the bright version of the ink and paper; `BRIGHT 0` the
normal one. `FLASH 1` makes the cell swap its ink and paper about every third
of a second. 8 is transparent (keeps what the cell had). Other values give
`K Invalid colour`.

```
PRINT FLASH 1; "GAME OVER"
```

### INVERSE, OVER

```
INVERSE n     OVER n
```

`INVERSE 1` prints with ink and paper swapped. `OVER 1` prints over what is
already there, changing each pixel of the character between ink and paper
(printing the same thing twice with `OVER 1` rubs it out). 0 turns them off;
other values give `K Invalid colour`.

```
10 PRINT AT 5,5; "X": PRINT AT 5,5; OVER 1; "O"
```

## Drawing with PLOT and DRAW

These commands draw on the program screen with the ink colour. Their
coordinates are in pixels, with **(0, 0) at the bottom left** corner: `x`
goes from 0 to 255 to the right and `y` from 0 to 175 upwards (the two
bottom rows belong to the lower part of the screen). In mode 1, `x` goes to
511 and `y` to 367. A point outside the screen gives
`B Integer out of range`.

You can put colour items at the start, as in `PLOT INK 2; 10,10`.

> The [fast graphics commands](#sprites-tile-maps-and-fast-graphics) (`BOX`,
> `SPRITE`...) count `y` from the top instead, like `PRINT AT`.

### PLOT

```
PLOT [colour items;] x, y
```

Draws a point and makes it the start for the next `DRAW`.

```
10 FOR x=0 TO 255 STEP 4: PLOT x, 88+40*SIN (x/20): NEXT x
```

### DRAW

```
DRAW [colour items;] dx, dy [, angle]
```

Draws a line from the last point to the point `dx` pixels to the right and
`dy` pixels up (negative values go left and down). With `angle` (in radians)
it draws an arc instead, turning that much on the way: `PI` draws half a
circle, and a positive angle curves anticlockwise.

```
10 PLOT 50,50: DRAW 100,0: DRAW 0,100: DRAW -100,-100
20 PLOT 150,60: DRAW 60,0,PI
```

### CIRCLE

```
CIRCLE [colour items;] x, y, radius
```

Draws a circle with its centre at `x`, `y`.

```
10 FOR r=10 TO 80 STEP 10: CIRCLE INK r/10; 128,88,r: NEXT r
```

### POINT

```
POINT (x, y)
```

1 if the pixel at `x`, `y` was drawn with ink, 0 if it is paper. It sees what
`PRINT`, `PLOT`, `DRAW` and `CIRCLE` draw, but not the fast graphics
commands (use [`PIXEL`](#pixel) for those).

```
10 PLOT 10,10: PRINT POINT (10,10); POINT (11,10)
```

## Video modes and screen effects

### MODE

```
MODE n
MODE 0, width, height
```

Chooses the screen mode and clears the screen:

| Mode | Pixels | Text |
|---|---|---|
| 0 | 256 × 192 | 32 columns × 24 rows (the one at start) |
| 1 | 512 × 384 | 64 columns × 48 rows |

`MODE` clears both pages, shows page 0 and removes scrolling, `REMAP`,
`RASTER` and `CLIP`. In mode 0, the page can also be larger than the screen,
so that the screen is a window that you move over it with
[`SCROLL`](#scroll): `MODE 0, 512, 384`, `MODE 0, 1024, 192` or
`MODE 0, 256, 768`. Then `PRINT AT`, `PLOT` and the rest cover the whole page
(`PRINT AT` reaches column 127 on a 1024-pixel-wide page). Other sizes give
`A Invalid argument`.

```
10 MODE 1: CIRCLE 256,184,150: PRINT AT 24,26; "Mode 1"
```

### PAGE, SCREEN

```
PAGE n
SCREEN n
```

ZX-V has two screen pages, 0 and 1. `PAGE` chooses the one that is shown;
`SCREEN` chooses the one where `PRINT`, `CLS`, `PLOT`, sprites and so on
draw. Drawing on the hidden page and then showing it gives smooth animation
without flicker (double buffering). Values other than 0 and 1 give
`A Invalid argument`.

```
10 LET s=1
20 SCREEN s: CLS: CIRCLE 128,88,RND*80
30 PAGE s: LET s=1-s: WAIT 10: GO TO 20
```

Error reports and `INPUT` always appear on the page being shown. The editor
also uses page 1: when a program uses page 1 again, the text and the `PLOT`
and `DRAW` drawings are restored, but the fast graphics (sprites, maps...)
drawn there are lost.

### SCROLL

```
SCROLL x, y
SCROLL
```

Moves the picture without redrawing it: pixel (`x`, `y`) of the page appears
at the top left corner of the screen, and the page wraps round at the edges.
`SCROLL` alone puts it back. This is how a game scrolls a large
[`MODE 0`](#mode) page.

```
10 MODE 0,1024,192: CIRCLE 600,100,50
20 FOR x=0 TO 1023 STEP 2: SCROLL x,0: WAIT: NEXT x
```

### REMAP

```
REMAP colour, newcolour [; colour, newcolour]...
REMAP #table; colour, newcolour [; colour, newcolour]...
REMAP #table
REMAP
```

Shows one colour as another everywhere on the screen, instantly and without
redrawing: after `REMAP 1,10`, everything blue is shown bright red. Colours go
from 0 to 15 (otherwise `K Invalid colour`). This is useful for fades,
flashing and colour cycling.

There are three remap tables, 0 to 2. Without `#`, `REMAP` changes table 0,
which is the one normally in use. `REMAP #t; ...` changes table `t` without
showing it; `REMAP #t` makes the whole screen use table `t` (so all its
changes appear at once); [`RASTER REMAP`](#raster) uses a table for some
lines only. `REMAP` alone resets the three tables and goes back to table 0.

```
10 PRINT INK 1; "blinking"
20 REMAP 1,10: PAUSE 25: REMAP 1,1: PAUSE 25: GO TO 20
```

### RASTER

```
RASTER BORDER y1, y2, colour
RASTER REMAP y1, y2, table
RASTER SCROLL y1, y2, x [, y]
RASTER
```

Changes things line by line on the screen, without using any processor time
while the program runs:

- `RASTER BORDER`: the lines from `y1` to `y2` have a border of that colour
  (0 to 15).
- `RASTER REMAP`: those lines use remap table 0 to 2 (see [REMAP](#remap)).
- `RASTER SCROLL`: those lines have their own horizontal scroll `x`, and, if
  `y` is given, show the lines of the page from line `y` onwards (for split
  screens that scroll separately, and parallax).
- `RASTER` alone turns all this off.

Lines are pixel rows counted from the top of the screen (0). The border lines
above the picture are negative (down to -24 in mode 0, -48 in mode 1) and
those below go past the height (up to 215 in mode 0, 431 in mode 1).
`MODE` turns this off.

```
10 FOR y=-24 TO 215 STEP 4: RASTER BORDER y,y+1,2: RASTER BORDER y+2,y+3,6: NEXT y
```

## Sprites, tile maps and fast graphics

These commands use the ROM's graphics engine to draw games quickly: sprites
with transparency, scrolling tile maps, a perspective floor ("mode 7") and
filled shapes. They differ from `PLOT` and `DRAW` in a few ways:

- **Coordinates** are in pixels from the **top left** corner of the page,
  with `y` going down, like `PRINT AT`. They can be negative or off the page:
  whatever falls outside is simply not drawn.
- **Colours** are the 16 colours by number, 0 to 15 (see the
  [colour table](#colours)); a number above 15 gives `K Invalid colour`.
  In sprites and tiles, colour 8 is transparent.
- They draw on the drawing page chosen with [`SCREEN`](#page-screen). For
  smooth animation, draw on the hidden page, show it with `PAGE` and wait
  with [`WAIT`](#wait).
- `POINT`, `ATTR` and `SCREEN$` do not see what they draw; [`PIXEL`](#pixel)
  does. A cell with `FLASH` stops flashing when one of these commands draws
  over it.

Sprites, tiles, maps and textures are kept by number in a **graphics store**
in memory. They survive `RUN`, `CLEAR` and `LOAD` (so a loader can prepare
the graphics and then load the game), and `NEW` deletes them. Defining a
number again replaces it. `SAVE "name" GRAPHICS` saves the whole store in a
`.gfx` file, and `LOAD "name" GRAPHICS` loads one (see [SAVE](#save) and
[LOAD](#load)). You can also draw them on the PC with the graphics editor
`zxv-gfx`, which comes with the emulator, or on the machine itself with
`LOAD "ROM:gfxedit"`, or make `.gfx` files from PNG images with the SDK tool
`png2gfx` (see the [SDK graphics
guide](../sdk/graphics.md#the-graphics-editor-for-the-pc-zxv-gfx)).

A sprite, tile set, map or texture that does not exist gives
`A Invalid argument`; numbers out of range give `B Integer out of range`; and
if the graphics do not fit in memory, `4 Out of memory`.

### GRAB

```
GRAB n, x, y, width, height [, t]
GRAB n, width, a$
```

Defines sprite `n` (0 to 255).

- The first form copies a rectangle of the drawing page, with its top left
  corner at `x`, `y` (up to 1024 × 1024 pixels). If `t` is given, that colour
  becomes transparent.
- The second form takes the pixels from a string, one character per pixel,
  row after row, `width` pixels per row: the hexadecimal digits `0`–`9` and
  `a`–`f` are colours 0 to 15, and any other character (`.`, a space...) is
  transparent.

```
10 GRAB 0,8,"..2222..22222222.222222...2222..."
20 FOR x=0 TO 240 STEP 16: SPRITE 0,x,80: NEXT x
```

### SPRITE

```
SPRITE n, x, y [, flip [, c$ [, width, height]]]
```

Draws sprite `n` with its top left corner at `x`, `y`. Its transparent pixels
let the background show through.

- `flip`: 0 normal, 1 mirrored left to right, 2 upside down, 3 both.
- `c$` changes colours: colour `i` is drawn as the colour given by character
  `i` of the string (counting from 0), a hexadecimal digit. A character that
  is not a hexadecimal digit (or a string too short) leaves that colour as it
  is: `"0a"` draws colour 1 as 10 and leaves the rest. `""` changes nothing.
- `width`, `height` (1 to 4096): draws it stretched or shrunk to that size,
  for things that come closer or move away.

```
10 GRAB 0,4,"1111122112211111"
20 SPRITE 0,10,10: SPRITE 0,30,10,0,"0a": SPRITE 0,50,10,0,"",32,32
```

**See also:** [HIT](#hit), [GRAB](#grab).

### HIT

```
HIT(a, ax, ay, b, bx, by)
```

Collision test: 1 if any solid (not transparent) pixel of sprite `a` drawn at
`ax`, `ay` would land on a solid pixel of sprite `b` drawn at `bx`, `by`;
otherwise 0. Nothing is drawn.

```
10 GRAB 0,4,"1111111111111111"
20 PRINT HIT(0,0,0,0,3,3); HIT(0,0,0,0,4,0)
```

shows `10`.

### TILE

```
TILE size, n, x, y [, width, height [, t]]
TILE size, n, a$
```

Defines tiles for tile maps. There are two tile sets, one of 8 × 8 pixel
tiles and one of 16 × 16 (`size` is 8 or 16, otherwise `A Invalid argument`),
each with tiles numbered 0 to 4095.

- The first form copies tile `n` from the drawing page at `x`, `y`. With
  `width` and `height`, it cuts that rectangle into tiles, numbered from `n`
  onwards row by row. Colour `t`, if given, becomes transparent.
- The second form takes the pixels from a string as in [`GRAB`](#grab)
  (size × size characters per tile; a longer string defines the next tiles).

A tile set grows when you define a higher tile; the new tiles start black.

```
10 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,1,0,0
```

### MAP DIM, MAP POKE

```
MAP DIM m, width, height [, size [, wrap]]
MAP POKE m, x, y, value
MAP POKE m, x, y, a$
```

`MAP DIM` creates tile map `m` (0 to 15) with `width` × `height` cells, all
set to tile 0. `size` is the tile size, 8 (the default) or 16. With
`wrap` = 1 the map repeats endlessly in both directions.

`MAP POKE` sets the cell at column `x`, row `y` (counting from 0). The value
is the tile number plus any of these:

| Add | Effect |
|---|---|
| 4096 | tile mirrored left to right |
| 8192 | tile upside down |
| 16384 | colour 8 of the tile is transparent (lets what is behind show) |
| 32768 | priority (see the filter of [MAP](#map)) |

With a string, it fills the cells from `x` to the right, using the code of
each character as the tile number: `"  ##.."` uses tiles 32, 35 and 46. This
makes it easy to draw levels with text.

**See also:** [MAP](#map), [CELL](#cell).

### MAP

```
MAP m [, mx, my [, x, y, width, height [, filter]]]
```

Draws tile map `m`, starting at pixel `mx`, `my` of the map (0, 0 if
omitted), on the whole page or in the rectangle `x`, `y`, `width`, `height`.
Changing `mx` and `my` scrolls the map. `filter`: 0 draws all cells, 1 only
those without priority and 2 only those with priority (so you can draw the
background, then the sprites, then the parts of the scenery in front). All the
tiles the map uses must exist (otherwise `A Invalid argument`).

```
10 BOX 5,0,0,8,8: TILE 8,32,0,0
20 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,35,0,0
30 MAP DIM 0,40,1: MAP POKE 0,0,0,"####  ####   ######  #########   ####"
40 FOR x=0 TO 64: MAP 0,x,0,0,100,256,8: WAIT: NEXT x
```

### CELL

```
CELL(m, x, y)
```

The value of the cell at column `x`, row `y` of map `m`, including the
mirror, transparency and priority bits. Outside a map that does not wrap, 0.
Useful to check what the scenery has at some place: the cell under a sprite
at pixel `px`, `py` is `CELL(m, INT (px/8), INT (py/8))` for 8-pixel tiles.

### TEXTURE

```
TEXTURE n, x, y, size [, c]
```

Defines texture `n` (0 to 3) for [`FLOOR`](#floor): the square of `size` ×
`size` pixels (a power of 2 from 2 to 256: 2, 4, 8... 256; any other size
gives `A Invalid argument`) of the drawing page with its top left corner
at `x`, `y`. The texture repeats for ever; if colour `c` is given, everything
outside it is that colour instead.

### FLOOR

```
FLOOR n, x, y, height, angle, horizon [, focal]
```

Draws a floor in perspective with texture `n`, as seen from point `x`, `y` of
the texture at that `height`, looking in direction `angle` (in radians; 0
looks towards growing `x`). The horizon is at line `horizon` of the page, and
the floor fills from the line below it to the bottom of the page. `focal` is
the focal distance (half the page width if omitted): smaller values give a
wider view. This is the "mode 7" effect of racing and flying games.

```
10 FOR i=0 TO 7: BOX i,i*8,0,8,64: NEXT i: TEXTURE 0,0,0,64
20 FOR a=0 TO 2*PI STEP .05: FLOOR 0,32,32,20,a,80: WAIT: NEXT a
```

### POLY

```
POLY colour, x1, y1 [, x2, y2]...
```

With one point, draws a pixel; with two, a line; with three or more (up to
64), a filled polygon.

```
POLY 4, 128,20, 200,150, 56,150
```

### BOX

```
BOX colour, x, y, width, height
```

A filled rectangle with its top left corner at `x`, `y`.

```
BOX 2, 10,10, 100,50
```

### DISC

```
DISC colour, x, y, radius
```

A filled circle with its centre at `x`, `y`.

```
DISC 6, 128,96, 40
```

### CLIP

```
CLIP x, y, width, height
CLIP
```

From now on, the commands of this section only draw inside that rectangle.
`CLIP` alone removes the limit (as do `MODE`, `RUN` and `CLEAR`).

```
10 CLIP 50,50,100,50: DISC 2,100,75,60: CLIP
```

### PIXEL

```
PIXEL(x, y)
```

The colour (0 to 15) of the pixel at `x`, `y` of the drawing page, counting
from the top left, whatever drew it; -1 outside the page. It ignores
`REMAP`. Useful for collisions with anything drawn on the screen.

```
10 BOX 4,10,10,20,20: PRINT PIXEL(15,15); " "; PIXEL(5,5); " "; PIXEL(-1,0)
```

## Sound and music

ZX-V has a beeper (a simple speaker, used by `BEEP`) and two AY sound chips
with three channels each (used by `PLAY`, `SOUND`, `MUSIC` and `SFX`).

### BEEP

```
BEEP duration, pitch
```

Plays a note on the beeper. `duration` is in seconds (0 to 10); `pitch` is in
semitones above middle C (0 is middle C, 12 the C above, -12 the C below), from
-60 to 69, and can have decimals. The program waits until the note ends.

```
10 FOR n=0 TO 12: BEEP 0.1,n: NEXT n
```

### PLAY

```
PLAY a$ [, b$]...
```

Plays music written as text, with up to six voices at once: the first three
strings play on the three channels of the first AY chip, and the next three
on the second. The program waits until all the strings have finished; BREAK
stops them. A badly written string gives `A Invalid argument`, and a number
out of range `B Integer out of range`; all strings are checked before any
sound starts.

Inside each string:

| Text | Meaning |
|---|---|
| `c d e f g a b` | a note; upper case `C`...`B` is one octave higher |
| `#`, `$` | sharp or flat (before the note): `#f`, `$b` |
| `&` | a rest (silence) |
| `1` to `9` | length of the following notes: 1 semiquaver (sixteenth note), 2 dotted semiquaver, 3 quaver (eighth note), 4 dotted quaver, 5 crotchet (quarter note, the default), 6 dotted crotchet, 7 minim (half note), 8 dotted minim, 9 semibreve (whole note) |
| `10` `11` `12` | triplets of semiquavers, quavers and crotchets: they apply to the next three notes |
| `n_m` | tied lengths: the next note lasts both |
| `O n` | octave, 0 to 8 (default 5, where `c` is middle C) |
| `T n` | tempo in crotchets per minute, 60 to 240 (default 120); only in the first string |
| `V n` | volume, 0 to 15 (default 15) |
| `U` | use the volume envelope |
| `W n` | envelope shape, 0 to 7 |
| `X n` | envelope period, 0 to 65535 |
| `M n` | mixer: bits 0–2 turn on the tone of channels A, B and C, bits 3–5 their noise |
| `N` | separator (does nothing) |
| `(` `)` | repeat what is inside once (up to 4 levels); a `)` without its `(` repeats the whole string for ever (until BREAK) |
| `!...!` | a comment |
| `H` | stop all voices |
| `Y n`, `Z n` | MIDI on the 128K: ignored |

Spaces are ignored. The envelope and mixer belong to each chip, so they are
shared by its three voices.

```
10 PLAY "T140 O4 5cdec 5cdec 5efg&"
20 PLAY "O4 3cegC", "O3 9c", "O5 V10 3(gfed)"
```

### SOUND

```
SOUND register, value [; register, value]...
```

Writes directly to the registers of the AY chips, for sound effects. Registers
0 to 15 belong to the first chip and 16 to 31 to the second (same meaning, 16
higher); values are 0 to 255. A register above 31 gives
`B Integer out of range`.

| Register | Use |
|---|---|
| 0–1, 2–3, 4–5 | tone period of channels A, B and C (low byte, high byte 0–15); the frequency is 110 840 / period Hz |
| 6 | noise period, 0–31 |
| 7 | mixer: a bit at **0** turns on: bits 0–2 tone A, B, C; bits 3–5 noise A, B, C |
| 8, 9, 10 | volume of A, B and C, 0–15; 16 uses the envelope |
| 11–12 | envelope period (low, high) |
| 13 | envelope shape (writing it restarts the envelope) |

```
10 SOUND 7,62; 8,15; 0,200; 1,0
20 PAUSE 25: SOUND 8,0
```

plays a tone on channel A for half a second.

**See also:** [`IN` and `OUT`](#in-out) for the 128K ports.

### MUSIC

```
LOAD name$ MUSIC
MUSIC PLAY
MUSIC STOP
MUSIC PAUSE
MUSIC CONTINUE
MUSIC                          (function)
```

Plays music made with a tracker, in the background: the program carries on
while it plays, as in games. `LOAD name$ MUSIC` loads a module (without an
extension, `name.pt3`): Pro Tracker 3 or Vortex Tracker II (`.pt3`, the one
the ZX-V tracker,
[`zxv-tracker`](../sdk/README.md#the-tracker-for-the-pc-zxv-tracker), saves),
a TurboSound (two modules, one on each AY chip: six voices), Pro Tracker 2,
Sound Tracker, ASC Sound Master or a PSG dump. A file that is none of those
gives `R Tape loading error`.

- `MUSIC PLAY` plays it from the start and, at the end, goes back to the
  module's loop position, for ever. Without a module loaded, `A Invalid
  argument`.
- `MUSIC STOP` stops it; `MUSIC PAUSE` silences it and `MUSIC CONTINUE`
  carries on where it was.
- The `MUSIC` function is 1 while it plays and 0 if not.

It plays on the first AY chip (a TurboSound, on both), with the same code
as the tracker and C programs. It stops with `MUSIC STOP`, BREAK, `RUN`,
`NEW`, `PLAY` (which uses the chips its own way) and when a native program
is loaded; if the program ends or gives an error, it keeps playing. What
`SOUND` writes on the music's chip lasts until the music writes that
register again.

The module takes the size of its file (2 to 15 KB for a normal one) and goes
with the graphics: `RUN` and `CLEAR` keep it and `NEW` deletes it.

```
10 LOAD "tune" MUSIC: MUSIC PLAY
20 PRINT "Playing..."
30 IF INKEY$="" THEN GO TO 30
40 MUSIC STOP
```

### SFX

```
LOAD name$ SFX
SFX n [, channel]
SFX STOP
SFX                            (function)
```

Sound effects as in games: an AYFX effect bank (`.afb`, the one the effects
tab of `zxv-tracker` or the AY Sound FX Editor saves; without an extension,
`name.afb`) or a single effect (`.afx`). A file that is not a bank gives
`R Tape loading error`.

- `SFX n` plays effect `n` of the bank (the first is 0) without waiting, on
  a free channel of the chip the music does not use (if there is none, on
  the one that has been playing longest). With `channel`, on that one: 0 to
  2 are A, B and C of the first chip, and 3 to 5 those of the second; an
  effect on a music channel covers it while it plays. An effect that is not
  in the bank gives `B Integer out of range`, and without a bank, `A Invalid
  argument`.
- `SFX STOP` stops all the effects.
- The `SFX` function is 1 while any effect plays.

They play at the same time as the music, `BEEP` and everything else.
BREAK, `RUN` and `NEW` stop them; the bank is kept like the module.

```
10 LOAD "effects" SFX
20 IF INKEY$=" " THEN SFX 0
30 GO TO 20
```

## Keyboard, joystick and mouse

### INPUT

```
INPUT [prompt items] var [, var]...
INPUT ... LINE a$
```

Stops the program and lets the user type values into variables, in the lower
part of the screen. The items are as in `PRINT`: string literals and
expressions in brackets are shown as a prompt, `AT`, `TAB` and colours work,
and each variable waits for a value.

- For a **number**, the user types a number or a numeric expression (even
  `2*3`), then Enter. If it is not valid, a `?` marks the mistake and the
  user can correct it.
- For a **string**, the input starts with a pair of quotes and the cursor
  inside them, because what the user types is taken as a string expression.
- `LINE a$` takes the text exactly as typed, with no quotes.
- Typing `STOP` (for a number), or pressing BREAK, stops the program with
  `H STOP in INPUT`.

The cursor shows `L`, or `C` when Caps Lock is on.

```
10 INPUT "Your name? "; LINE n$
20 INPUT "Your age? "; a
30 PRINT "Hello, "; n$; ". Next year you will be "; a+1
```

What the user types follows the rules of the editor: accented letters lose
their accent, so `Peña` arrives as `Pena` (see [Character
set](#character-set)).

`INPUT #n` reads from a file (see [Streams](#reading-and-writing-text-files)).

### INKEY$

```
INKEY$
```

The key being pressed at this moment, as a one-character string, or `""` if
none. It does not wait. Letters come out in lower case (upper case with Shift
or Caps Lock), digits as digits, Enter as `CHR$ 13`, and the cursor keys as
`CHR$ 8` (left), `CHR$ 9` (right), `CHR$ 10` (down) and `CHR$ 11` (up). Shift
works as the Spectrum's CAPS SHIFT and Ctrl as its SYMBOL SHIFT. A joystick
set up as keys with [`JOY`](#joy) also counts.

Whatever is pressed while a program reads the keyboard this way (with
`INKEY$`, `IN` or [`KEY`](#key)) is not kept for later: when the program ends
it does not appear typed in the editor, and an `INPUT` starts empty.

```
10 LET k$=INKEY$: IF k$="" THEN GO TO 10
20 PRINT "You pressed "; k$: IF k$<>"q" THEN GO TO 10
```

**See also:** [KEY](#key), which is better for games, and [PAUSE](#pause).

### KEY

```
KEY n
```

1 if key `n` is held down right now, 0 if not. Several keys can be held at
once, so this is the way to read the keyboard in games. `n` is:

| `n` | Key |
|---|---|
| `CODE "a"` to `CODE "z"` (or upper case) | a letter |
| `CODE "0"` to `CODE "9"` | a digit |
| 8, 9, 10, 11 | the cursor keys left, right, down, up |
| 12 | Backspace |
| 13 | Enter |
| 32 | Space |
| 256 + code | any other key, by its USB key code: 298 Backspace, 299 Tab, 314 to 325 F1 to F12, 480 left Ctrl, 481 left Shift, 482 left Alt |

Other values give `A Invalid argument`.

```
10 LET x=128
20 LET x=x+KEY 9-KEY 8: PLOT x,88: WAIT: GO TO 20
```

### JOY

```
JOY n
JOY n, mode
```

The function `JOY n` gives the state of joystick or gamepad `n` (0 for the
first one connected, 1 for the second) as a number made of these bits; 0 if
it is not connected:

| Bit | Value | Button |
|---|---|---|
| 0 | 1 | right |
| 1 | 2 | left |
| 2 | 4 | down |
| 3 | 8 | up |
| 4 | 16 | fire 1 |
| 5 | 32 | fire 2 |
| 6 | 64 | fire 3 |
| 7 | 128 | start |
| 8 | 256 | select |

To test a button, divide and look at the remainder, for example
`INT (JOY 0/16)-2*INT (JOY 0/32)` is 1 while fire 1 is held.

The statement `JOY n, mode` makes joystick `n` also act as keys, for programs
that read the keyboard with `INKEY$` or `IN` (any fire button counts as fire):

| Mode | Left | Right | Down | Up | Fire |
|---|---|---|---|---|---|
| 0 | (not as keys) | | | | |
| 1 Sinclair 1 | 6 | 7 | 8 | 9 | 0 |
| 2 Sinclair 2 | 1 | 2 | 3 | 4 | 5 |
| 3 Cursor | 5 | 8 | 6 | 7 | 0 |

At start, joystick 0 is Sinclair 1 and joystick 1 is Sinclair 2. `KEY` does
not see these keys. A joystick other than 0 or 1, or a mode other than 0 to
3, gives `B Integer out of range`, as in the function.

```
10 PRINT AT 0,0; JOY 0; "   ": GO TO 10
```

### MOUSE

```
MOUSE n
MOUSE x, y
```

The function `MOUSE n` reads the mouse:

| `n` | Value |
|---|---|
| 0 | x of the pointer, in pixels from the left of the screen |
| 1 | y of the pointer, in pixels from the top |
| 2 | buttons: 1 left, 2 right, 4 middle (added together) |
| 3 | wheel: grows when turned up, shrinks when turned down |

The pointer starts at the centre of the screen and stays inside it (0–255 and
0–191 in mode 0). The statement `MOUSE x, y` moves the pointer. ZX-V does not
draw the pointer: your program draws it, for example with a sprite.

```
10 GRAB 0,3,"ff.f.f..f"
20 CLS: SPRITE 0,MOUSE 0,MOUSE 1: IF MOUSE 2 THEN PRINT "click"
30 WAIT: GO TO 20
```

## Files and the SD card

Programs and data are kept on the SD card. There is also a read-only drive,
`ROM:`, with the examples that come with the machine.

### File names and drives

- A name has up to 63 characters, without `\ : * ? " < > |`, and does not end
  in a space or a dot. Upper and lower case are the same.
- Folders are separated with `/`. A path starting with `/` begins at the root
  of the drive; otherwise it starts in the current folder (see [CD](#cd-dir)).
  `..` is the folder above.
- `ROM:` and `SD:` at the start choose a drive, from its root:
  `LOAD "ROM:hello"`, `CAT "SD:/games"`. Without a drive, the drive of the
  current folder is used. Writing to `ROM:` (`SAVE`, `ERASE`, `MKDIR`...)
  gives `V Disk error (not allowed)`.
- **Wildcards** in `CAT`, `ERASE`, `MOVE` and `COPY`: `*` stands for any
  characters (also none) and `?` for one character, in the last part of the
  path only: `"*.tap"`, `"games/level?.dat"`. If nothing matches,
  `T File not found`.

Common file errors are `F Invalid file name`, `T File not found`, `U No SD
card` and `V Disk error`, which adds the reason when it is known:
`(already exists)`, `(folder not empty)`, `(it is a folder)`,
`(not a folder)`, `(card full)` or `(not allowed)`.

### SAVE

```
SAVE name$ [LINE line]
SAVE name$ CODE start, length
SAVE name$ SCREEN$
SAVE name$ DATA letter()
SAVE name$ DATA letter$()
SAVE name$ GRAPHICS
```

Saves to the SD card:

- The **program** (with no other words): as a `.tap` file, the tape format of
  the Spectrum, so it can be opened in Spectrum emulators. With `LINE`, the
  program starts by itself from that line when loaded.
- `CODE`: `length` bytes of memory from address `start`.
- `SCREEN$`: the screen, as a Spectrum screen (`CODE 16384,6912`).
- `DATA`: an array (numbers or characters), with its name.
- `GRAPHICS`: the graphics store (sprites, tiles, maps, textures) as a
  `.gfx` file.

If the name has no extension, `.tap` is added (`.gfx` for `GRAPHICS`). With
another extension, `CODE` and `SCREEN$` are saved as plain bytes, without the
tape header; `DATA` is always saved as `.tap`. An existing file is replaced.

With the extension `.bas`, the program is saved as a **listing in plain text**
(see [Listings in text](#listings-in-text)): `SAVE "game.bas"`.

```
10 PRINT "I was saved"
SAVE "hello" LINE 10
```

### LOAD

```
LOAD name$
LOAD name$ CODE [start [, length]]
LOAD name$ SCREEN$
LOAD name$ DATA letter()
LOAD name$ DATA letter$()
LOAD name$ GRAPHICS
LOAD name$ MUSIC
LOAD name$ SFX
```

Loads from the SD card (or the `ROM:` drive):

- A **program**: replaces the program in memory (variables are not kept). If
  it was saved with `LINE`, it runs. Without an extension, ZX-V tries
  `name.tap`, then `name.zxv` (a [native program](#native-programs), which
  runs), then `name.bas` (a [listing in text](#listings-in-text)), then the
  name as it is. Spectrum `.tap` and `.tzx` files with BASIC programs load
  directly.
- `CODE`: a block of bytes, at the address it was saved from or at `start`,
  and at most `length` bytes. A file that is not a `.tap` is read as plain
  bytes, and then `start` is needed.
- `SCREEN$`: a Spectrum screen (a `.scr` file or a block of a `.tap`).
- `DATA`: an array, replacing the array of that name. The type must match.
- `GRAPHICS`: the objects of a `.gfx` file replace those with the same number;
  the others stay, so you can load sprites from one file and a level from
  another. A file that is not valid gives `R Tape loading error`.
- `MUSIC` and `SFX`: a music module or an effect bank (see
  [`MUSIC`](#music) and [`SFX`](#sfx)).

`LOAD ""` (an empty name) loads the next block from the inserted tape (see
[Tape files](#tape-files)).

```
LOAD "hello"
LOAD "ROM:graphics"
```

### VERIFY

```
VERIFY name$ [CODE ... | SCREEN$ | DATA ... | GRAPHICS]
```

Checks that a file is the same as what is in memory, with the same forms as
`LOAD`, without changing anything. If it differs, `R Tape loading error`.

### MERGE

```
MERGE name$
```

Adds the lines of a saved program to the one in memory; lines with the same
number are replaced. The variables are deleted. It also works with a
[listing in text](#listings-in-text) (`.bas`).

### Listings in text

A program can also live in a plain text file with the extension `.bas`, one
line of the program per line, each with its number, as `LIST` shows it. You
can write it in any text editor on your computer, copy it to the card and
load it with `LOAD "game.bas"` (or `MERGE`); `SAVE "game.bas"` writes the one
in memory.

```
10 REM Written outside ZX-V
20 for i = 1 to 10: print i: next i
```

- Each line is read as if you typed it in the editor: keywords in capitals or
  not, and the same syntax check. A line with an error stops the loading with
  its report and its line number (`C Nonsense in BASIC, 20:1`); the lines
  before it stay in memory.
- A line with only a number deletes that line, and blank lines are ignored. A
  line without a number gives an error at the line before it: a `.bas` file
  is a program, not a list of commands (leave `RUN` out).
- The text is UTF-8. Accented letters lose the accent, as when you type them
  (`año` becomes `ano`); `£` and `©` are those of the Spectrum, and the block
  graphics are written with the Unicode quadrant characters (`▘ ▝ ▀ ▖ ▌ ▞ ▛ ▗ ▚ ▐ ▜ ▄ ▙ ▟ █`).
  UDG characters are saved as single bytes, which a text editor may show
  oddly but which load back correctly.
- Lines can end in LF (Linux, macOS) or CR LF (Windows); a line can be up to
  1024 characters long.

### CAT

```
CAT [name$]
```

Lists a folder: first the folder name, then each file with its size in bytes,
and each folder with `/` and `<DIR>`. Without a name, the current folder. With
wildcards or a file name, only what matches.

```
CAT "ROM:"
CAT "*.tap"
```

### CD, DIR$

```
CD folder$
DIR$
```

`CD` changes the current folder: `CD "games"`, `CD ".."` (the folder above),
`CD "/"` (the root of the drive), `CD "ROM:"` (the built-in examples drive)
and `CD "SD:"` (back to the card). The function `DIR$` gives the current
folder, such as `"/games"` or `"ROM:/"`.

```
CD "ROM:": PRINT DIR$: CAT
```

A program that loads more files (like the platform game) looks for them in
the current folder, so change folder first: `CD "ROM:": LOAD "platformer"`.

### MKDIR, RMDIR

```
MKDIR folder$
RMDIR folder$
```

`MKDIR` creates a folder; `RMDIR` deletes an empty folder (otherwise
`V Disk error (folder not empty)`).

### ERASE

```
ERASE name$
```

Deletes a file, or with wildcards every file that matches (`ERASE "*.bak"`).
Folders are not deleted (use `RMDIR`). There is no undo.

### MOVE

```
MOVE from$ TO to$
```

Renames a file or folder, or moves it to another folder: if `to$` is a
folder, it goes inside it with the same name. With wildcards, `to$` must be a
folder and every file that matches is moved.

```
MOVE "game.tap" TO "old game.tap"
MOVE "*.tap" TO "games"
```

### COPY

```
COPY from$ TO to$
COPY
```

With two names, copies a file (replacing `to$` if it exists; if `to$` is a
folder, the copy goes inside). With wildcards, `to$` must be a folder. Copying
a file onto itself gives `V Disk error (already exists)`. To copy an example
to the card: `COPY "ROM:hello.zxv" TO "hello.zxv"`.

`COPY` alone sends the text on the screen to the printer (the file
`printer.txt`, see [LPRINT](#lprint-llist)).

### Native programs

Programs written in C with the [SDK](../sdk/README.md) are `.zxv` files.
`LOAD "name"` runs them (the extension is optional). A native program uses the
memory of BASIC: **when it ends, the BASIC program and its variables are
gone**, so save your BASIC program first.

```
LOAD "ROM:hello"
```

When the program ends, BASIC shows a report, as after a command:

- `0 OK, 0:1` if it ended normally.
- `W Native code error (3), 0:1` if it ended with an error code (3 in this
  example), or `W` followed by the program's own message if it gave one:
  `W Data file damaged (5), 0:1`.
- `W Native code error, 0:1` if it crashed (for example, by reading from a
  forbidden address).
- `L BREAK into program, 0:1` if you stopped it with Ctrl+Alt+Delete.

The report is the program's: it says nothing about your BASIC program, which
is gone in every case.

## Tape files

ZX-V reads the tape files of Spectrum emulators, `.tap` and `.tzx` (the
standard-speed blocks: programs, `CODE`, screens and arrays). Spectrum machine
code inside them cannot run.

A tape file that has been loaded stays **inserted**, like a cassette in a
tape recorder, so programs in several parts work:

- `LOAD ""` (also `LOAD "" SCREEN$`, `LOAD "" CODE`, `LOAD "" DATA a()`,
  `VERIFY ""`, `MERGE ""`) reads the next block of that kind from where the
  tape stopped. When it reaches the end it rewinds once. With no tape
  inserted, or no block of that kind, `R Tape loading error`.
- `LOAD "name"` uses the file `name` if it exists on the card (which becomes
  the inserted tape); if not, it looks for a block with that name on the
  inserted tape, and if there is none, `T File not found`.
- The tape stays inserted after `NEW` and `CD`, until another tape file is
  loaded or the machine is reset.

So `LOAD "game"` loads the loader from `game.tap`, which can then bring its
title screen and the main program with `LOAD "" SCREEN$: LOAD ""`.

## Streams and channels

Text goes in and out through numbered **streams**, 0 to 15. Streams 0 to 3
are ready from the start:

| Stream | Goes to |
|---|---|
| 0, 1 | the lower part of the screen (channel `"k"`) |
| 2 | the upper part of the screen (channel `"s"`), used by `PRINT` |
| 3 | the printer, the file `printer.txt` (channel `"p"`), used by `LPRINT` |

`PRINT`, `LIST` and `INPUT` take a stream with `#`: `PRINT #1; "bottom"`
prints in the lower part, `LIST #3` lists to the printer. Streams 4 to 15 can
be connected to text files on the SD card. A stream that is not open gives
`O Invalid stream`.

### OPEN, CLOSE

```
OPEN #stream, name$ IN
OPEN #stream, name$ OUT
OPEN #stream, channel$
CLOSE #stream
```

`OPEN # ... OUT` creates a text file (emptying it if it exists) to write to;
`OPEN # ... IN` opens one to read. Files can only use streams 4 to 15, and up
to 8 files can be open at once (otherwise `O Invalid stream`). Opening a
stream that already has a file closes it first.

`OPEN #stream, "k"`, `"s"` or `"p"` connects a stream to the lower screen,
the upper screen or the printer; another channel gives `J Invalid I/O
device`.

`CLOSE #stream` closes the file (writing whatever is pending) and frees the
stream; streams 0 to 3 go back to their original channels. `RUN`, `CLEAR` and
`NEW` close all files.

### Reading and writing text files

```
PRINT #stream; items
LIST #stream
INPUT #stream; var [, var]... [LINE a$]
INKEY$ #stream
```

- `PRINT #n` writes like `PRINT`, without colours or `AT`; `TAB` writes spaces.
  In a file, **a comma starts a new line**, so that what `PRINT #n; a, b$`
  writes can be read back with `INPUT #n; a, b$`.
- `LIST #n` writes the program as text.
- `INPUT #n` reads one line for each variable (lines can end in LF, CR or
  CR LF). A string variable takes the whole line; a number is worked out as an
  expression (if it is not valid, `C Nonsense in BASIC`). At the end of the
  file, `8 End of file`.
- `INKEY$ #n` gives the next character of the file, or `""` at the end.

Writing to a file opened with `IN`, or reading from one opened with `OUT`,
gives `J Invalid I/O device`.

```
10 OPEN #4, "scores.txt" OUT: PRINT #4; "Ann", 120: CLOSE #4
20 OPEN #4, "scores.txt" IN: INPUT #4; n$, p: CLOSE #4
30 PRINT n$; " scored "; p
```

To read a file whose length you do not know, catch the end of file with
[`ON ERROR`](#on-error):

```
10 ON ERROR GO TO 50: OPEN #5, "scores.txt" IN
20 INPUT #5; LINE l$: PRINT l$: GO TO 20
50 CLOSE #5: IF ERR<>8 THEN PRINT "Error "; ERR
```

### FORMAT

`FORMAT` exists only for compatibility with old Spectrum programs and gives
`O Invalid stream`.

## Memory and system

### PEEK, POKE

```
PEEK address
POKE address, value
```

`PEEK` reads a byte (0 to 255) from memory and `POKE` writes one (a negative
value from -255 to -1 is written as 256 plus that value).

- Addresses **below 65536** go to a **virtual Spectrum memory**, so that
  Spectrum programs that `PEEK` and `POKE` the screen, the colours or the
  system variables still work (see the table below).
- Addresses **from 65536 up** read and write the real memory of ZX-V, byte by
  byte (for example the [machine registers](../machine.md)). `POKE` only
  writes in RAM; addresses outside memory, and the protected system area,
  give `B Integer out of range`.

```
PRINT PEEK 536872704
```

prints the low byte of the ZX-V frame counter (60 per second; the
Spectrum's, `FRAMES`, is at 23672).

### The virtual Spectrum memory

| Addresses | Contents |
|---|---|
| 0–16383 | Spectrum ROM: reads as 0, except the character set at 15616 |
| 16384–22527 | the screen pixels, laid out as on the Spectrum (`POKE` draws, using the colours of each cell) |
| 22528–23295 | the colours of each cell (`POKE` changes them) |
| 23552–23733 | system variables (see below) |
| 23296–65535 | normal memory: what you `POKE` you can `PEEK` |
| 65368–65535 | the user-defined graphics A to U |

Only the first 256 × 192 pixels of the screen are visible at these addresses.

System variables that work: `LAST K` (23560, the last key), `CHARS`
(23606, where the character set is; you can point it to your own font in
virtual memory), `BORDCR` (23624), `PROG` (23635), `FLAGS2` (23658, bit 3 is
Caps Lock), `DF SZ` (23659), `SEED` (23670, the `RND` seed), `FRAMES`
(23672, three bytes counting fiftieths of a second, like the Spectrum's
frames), `UDG` (23675), `COORDS` (23677, the
last point of `PLOT`), `S POSN` (23688), `SCR CT` (23692), `ATTR P`,
`MASK P`, `ATTR T`, `MASK T` and `P FLAG` (23693 to 23697) and `RAMTOP`
(23730).

### User-defined graphics (UDG)

The character codes 144 to 164 are 21 characters, A to U, that you can draw
yourself. `USR "a"` gives the address of the first byte of character A (and
so on to `USR "u"`); each character is 8 bytes, one per row from the top, and
each bit is a pixel (1 = ink). At start they look like the letters A to U.

```
10 FOR i=0 TO 7: READ b: POKE USR "a"+i, b: NEXT i
20 PRINT CHR$ 144; CHR$ 144; CHR$ 144
30 DATA BIN 00111100, BIN 01111110, BIN 11011011, BIN 11111111
40 DATA BIN 11111111, BIN 10111101, BIN 11000011, BIN 01111110
```

The editor cannot type these characters: use `CHR$`.

### USR

```
USR address
USR "letter"
```

With an address of 65536 or more, calls native RISC-V machine code at that
address and gives back the value it returns (in register `a0`). If the code
fails, `W Native code error`. Spectrum Z80 machine code cannot run: an
address below 65536 gives `S Z80 code not supported`. To call code without
using the result: `RANDOMIZE USR address`.

`USR "a"` to `USR "u"` give the address of a [user-defined
graphic](#user-defined-graphics-udg); another letter gives
`A Invalid argument`.

### IN, OUT

```
IN port
OUT port, value
```

Read and write the ports of the Spectrum, as emulated by ZX-V:

| Port | `IN` | `OUT` |
|---|---|---|
| 254 (and any even port) | the keyboard half-rows selected by the high byte, as on the Spectrum (bits at 0 = pressed) | bits 0–2 border colour, bit 4 beeper |
| 31 | Kempston joystick (joystick 0) | |
| 64479, 65503 | Kempston mouse: x and y counters | |
| 64223 | Kempston mouse buttons (0 = pressed) | |
| 65533 | read the selected AY register | select an AY register; 255 selects the first chip and 254 the second |
| 49149 | | write the selected AY register |

Other ports read 255 and ignore writes. Programs written for ZX-V are better
off with `KEY`, `JOY`, `MOUSE`, `BORDER` and `SOUND`.

### TIME

```
TIME
```

The number of frames since the machine was switched on, 60 per second. To
time something, subtract two readings:

```
10 LET t=TIME: PAUSE 100: PRINT (TIME-t)/60; " seconds"
```

### CLOCK

```
CLOCK n
CLOCK year, month, day, hour, minutes [, seconds]
```

The function `CLOCK n` reads the date and time: 0 year, 1 month, 2 day, 3
hour, 4 minutes, 5 seconds, 6 day of the week (1 Monday to 7 Sunday). It
gives 0 if the clock has not been set. The statement sets the clock (years
2000 to 2099); a date that does not exist gives `A Invalid argument`. Files
saved on the card are stamped with this date.

```
PRINT CLOCK 2; "/"; CLOCK 1; "/"; CLOCK 0; " "; CLOCK 3; ":"; CLOCK 4
```

### SPEED

```
SPEED n
```

`SPEED 1` (the default) runs BASIC at full speed, about 200 times faster than
a Spectrum. `SPEED 0` makes each statement take about as long as on a 48K
Spectrum, for old programs that use loops as delays or would be unplayable at
full speed. Other values give `A Invalid argument`. The new commands
(sprites, maps...) are not slowed down.

### LANGUAGE

```
LANGUAGE code$
```

Sets the language of the machine with a two-letter code: `"en"` English or
`"es"` Spanish. Error reports and the editor's bar and menu use it from then
on, and the machine remembers it (on the real hardware, in its battery-backed
clock; in the emulator, in its settings). Any other two-letter
code is accepted and shows English (a program can read it to choose its own
texts: `PEEK 536872736` and `PEEK 536872737` are the codes of the two
letters). Something that is not two letters gives `A Invalid argument`.

```
LANGUAGE "es"
```

### MONITOR

```
MONITOR
```

Opens the test monitor, a screen for checking the hardware (keyboard,
joystick, video modes, sound). To leave it, reset the machine
(Ctrl+Alt+Del).

## Editing commands

These commands are mostly typed as direct commands while you write a program.
See [the editor](editor.md) for the full picture.

### LIST

```
LIST [line]
LIST #stream [, line]
```

As a direct command, `LIST` takes you to the editor with the cursor on that
line (or the next one that exists), ready to change it. Inside a program, or
with a stream, it prints the listing from that line: `LIST #2` on the program
screen, `LIST #3` to the printer, `LIST #4` to a file. The listing is
indented: whatever is inside a loop, a procedure or a block shows two spaces
further right for each level (the spaces are not stored).

**See also:** [LLIST](#lprint-llist), [EDIT](#edit).

### EDIT

```
EDIT [line]
```

Puts the editor's cursor on that line (without a number, on the current
line). Not to be confused with the EDIT key (Tab), which opens the editor's
options menu.

### AUTO

```
AUTO [first [, step]]
```

Automatic line numbers: each new line in the editor starts with the next
number. Without values, it starts at the last line plus 10, and goes up in
tens. It ends when you press Enter on a line with only the number, press Esc,
or move to another line. Only as a direct command (in a program,
`C Nonsense in BASIC`).

```
AUTO 100, 5
```

### RENUM

```
RENUM [first [, step]]
```

Renumbers the program (from 10 in tens by default), and corrects the line
numbers after `GO TO`, `GO SUB`, `RESTORE`, `RUN`, `LIST`, `LLIST` and
`LINE`. Only numbers written directly are corrected: in `GO TO n*10` the
expression is not changed. Labels do not need correcting. If the numbers would
go past 9999, `B Integer out of range`. Only as a direct command. The
editor's options menu has the same thing.

```
RENUM 100, 10
```

### DELETE

```
DELETE first [, last]
```

Deletes the lines from `first` to `last` (or just line `first`). Only as a
direct command.

```
DELETE 100, 190
```

### FIND

```
FIND [text]
```

Moves the editor's cursor to the next program line where `text` appears,
starting with the line after the cursor and going round at the end, with the
cursor on the text. Upper and lower case are the same, and it searches the
listing as you see it (not the line numbers). Without text, it searches for
the same as last time, like the F3 key. If it is not there, the status line
says `Not found`. Only as a direct command.

```
FIND "score"
```

**See also:** [REPLACE](#replace).

### REPLACE

```
REPLACE old, new
```

Changes the text `old` to `new` in the whole program, with upper and lower
case the same. Each changed line is checked as if it were typed, and lines
that would have a mistake are left alone. The status line says how many
changes were made and how many lines were left unchanged. With an empty
`old`, `A Invalid argument`. Only as a direct command.

```
REPLACE "lives", "energy"
```

**See also:** [FIND](#find).

### NEW

```
NEW
```

Deletes the program, the variables and the graphics store, and starts afresh
(the inserted tape stays). There is no undo: save first.

## Error handling

### Reports

When a program stops, a report appears at the bottom of the program screen:

```
2 Variable not found, 20:1
```

It gives a code (a digit or a letter), the message, and where it happened:
the line number and the statement within the line (`20:1` is the first
statement of line 20; line 0 means a direct command). `0 OK` means the
program ended normally. Press a key to go back to the editor.

The reports appear in the language of the machine (see
[LANGUAGE](#language)).

### ON ERROR

```
ON ERROR GO TO line | @label
ON ERROR GO TO 0
```

From now on, if an error stops the program, it jumps to that line or label
instead. There, `ERR` and `ERL` say what happened. `ON ERROR GO TO 0` turns it
off, as do `RUN` and `CLEAR`.

- The trap turns itself off when it is used, so that an error inside the
  handler does not loop for ever: write `ON ERROR` again to keep catching
  errors.
- `CONTINUE` repeats the statement that failed (after BREAK, it goes on with
  the next one).
- It catches all errors, including BREAK, except `STOP` and errors in direct
  commands.

```
10 ON ERROR GO TO @oops
20 PRINT 1/0
30 PRINT "carrying on"
40 STOP
50 @oops: PRINT "Error "; ERR; " at line "; ERL
60 ON ERROR GO TO @oops: GO TO 30
```

### ERR, ERL

```
ERR
ERL
```

After an error caught by `ON ERROR`, `ERR` is the error code as a number (the
digits are themselves, and the letters count on from 10: `A` is 10, `B` 11...
`Z` 35; the table below gives them all) and `ERL` is the line where it
happened.

## Error messages

The reports in English and Spanish, with the number that `ERR` gives:

| Code | `ERR` | English | Spanish | What usually causes it |
|---|---|---|---|---|
| 0 | 0 | OK | OK | Not an error: the program or command ended normally. |
| 1 | 1 | NEXT without FOR | NEXT sin FOR | `NEXT` with a variable that is not the control variable of a `FOR`. |
| 2 | 2 | Variable not found | Variable no encontrada | Using a variable before giving it a value, or an array before its `DIM`. |
| 3 | 3 | Subscript wrong | Subíndice incorrecto | An array subscript out of its size, the wrong number of subscripts, or a slice outside a string. |
| 4 | 4 | Out of memory | Sin memoria | Not enough memory, too many nested `GO SUB` (512) or loops and procedures (64), or an expression nested too deep. |
| 5 | 5 | Out of screen | Fuera de la pantalla | `PRINT AT` or `INPUT` outside the screen. |
| 6 | 6 | Number too big | Número demasiado grande | A result above 1.7E38, often a division by zero. |
| 7 | 7 | RETURN without GOSUB | RETURN sin GO SUB | `RETURN` with no `GO SUB` to go back to. |
| 8 | 8 | End of file | Fin del fichero | `INPUT #` after the last line of a file. |
| 9 | 9 | STOP statement | Sentencia STOP | The program reached `STOP`. `CONTINUE` carries on. |
| A | 10 | Invalid argument | Argumento no válido | A value a function cannot take (`SQR -1`, `LN 0`), a sprite or map that does not exist, a bad `PLAY` string, an impossible date. |
| B | 11 | Integer out of range | Entero fuera de rango | A whole number out of its range: a coordinate off the screen, `CHR$ 300`, `POKE` with 256... |
| C | 12 | Nonsense in BASIC | Sintaxis incorrecta | The line is not valid BASIC (a typing mistake, a missing bracket), or a value of the wrong type. |
| D | 13 | BREAK - CONT repeats | BREAK - CONT repite | You answered N, Space or BREAK to `scroll?`. `CONTINUE` repeats. |
| E | 14 | Out of DATA | No quedan DATA | `READ` with no `DATA` left. |
| F | 15 | Invalid file name | Nombre de fichero no válido | An empty name, a name too long, or with characters not allowed. |
| G | 16 | No room for line | No cabe la línea | The line is too long, or memory is full. |
| H | 17 | STOP in INPUT | STOP en INPUT | `STOP` typed, or BREAK pressed, in an `INPUT`. |
| I | 18 | FOR without NEXT | FOR sin NEXT | A `FOR` loop that runs no times and has no `NEXT` to jump to. |
| J | 19 | Invalid I/O device | Dispositivo no válido | Writing to a file opened for reading (or the reverse), or an unknown channel in `OPEN #`. |
| K | 20 | Invalid colour | Color no válido | A colour out of range (`INK 10`, `BORDER 8`, `BOX 16,...`). |
| L | 21 | BREAK into program | BREAK en el programa | You pressed BREAK (Esc). `CONTINUE` carries on with the next statement. |
| M | 22 | RAMTOP no good | RAMTOP no válido | A `CLEAR` address that is too low or too high. |
| N | 23 | Statement lost | Sentencia perdida | A jump back to a statement that no longer exists (for example after changing the line). |
| O | 24 | Invalid stream | Canal no válido | A stream not open, a file on streams 0–3, more than 8 open files, or `FORMAT`. |
| P | 25 | FN without DEF | FN sin DEF FN | `FN` of a function with no `DEF FN` in the program. |
| Q | 26 | Parameter error | Error en los parámetros | The wrong number or type of values for `FN` or `PROC`, or too many. |
| R | 27 | Tape loading error | Error al cargar | A file that is not valid or not the right kind, `LOAD ""` with no tape inserted, or `VERIFY` found differences. |
| S | 28 | Z80 code not supported | Código Z80 no admitido | `USR` with an address below 65536: Spectrum machine code. |
| T | 29 | File not found | Fichero no encontrado | No file or folder with that name, or nothing matches the wildcards. |
| U | 30 | No SD card | No hay tarjeta SD | There is no card in the slot. |
| V | 31 | Disk error | Error de disco | The card failed or the operation is not possible; the reason follows in brackets. |
| W | 32 | Native code error | Error en el código nativo | Native code called with `USR` crashed, or a native program (`.zxv`) ended with an error. The program's code or its own message may appear instead (see [Native programs](#native-programs)). |
| X | 33 | Label not found | Etiqueta no encontrada | `GO TO @name` (or similar) with no `@name` in the program. |
| Y | 34 | Structure error | Error de estructura | `WEND`, `UNTIL`, `END PROC` or `LOCAL` without their opening, or a block (`WHILE`, `IF`, `SELECT CASE`...) without its end. |
| Z | 35 | PROC not found | PROC no encontrado | `PROC` of a procedure with no `DEF PROC`. |

The reasons of `V Disk error`: `already exists` / `ya existe`,
`folder not empty` / `la carpeta no está vacía`, `it is a folder` /
`es una carpeta`, `not a folder` / `no es una carpeta`, `card full` /
`la tarjeta está llena`, `not allowed` / `no se puede`.

## Character set

Each character has a code from 0 to 255 (`CODE` and `CHR$` convert between
them):

| Codes | Characters |
|---|---|
| 0–31 | control codes (see below); others print as `?` |
| 32–126 | letters, digits and symbols, as in ASCII, except 96, which is `£` |
| 127 | `©` |
| 128–143 | block graphics: the 16 combinations of the four quarters of a character cell |
| 144–164 | the [user-defined graphics](#user-defined-graphics-udg) A to U |
| 165–255 | the keywords: `PRINT CHR$ 245` prints `PRINT` |

Control codes that have an effect when printed:

| Code | Effect |
|---|---|
| 6 | as a comma in `PRINT` |
| 8 | back one column |
| 13 | new line |
| 16 to 21 | `INK`, `PAPER`, `FLASH`, `BRIGHT`, `INVERSE`, `OVER`: the next character is the value |
| 22 | `AT`: the next two characters are the row and the column |
| 23 | `TAB`: the next two characters are the column (low and high byte) |

For example, `PRINT CHR$ 22; CHR$ 5; CHR$ 10; "X"` is the same as
`PRINT AT 5,10; "X"`.

**Block graphics.** Codes 128 to 143 divide the character cell into four
quarters. To find the code of a pattern, add to 128 the values of the
quarters filled with ink: 1 top right, 2 top left, 4 bottom right, 8 bottom
left. For example, 128 + 2 + 8 = 138 is the left half.

| Code | Character | Code | Character | Code | Character | Code | Character |
|---|---|---|---|---|---|---|---|
| 128 | (empty) | 132 | ▗ | 136 | ▖ | 140 | ▄ |
| 129 | ▝ | 133 | ▐ | 137 | ▞ | 141 | ▟ |
| 130 | ▘ | 134 | ▚ | 138 | ▌ | 142 | ▙ |
| 131 | ▀ | 135 | ▜ | 139 | ▛ | 143 | █ |

The quarters are in ink and the rest of the cell in paper, as with any other
character. The codes are the same as on the Spectrum.

In the editor you can type the codes 32 to 127 (`£` and `©` too, if your
keyboard has them). Block graphics and user-defined graphics are written with
`CHR$`. Other characters, such as accented letters, are not part of the set,
so BASIC strings cannot hold them. When you type them, in the editor or in an
`INPUT`, a letter with an accent, a diaeresis, a tilde or a cedilla comes out
without it (`á` as `a`, `Ñ` as `N`, `ç` as `c`), `¿` and `¡` come out as `?`
and `!`, and any other character does nothing. The ROM can show the Spanish
letters (`á`, `ñ`, `¿`...) in its own messages, such as the error reports in
Spanish, but your programs cannot.

## Limits

| What | Limit |
|---|---|
| Line numbers | 1 to 9999 |
| Length of a line in the editor | 1024 characters |
| Numbers | about ±2.9E-39 to ±1.7E38, 8 digits shown |
| Numeric variable names | up to 63 letters and digits |
| String variables, arrays, `FOR` variables, `DEF FN` | one letter (26 of each) |
| Strings | as long as memory allows |
| Array dimensions | up to 16, each up to 65535 |
| Nested `GO SUB` | 512 |
| Nested `WHILE`, `REPEAT` and `PROC` | 64 |
| Nested block IFs and `SELECT CASE` | no limit |
| Parameters of a procedure | 16 |
| Nested brackets in an expression | about 150 (about 25 nested `FN`) |
| Memory for program and variables | about 96 KB (shown in the editor's bar) |
| Open files | 8, on streams 4 to 15 |
| Sprites | 256 (0–255), each up to 1024 × 1024 pixels |
| Tiles | 4096 of 8 × 8 and 4096 of 16 × 16 |
| Tile maps | 16 (0–15), up to 32767 × 32767 cells |
| Textures | 4 (0–3) |
| Polygon corners | 64 |
| `PLAY` | 8 strings (6 play), 4 levels of brackets |
| File names | 63 characters; paths 255 |

## Keyword index

| Keyword | Kind | Entry |
|---|---|---|
| `@` | statement | [@label](#label) |
| `ABS` | function | [ABS, SGN, INT](#abs-sgn-int) |
| `ACS` | function | [ASN, ACS, ATN](#asn-acs-atn) |
| `AND` | operator | [True and false](#true-and-false) |
| `ASN` | function | [ASN, ACS, ATN](#asn-acs-atn) |
| `AT` | in `PRINT`, `INPUT` | [PRINT](#print) |
| `ATN` | function | [ASN, ACS, ATN](#asn-acs-atn) |
| `ATTR` | function | [SCREEN$, ATTR](#screen-attr) |
| `AUTO` | statement | [AUTO](#auto) |
| `BEEP` | statement | [BEEP](#beep) |
| `BIN` | number | [BIN](#bin) |
| `BORDER` | statement | [BORDER](#border) |
| `BOX` | statement | [BOX](#box) |
| `BRIGHT` | statement, item | [BRIGHT, FLASH](#bright-flash) |
| `CAT` | statement | [CAT](#cat) |
| `CASE` | in `SELECT CASE` | [SELECT CASE](#select-case) |
| `CD` | statement | [CD, DIR$](#cd-dir) |
| `CELL` | function | [CELL](#cell) |
| `CHR$` | function | [CHR$, CODE](#chr-code) |
| `CIRCLE` | statement | [CIRCLE](#circle) |
| `CLEAR` | statement | [CLEAR](#clear) |
| `CLIP` | statement | [CLIP](#clip) |
| `CLOCK` | function, statement | [CLOCK](#clock) |
| `CLOSE #` | statement | [OPEN, CLOSE](#open-close) |
| `CLS` | statement | [CLS](#cls) |
| `CODE` | function; in `SAVE`, `LOAD` | [CHR$, CODE](#chr-code), [SAVE](#save) |
| `CONTINUE` | statement | [CONTINUE](#continue) |
| `COPY` | statement | [COPY](#copy) |
| `COS` | function | [SIN, COS, TAN](#sin-cos-tan) |
| `DATA` | statement; in `SAVE`, `LOAD` | [READ, DATA, RESTORE](#read-data-restore), [SAVE](#save) |
| `DEF FN` | statement | [DEF FN, FN](#def-fn-fn) |
| `DEF PROC` | statement | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `DELETE` | statement | [DELETE](#delete) |
| `DIM` | statement | [DIM](#dim) |
| `DIR$` | function | [CD, DIR$](#cd-dir) |
| `DISC` | statement | [DISC](#disc) |
| `DRAW` | statement | [DRAW](#draw) |
| `EDIT` | statement | [EDIT](#edit) |
| `ELSE` | in `IF` | [IF, THEN, ELSE](#if-then-else) |
| `END IF` | statement | [IF, THEN, ELSE](#if-then-else) |
| `END PROC` | statement | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `END SELECT` | statement | [SELECT CASE](#select-case) |
| `ERASE` | statement | [ERASE](#erase) |
| `ERL` | function | [ERR, ERL](#err-erl) |
| `ERR` | function | [ERR, ERL](#err-erl) |
| `EXP` | function | [EXP, LN](#exp-ln) |
| `FIND` | statement | [FIND](#find) |
| `FLASH` | statement, item | [BRIGHT, FLASH](#bright-flash) |
| `FLOOR` | statement | [FLOOR](#floor) |
| `FN` | function | [DEF FN, FN](#def-fn-fn) |
| `FOR` | statement | [FOR, NEXT](#for-next) |
| `FORMAT` | statement | [FORMAT](#format) |
| `GO SUB` | statement | [GO SUB, RETURN](#go-sub-return) |
| `GO TO` | statement | [GO TO](#go-to) |
| `GRAB` | statement | [GRAB](#grab) |
| `GRAPHICS` | in `SAVE`, `LOAD`, `VERIFY` | [SAVE](#save), [LOAD](#load) |
| `HIT` | function | [HIT](#hit) |
| `IF` | statement | [IF, THEN, ELSE](#if-then-else) |
| `IN` | function; in `OPEN #` | [IN, OUT](#in-out), [OPEN, CLOSE](#open-close) |
| `INK` | statement, item | [INK, PAPER](#ink-paper) |
| `INKEY$` | function | [INKEY$](#inkey) |
| `INPUT` | statement | [INPUT](#input) |
| `INT` | function | [ABS, SGN, INT](#abs-sgn-int) |
| `INVERSE` | statement, item | [INVERSE, OVER](#inverse-over) |
| `JOY` | function, statement | [JOY](#joy) |
| `KEY` | function | [KEY](#key) |
| `LANGUAGE` | statement | [LANGUAGE](#language) |
| `LEN` | function | [LEN](#len) |
| `LET` | statement | [LET](#let) |
| `LINE` | in `INPUT`, `SAVE` | [INPUT](#input), [SAVE](#save) |
| `LIST` | statement | [LIST](#list) |
| `LLIST` | statement | [LPRINT, LLIST](#lprint-llist) |
| `LN` | function | [EXP, LN](#exp-ln) |
| `LOAD` | statement | [LOAD](#load) |
| `LOCAL` | statement | [LOCAL](#local) |
| `LPRINT` | statement | [LPRINT, LLIST](#lprint-llist) |
| `MAP` | statement | [MAP](#map), [MAP DIM, MAP POKE](#map-dim-map-poke) |
| `MERGE` | statement | [MERGE](#merge) |
| `MKDIR` | statement | [MKDIR, RMDIR](#mkdir-rmdir) |
| `MODE` | statement | [MODE](#mode) |
| `MONITOR` | statement | [MONITOR](#monitor) |
| `MOUSE` | function, statement | [MOUSE](#mouse) |
| `MOVE` | statement | [MOVE](#move) |
| `NEW` | statement | [NEW](#new) |
| `NEXT` | statement | [FOR, NEXT](#for-next) |
| `NOT` | operator | [True and false](#true-and-false) |
| `ON ERROR` | statement | [ON ERROR](#on-error) |
| `OPEN #` | statement | [OPEN, CLOSE](#open-close) |
| `OR` | operator | [True and false](#true-and-false) |
| `OUT` | statement; in `OPEN #` | [IN, OUT](#in-out), [OPEN, CLOSE](#open-close) |
| `OVER` | statement, item | [INVERSE, OVER](#inverse-over) |
| `PAGE` | statement | [PAGE, SCREEN](#page-screen) |
| `PAPER` | statement, item | [INK, PAPER](#ink-paper) |
| `PAUSE` | statement | [PAUSE](#pause) |
| `PEEK` | function | [PEEK, POKE](#peek-poke) |
| `PI` | function | [PI](#pi) |
| `PIXEL` | function | [PIXEL](#pixel) |
| `PLAY` | statement | [PLAY](#play) |
| `PLOT` | statement | [PLOT](#plot) |
| `POINT` | function | [POINT](#point) |
| `POKE` | statement; in `MAP` | [PEEK, POKE](#peek-poke), [MAP DIM, MAP POKE](#map-dim-map-poke) |
| `POLY` | statement | [POLY](#poly) |
| `PRINT` | statement | [PRINT](#print) |
| `PROC` | statement | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `RANDOMIZE` | statement | [RND, RANDOMIZE](#rnd-randomize) |
| `RASTER` | statement | [RASTER](#raster) |
| `READ` | statement | [READ, DATA, RESTORE](#read-data-restore) |
| `REM` | statement | [REM](#rem) |
| `REMAP` | statement | [REMAP](#remap) |
| `RENUM` | statement | [RENUM](#renum) |
| `REPEAT` | statement | [REPEAT, UNTIL](#repeat-until) |
| `REPLACE` | statement | [REPLACE](#replace) |
| `RESTORE` | statement | [READ, DATA, RESTORE](#read-data-restore) |
| `RETURN` | statement | [GO SUB, RETURN](#go-sub-return) |
| `RMDIR` | statement | [MKDIR, RMDIR](#mkdir-rmdir) |
| `RND` | function | [RND, RANDOMIZE](#rnd-randomize) |
| `RUN` | statement | [RUN](#run) |
| `SAVE` | statement | [SAVE](#save) |
| `SCREEN` | statement | [PAGE, SCREEN](#page-screen) |
| `SCREEN$` | function; in `SAVE`, `LOAD` | [SCREEN$, ATTR](#screen-attr), [SAVE](#save) |
| `SCROLL` | statement | [SCROLL](#scroll) |
| `SELECT CASE` | statement | [SELECT CASE](#select-case) |
| `SGN` | function | [ABS, SGN, INT](#abs-sgn-int) |
| `SIN` | function | [SIN, COS, TAN](#sin-cos-tan) |
| `SOUND` | statement | [SOUND](#sound) |
| `SPEED` | statement | [SPEED](#speed) |
| `SPRITE` | statement | [SPRITE](#sprite) |
| `SQR` | function | [SQR](#sqr) |
| `STEP` | in `FOR` | [FOR, NEXT](#for-next) |
| `STOP` | statement | [STOP](#stop) |
| `STR$` | function | [STR$, VAL, VAL$](#str-val-val) |
| `TAB` | in `PRINT`, `INPUT` | [PRINT](#print) |
| `TAN` | function | [SIN, COS, TAN](#sin-cos-tan) |
| `TEXTURE` | statement | [TEXTURE](#texture) |
| `THEN` | in `IF` | [IF, THEN, ELSE](#if-then-else) |
| `TILE` | statement | [TILE](#tile) |
| `TIME` | function | [TIME](#time) |
| `TO` | in `FOR`, slices, `MOVE`, `COPY` | [FOR, NEXT](#for-next), [Strings](#strings) |
| `TRACE` | statement | [TRACE](#trace) |
| `UNTIL` | statement | [REPEAT, UNTIL](#repeat-until) |
| `USR` | function | [USR](#usr) |
| `VAL` | function | [STR$, VAL, VAL$](#str-val-val) |
| `VAL$` | function | [STR$, VAL, VAL$](#str-val-val) |
| `VERIFY` | statement | [VERIFY](#verify) |
| `WAIT` | statement | [WAIT](#wait) |
| `WEND` | statement | [WHILE, WEND](#while-wend) |
| `WHILE` | statement | [WHILE, WEND](#while-wend) |
