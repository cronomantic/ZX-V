**English** · [Español](../../es/basic/reference.md)

# BASIC reference

This page describes every statement, function and operator of ZX-V BASIC,
plus the error messages, the character set and the limits of the machine. It
is a reference: to learn the language step by step, start with the
[BASIC course](README.md); to learn how to type and change programs, read
[the editor](editor.md).

## How to read this reference

Each statement and each function has an entry with the same parts, always in
this order:

1. **Syntax**, in a box:

   ```
   PRINT [AT row, column;] [item] [; item]...
   ```

2. **What it does**, in one or two sentences, with anything that may
   surprise you.
3. **Parameters**: a table with each value you supply, the values it takes
   (ranges, units, special values, the one used if you leave it out) and
   what it is.
4. **Returns** (functions only): the value they give and what it means.
5. **Errors**: a table with the reports it can give and when. The
   `(common)` row points to the [errors any statement can
   give](#errors-in-any-statement), which are not repeated in each entry.
6. **Example**: a few lines you can type as they are.
7. **See also**: the related entries.

The entries that explain an idea and are not a statement (Numbers, Strings,
Variables...) do not have these parts: they are an explanation.

In the syntax:

- Words in UPPER CASE are keywords: type them as they are (in upper or lower
  case, it makes no difference).
- Words in lower case are things you supply: `n`, `x`, `row` are numbers or
  numeric expressions; names ending in `$` (`a$`, `name$`) are strings;
  `line` is a line number; `var` is a variable name.
- `[` `]` mark an optional part; `...` means the part before can be repeated;
  `|` separates alternatives (`IN | OUT` means "IN or OUT").
- Punctuation such as `,`, `;`, `(` and `#` is typed exactly as shown.

When something goes wrong, the machine gives an **error report**, written
here as its code and message, for example `B Integer out of range` (see
[Error messages](#error-messages)).

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
  operators, whole-number arguments and the common errors
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
  `PAGE`, `SCREEN`, `SCROLL`, `REMAP`, `PALETTE`, `RASTER`
- [Sprites, tile maps and fast graphics](#sprites-tile-maps-and-fast-graphics):
  `GRAB`, `SPRITE`, `TILE`, `MAP`, `FLOOR`, `ROTATE`, `BOX`, `DISC`, `POLY`,
  `PATTERN`...
- [Sound and music](#sound-and-music): `BEEP`, `PLAY`, `SOUND`, `MUSIC`,
  `SFX`, `SAMPLE`
- [Keyboard, joystick and mouse](#keyboard-joystick-and-mouse): `INPUT`,
  `INKEY$`, `KEY`, `JOY`, `MOUSE`
- [Files and the SD card](#files-and-the-sd-card): `SAVE`, `LOAD`, `CAT`,
  `CD`, `COPY`, `ERASE`...
- [Tape files](#tape-files): the Spectrum's `.tap` and `.tzx`
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

When you press Enter, the editor checks the line. If it is not valid BASIC,
it neither stores nor runs it: the status line says what is wrong (nearly
always `C Nonsense in BASIC`) and the cursor stays where the fault is. So
most typing mistakes show up as you type, not when the program runs.

When something goes wrong while running, the error report says where:
`2 Variable not found, 20:1` means statement 1 of line 20 (see
[Reports](#reports)).

### Numbers

Numbers are written as `42`, `3.25`, `.5` (with a point, not a comma) or in
scientific notation: `1E6` is 1 000 000 and `2.5E-3` is 0.0025 (`-7` is the
number 7 with the minus sign in front). They go from about ±2.9E-39 to
±1.7E38; a larger result gives `6 Number too big`, and a smaller one becomes
0. [`BIN`](#bin) writes a number in binary.

`PRINT` shows up to 8 significant digits, rounded and without trailing
zeros. It switches to scientific notation when there are 9 or more digits
before the point, or more than 4 zeros after it; numbers between 0 and 1
with a zero after the point are written without the leading 0:

```
PRINT 1/3, 2^31, 1E10, 0.000001, 0.05
```

shows `0.33333333`, `2.1474836E+9`, `1E+10`, `1E-6` and `.05`.

Inside, numbers have more precision than on the Spectrum, but comparisons
(`=`, `<`, `>`...) are done with the same precision as the Spectrum, so
`0.1*3=0.3` is true.

### Strings

A string is a piece of text between double quotes: `"Hello"`. To put a quote
inside a string, write it twice: `PRINT "say ""hi"""` shows `say "hi"`. The
empty string is `""`. A string can be as long as memory allows.

`+` joins strings: `"ZX" + "-V"` is `"ZX-V"`.

A **slice** is part of a string. Character positions start at 1:

| Expression | Result | Meaning |
|---|---|---|
| `"hello"(2 TO 3)` | `"el"` | characters 2 to 3 |
| `"hello"(3)` | `"l"` | character 3 |
| `"hello"(TO 2)` | `"he"` | from the start to 2 |
| `"hello"(3 TO)` | `"llo"` | from 3 to the end |

A slice that starts after it ends is the empty string (`"hello"(3 TO 2)` is
`""`, with no error); otherwise a position outside the string (0 too) gives
`3 Subscript wrong`, and one greater than 65535 or negative,
`B Integer out of range`. Slices work on string variables, on strings
written in the program and on any expression in brackets: `(a$+b$)(2 TO)`.
You can also assign to a slice: `LET a$(1)="J"` changes the first character
(without changing the length of `a$`).

Strings are compared character by character using their codes (see
[Character set](#character-set)), so `"B"<"a"` is true, and `"ab"<"abc"`
too.

### Variables

A variable holds a value under a name.

- **Numeric variables** have names made of letters and digits, starting with
  a letter: `x`, `score`, `level2`. Names can be long (up to 63 characters),
  upper and lower case are the same (`Score` and `SCORE` are one variable),
  and spaces inside the name are ignored (`my score` is `myscore`). They do
  not take accented letters or ñ (if you type them, they are written without
  the accent: `número` becomes `numero`).
- **String variables** have a single letter followed by `$`: `a$`, `n$`.
  There are 26 of them.
- The control variable of [`FOR`](#for-next) is a single letter, and it is an
  ordinary numeric variable: `FOR i` uses (and changes) any `i` there was.
- A name cannot be a Spectrum keyword or a ZX-V function: `LET print=1` and
  `LET time=1` are errors. ZX-V's own statements (`MODE`, `WHILE`,
  `SPRITE`...) are only recognised at the start of a statement, so they are
  fine as names (`LET mode=1`), so as not to break old programs. A keyword
  inside a longer name is fine (`total`, `ink2`).

A variable is created the first time it is given a value (with `LET`,
`INPUT`, `READ`, `FOR`, or as a parameter of a procedure). Reading a
variable that does not exist gives `2 Variable not found`. [`RUN`](#run) and
[`CLEAR`](#clear) delete all the variables.

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
string array `a$` replaces the string variable `a$`. A subscript outside its
dimension, or a number of subscripts that is not the array's, gives
`3 Subscript wrong`; using an array before its `DIM`, `2 Variable not found`.

**String arrays** are arrays of characters: the last dimension is the fixed
length of each string. `DIM n$(3,10)` makes 3 strings of 10 characters,
filled with spaces. `n$(2)` is the second string, and `n$(2,1)` its first
character. Assigning a shorter text fills the rest with spaces; a longer one
is cut:

```
10 DIM n$(2,4)
20 LET n$(1)="good morning"
30 PRINT n$(1); "|"; n$(2); "|"
```

shows `good|    |`. `n$` without subscripts is the whole array as a single
string (here, of 8 characters), and `LET n$="..."` fills it from start to
end in the same way, without changing its size.

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
-4. Brackets change the order: `(1+2)*3` is 9. A `+` in front of a number is
allowed and does nothing.

Functions take only the value that follows them: `SQR 16+9` is `(SQR 16)+9`
= 13, and `SQR (16+9)` is 5. When in doubt, use brackets.

Both sides of an operator must be of the same type (two numbers or two
strings); otherwise, `C Nonsense in BASIC`. With strings only `+`, the
comparisons and `AND` (with a number on the right) are allowed.

Dividing by zero gives `6 Number too big`, and so does raising 0 to a
negative number. Raising a negative number to a power gives
`A Invalid argument`, even if the power is whole (`(-2)^2`); `0^0` is 1.

### True and false

Comparisons give 1 when true and 0 when false: `PRINT 3>2` shows 1. `IF`,
`WHILE` and `UNTIL` take any number: 0 is false and anything else is true.

| Expression | Value |
|---|---|
| `NOT x` | 1 if `x` is 0, otherwise 0 |
| `x AND y` | `x` if `y` is not 0, otherwise 0 |
| `x OR y` | 1 if `y` is not 0, otherwise `x` |
| `a$ AND y` | `a$` if `y` is not 0, otherwise `""` |

So `IF a>0 AND a<10 THEN ...` works as you expect, and a trick like
`PRINT ("valid" AND ok); ("not valid" AND NOT ok)` prints one thing or the
other.

### Whole-number arguments

Many statements need whole numbers (a line number, a colour, a coordinate).
If you give a number with decimals it is rounded to the nearest whole
number, with halves going up: 2.5 becomes 3 and -2.5 becomes -2. A value
between -0.5 and 0 counts as 0.

Nearly all of them first read the number with a wide range, the one for its
type: 0 to 255 for colours, rows and columns; 0 to 65535 for line numbers
and times. A negative value or one above that maximum always gives
`B Integer out of range`. Then each statement checks its own range, and
there the report may be a different one. For example, with `INK` (which
takes 0 to 9):

| Statement | Report | Why |
|---|---|---|
| `INK 10` | `K Invalid colour` | Between 0 and 255, but not a colour. |
| `INK 300` | `B Integer out of range` | Greater than 255. |
| `INK -1` | `B Integer out of range` | Negative. |

Each entry gives both ranges and which report each one gives.

### Errors in any statement

These reports can come from almost any statement or function, because they
arise while working out its values or while the program runs. The error
tables of each entry sum them up in the `(common)` row.

| Report | When |
|---|---|
| `C Nonsense in BASIC` | The line is not valid BASIC, or it mixes numbers and strings where it cannot (`LET a="x"`, `"a"+1`). Nearly always the editor says so when you type the line, and does not store it; the entries say when it comes up while running. |
| `2 Variable not found` | A variable with no value is used, or an array before its `DIM`. |
| `3 Subscript wrong` | A subscript or a slice outside the array or the string. |
| `4 Out of memory` | There is no memory left for a result, or the expression has too many brackets or `FN` one inside another. |
| `6 Number too big` | A result greater than 1.7E38, a division by zero or 0 raised to a negative number. |
| `A Invalid argument` | A function with a value it does not take: `SQR -1`, `LN 0`, `ASN 2`, the power of a negative number. |
| `B Integer out of range` | A whole-number argument that is negative or greater than the maximum for its type (see [Whole-number arguments](#whole-number-arguments)). |
| `L BREAK into program` | You pressed BREAK (Esc) while the program was running, or during a wait (`PAUSE`, `WAIT`, `BEEP`...). `CONTINUE` carries on from the next statement. |

## Program flow

### GO TO

```
GO TO line
GO TO @label
```

Jumps to a line and carries on from there. If that line does not exist, it
goes to the next one after it; if there is none, the program ends with
`0 OK`. With a [label](#label) it jumps to that exact statement, even if it
is not the first one on its line. You can also type `GOTO`, as one word.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `line` | 0–61439; any expression (`GO TO n*100`) | The line number. Lines go up to 9999: a larger number ends the program. |
| `label` | a name | A `@name` label in the program. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `line` is negative or greater than 61439. |
| `X Label not found` | There is no `@label` with that name. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PRINT "forever"
20 GO TO 10
```

Press BREAK (Esc) to stop it.

**See also:** [GO SUB](#go-sub-return), [@label](#label), [RUN](#run),
[ON ERROR](#on-error).

### GO SUB, RETURN

```
GO SUB line
GO SUB @label
RETURN
```

`GO SUB` jumps to a subroutine, like `GO TO`, but remembers where it came
from; `RETURN` goes back to the statement after the `GO SUB`. A subroutine
can call others, up to 512 pending `GO SUB`. You can also type `GOSUB`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `line` | 0–61439 | As in [`GO TO`](#go-to). |
| `label` | a name | A `@name` label in the program. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `line` is negative or greater than 61439. |
| `X Label not found` | There is no `@label` with that name. |
| `4 Out of memory` | There are already 512 `GO SUB` without their `RETURN`. |
| `7 RETURN without GOSUB` | `RETURN` with no pending `GO SUB` to go back to. |
| `N Statement lost` | The line `RETURN` goes back to no longer has that statement (the program was changed with the `GO SUB` pending). |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 GO SUB 100: GO SUB 100
20 STOP
100 PRINT "in the subroutine"
110 RETURN
```

**See also:** [DEF PROC](#def-proc-proc-end-proc), which is usually clearer;
[GO TO](#go-to), [@label](#label).

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
`ELSE`, if there is one on the same line, or at the next line.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `condition` | a number | True if it is not 0 (see [True and false](#true-and-false)). A string is an error. |
| `statements` | statements separated by `:` | What runs. A statement must follow `THEN`: a number on its own (`IF a THEN 100`) is an error; write `IF a THEN GO TO 100`. |

Everything after `THEN` up to `ELSE` is the true part, and everything after
`ELSE` up to the end of the line, the false part. With several `IF` on one
line, each `ELSE` goes with the nearest `IF`.

**The block IF.** If `THEN` is the last thing on the line, the `IF` opens a
block of several lines, which ends at `END IF`. Inside it there can be
`ELSE IF condition THEN` parts (also at the end of their line), tried in
order if nothing before was true, and an `ELSE` part for when none is. Only
one part runs; then the program carries on after `END IF`.

- `ELSE`, `ELSE IF` and `END IF` start their line. `ELSE` can be followed by
  statements on the same line (`ELSE PRINT "Adult"`), and `END IF` by others
  separated by `:`. An `ELSE` that does not start the line belongs to a
  one-line IF.
- Blocks can go one inside another, with no limit, and contain one-line IFs,
  loops or `GO SUB`. You can leave a block with `GO TO`.
- It only works in a program: as a direct command, `IF a THEN` does nothing.

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | When typing the line: the condition is a string, `THEN` is missing, a number follows `THEN`, or an `ELSE IF ... THEN` does not end the line. |
| `Y Structure error` | A block is missing its `END IF`. It comes up when it has to be looked for: when skipping a part that is not true, or at the end of the one that was. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 LET n=-5
20 IF n<0 THEN PRINT "negative" ELSE PRINT "zero or positive"
30 IF n<0 THEN PRINT "a": PRINT "b" ELSE PRINT "c": PRINT "d"
```

Line 30 prints `a` and `b`. With a block:

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

**See also:** [SELECT CASE](#select-case), [WHILE](#while-wend),
[Expressions](#expressions-and-operators),
[True and false](#true-and-false).

### SELECT CASE

```
SELECT CASE value
CASE values
  lines
CASE ELSE
  lines
END SELECT
```

Chooses a part by `value`: the part of the first `CASE` that matches runs
(or the `CASE ELSE` part if none does), and the program carries on after
`END SELECT`. If none matches and there is no `CASE ELSE`, nothing runs.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `value` | a number or a string | What is compared with each `CASE`. |
| `values` | a list separated by commas (table) | Matches if any in the list matches. All of them must be of the type of `value`. |

| Value | Matches if |
|---|---|
| `5`, `"yes"`, `a+1` | `value` is equal |
| `1 TO 9`, `"a" TO "m"` | `value` is between the two (both included) |
| `< 0`, `>= 100`, `<> 7`, `= 3` | `value` passes the comparison |

- The `CASE` are tried in order, and `CASE ELSE` is taken as soon as it is
  reached: put it last, because the `CASE` after it are never tried.
- Numbers are compared with their full precision, not that of `=`:
  `SELECT CASE 0.1*3` does not match `CASE 0.3`. For values with decimals,
  use ranges (`CASE 0.29 TO 0.31`).
- `CASE` can go on its own line or after `:`, and the statements of each
  part on the same line or the following ones; it can all even go on one
  line, also as a direct command. `SELECT CASE` can go one inside another.

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | A value of a `CASE` is not of the type of `value` (it comes up when comparing it), or the values of a `CASE` are followed by something that is not `:` or the end of the line. |
| `Y Structure error` | `END SELECT` is missing. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

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

On one line: `SELECT CASE n: CASE 1: PRINT "one": CASE ELSE: PRINT
"other": END SELECT`.

**See also:** [IF](#if-then-else).

### FOR, NEXT

```
FOR var = first TO last [STEP step]
NEXT var
```

Repeats the statements between `FOR` and `NEXT`, with the variable going
from `first` to `last`, adding `step` each time. With a negative step it
counts down; with `STEP 0`, it never ends. If the loop should not run at all
(`FOR i=1 TO 0`), the program jumps to after the first `NEXT` of that
variable further on.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `var` | a letter | The control variable, numeric. If there was already a variable with that name, it replaces it. |
| `first`, `last` | numbers | The first value and the last one. |
| `step` | a number; 1 if omitted | What is added on each turn. |

After the loop the variable keeps the first value past the end (in the
example, 0). `NEXT` compares with the precision of `=`, so
`FOR x=0 TO 1 STEP 0.1` reaches 1.

**Errors**

| Report | When |
|---|---|
| `I FOR without NEXT` | The loop does not run at all and there is no `NEXT` of that variable further on. |
| `1 NEXT without FOR` | The variable of `NEXT` exists, but is not that of a `FOR`. |
| `2 Variable not found` | There is no variable with the name of `NEXT`. |
| `6 Number too big` | When adding the step, the variable goes past 1.7E38. |
| `C Nonsense in BASIC` | When typing the line: `var` is not a single letter, or it is a string. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 FOR i=1 TO 10 STEP 3: PRINT i;" ";: NEXT i
20 PRINT
30 FOR i=3 TO 1 STEP -1: PRINT i: NEXT i
```

**See also:** [WHILE](#while-wend), [REPEAT](#repeat-until).

### WHILE, WEND

```
WHILE condition
WEND
```

Repeats the statements between `WHILE` and `WEND` while the condition is
true. The condition is checked at the start, so if it is false the first
time, the loop is skipped entirely. Loops can be nested: `WHILE`, `REPEAT`
and procedures, up to 64 one inside another.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `condition` | a number | True if it is not 0. |

**Errors**

| Report | When |
|---|---|
| `Y Structure error` | `WEND` without its `WHILE`, or the condition is false and there is no `WEND` further on. |
| `4 Out of memory` | More than 64 loops and procedures one inside another. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 LET n=1
20 WHILE n<100: LET n=n*2: WEND
30 PRINT n
```

shows 128.

**See also:** [REPEAT](#repeat-until), [FOR](#for-next).

### REPEAT, UNTIL

```
REPEAT
UNTIL condition
```

Repeats the statements between `REPEAT` and `UNTIL` until the condition is
true. The condition is checked at the end, so the loop runs at least once.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `condition` | a number | The loop ends when it is not 0. |

**Errors**

| Report | When |
|---|---|
| `Y Structure error` | `UNTIL` without its `REPEAT`. |
| `4 Out of memory` | More than 64 loops and procedures one inside another. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 REPEAT
20 LET d=INT (RND*6)+1: PRINT d
30 UNTIL d=6
```

**See also:** [WHILE](#while-wend), [FOR](#for-next).

### DEF PROC, PROC, END PROC

```
DEF PROC name [(param [, param]...)]
END PROC
PROC name [(value [, value]...)]
```

A **procedure** is a named piece of program. `DEF PROC` starts it and
`END PROC` ends it; `PROC` runs it and then comes back to the next
statement. The parameters receive the values given in `PROC` and are local:
at `END PROC` they get back the value they had before (or stop existing, if
they did not exist). A procedure can call itself.

When the program reaches a `DEF PROC` while running normally, it skips the
whole procedure, so procedures can go anywhere. A procedure does not return
a value: to give a result back, store it in a variable.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `name` | like a numeric variable name | The name of the procedure: letters and digits, case and spaces ignored. |
| `param` | up to 16 | A numeric variable name, or a letter with `$` for a string. |
| `value` | up to 16, one per parameter | A number or a string, of the type of its parameter. |

`END PROC` also closes any `WHILE` and `REPEAT` left open inside the
procedure.

**Errors**

| Report | When |
|---|---|
| `Z PROC not found` | There is no `DEF PROC` with that name. |
| `Q Parameter error` | The number or the type of the values of `PROC` do not match the parameters, or there are more than 16. |
| `Y Structure error` | `END PROC` outside a procedure, or the program reaches a `DEF PROC` that has no `END PROC` before the next `DEF PROC`. |
| `4 Out of memory` | More than 64 loops and procedures one inside another, or there is no room for more saved values of parameters and local variables (8 KB for all of them). |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PROC greet("Ann", 2)
20 PROC greet("Bob", 1)
30 STOP
100 DEF PROC greet(n$, times)
110 FOR i=1 TO times: PRINT "Hello, "; n$: NEXT i
120 END PROC
```

**See also:** [LOCAL](#local), [GO SUB](#go-sub-return).

### LOCAL

```
LOCAL var [, var]...
```

Inside a procedure, makes those variables local: they start as 0 (or `""`)
and get their old values back at `END PROC`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `var` | up to 16 names | Numeric variables (any name) or string variables (`a$`). Not arrays. |

**Errors**

| Report | When |
|---|---|
| `Y Structure error` | `LOCAL` outside a procedure. |
| `Q Parameter error` | More than 16 names in one `LOCAL`. |
| `4 Out of memory` | There is no room for more saved values of parameters and local variables (8 KB for all of them). |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

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

A statement made of `@` and a name marks a place in the program, and does
nothing else. `GO TO`, `GO SUB`, `RUN`, `RESTORE` and `ON ERROR GO TO` can
jump to it by name instead of by line number. `GO TO`, `GO SUB`, `RUN` and
`ON ERROR GO TO` go to the exact statement (it does not need to be the first
one on its line); `RESTORE`, to the start of its line. Labels do not change
when you renumber. If two labels have the same name, the first one counts.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `name` | like a numeric variable name | Letters and digits, case and spaces ignored. |

**Errors**

| Report | When |
|---|---|
| `X Label not found` | (In the statement that jumps.) There is no label with that name. |
| `C Nonsense in BASIC` | When typing the line: `@` is not followed by a valid name. |

**Example**

```
10 GO SUB @hello: GO SUB @hello
20 STOP
30 @hello: PRINT "hello": RETURN
```

**See also:** [GO TO](#go-to), [RESTORE](#read-data-restore),
[RENUM](#renum).

### RUN

```
RUN [line | @label]
```

Runs the program from the beginning, or from that line or label. Before
starting it stops the music and the sound effects and clears everything like
[`CLEAR`](#clear): deletes the variables and clears the screen, closes open
files, turns off `ON ERROR`, `CLIP` and `PATTERN`, and makes `READ` start
from the first `DATA`. It does not change the video mode, the colours,
`TRACE` or the stored sprites. To start without losing the variables, use
`GO TO`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `line` | 0–65535; 0 if omitted | Where it starts. If it does not exist, at the next one there is; if there is none, nothing runs (`0 OK`). |
| `label` | a name | A `@name` label in the program. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `line` is negative or greater than 65535. |
| `X Label not found` | There is no `@label` with that name. |
| (common) | And those of the program it runs. See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
RUN 100
```

**See also:** [CONTINUE](#continue), [STOP](#stop), [CLEAR](#clear),
[GO TO](#go-to).

### STOP

```
STOP
```

Stops the program with the report `9 STOP statement`. [`CONTINUE`](#continue)
carries on from the next statement. `ON ERROR` does not catch `STOP`.

**Parameters:** none.

**Errors**

| Report | When |
|---|---|
| `9 STOP statement` | Always: it is how it stops. |

**Example**

```
10 PRINT "one": STOP: PRINT "two"
```

After `9 STOP statement, 10:2`, `CONTINUE` prints `two`.

**See also:** [CONTINUE](#continue), [ON ERROR](#on-error).

### CONTINUE

```
CONTINUE
```

Carries on after a stop: after `STOP` or BREAK (`L`), from the next
statement; after an error (`D BREAK - CONT repeats` too), it repeats the
statement that failed, which is useful once you have fixed the cause (for
example, by giving a missing variable a value).

It only works for programs: after a stop in a direct command, that command
is no longer there (the edit line is now the `CONTINUE` itself), so it gives
`N Statement lost`. (On the Spectrum, the `CONTINUE` repeated for ever.)

**Parameters:** none.

**Errors**

| Report | When |
|---|---|
| `N Statement lost` | The stop was in a direct command, or the line where it has to carry on no longer has that statement (it has been changed). |
| (common) | And those of the statement it repeats. See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PRINT x
```

`RUN` gives `2 Variable not found, 10:1`; then `LET x=5` and `CONTINUE`
print 5.

**See also:** [STOP](#stop), [ON ERROR](#on-error), [Reports](#reports).

### TRACE

```
TRACE ON
TRACE #stream
TRACE OFF
```

To follow where a program goes: each time a line starts running (also when
it comes back to it with `GO TO`, `NEXT` or `RETURN`), its number is printed
in brackets, like `[10][20][30]`. `TRACE OFF` turns it off; it can also go
inside the program, to follow only one part. `NEW` turns it off, `RUN` does
not. If the stream is closed, it turns itself off.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `ON` | — | On the program screen (stream 2), mixed with what the program prints. |
| `stream` | 0–15, open | Through that stream: `TRACE #3` goes to the printer (the file `printer.txt`), and a stream opened with `OPEN #` to a file, to that file, so that the screen is not spoilt. |

**Errors**

| Report | When |
|---|---|
| `O Invalid stream` | The stream is not open. |
| `B Integer out of range` | `stream` is negative or greater than 15. |
| `C Nonsense in BASIC` | When typing the line: `TRACE` is not followed by `ON`, `OFF` or `#`. |

**Example**

```
TRACE ON: RUN
```

**See also:** the [BASIC debugger](../emulator.md#debugging), to stop a
program at a line and look at its variables from VS Code;
[OPEN #](#open-close).

### REM

```
REM any text
```

A remark: everything after `REM` up to the end of the line is ignored,
including any `:` and statements there.

**Parameters:** none. **Errors:** none.

**Example**

```
10 REM Space game, by Ann
```

**See also:** [Program lines and statements](#program-lines-and-statements).

### PAUSE

```
PAUSE n
```

Waits `n` fiftieths of a second (`PAUSE 50` is one second, as on the
Spectrum), or until a key is pressed, whichever comes first. Only keys
pressed after the `PAUSE` starts count.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0–65535 | Fiftieths of a second. 0 waits for a key with no limit. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `n` is negative or greater than 65535. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PRINT "Press a key": PAUSE 0: PRINT "Thanks"
```

**See also:** [WAIT](#wait), [INKEY$](#inkey).

### WAIT

```
WAIT [n]
```

Waits for the next frame, and until `n` frames have passed since the
previous `WAIT`. There are 60 frames a second. This keeps a game loop at a
steady speed even if some turns of the loop take longer than others:
`WAIT 2` gives 30 turns a second. It does not look at the keyboard.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0–65535; 1 if omitted | Frames since the previous `WAIT`. With 0 (or if they have already passed), it only waits for the next frame. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `n` is negative or greater than 65535. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 FOR x=0 TO 200: BOX 0,x,80,8,8: WAIT: NEXT x
```

**See also:** [PAUSE](#pause), [TIME](#time).

## Variables and data

### LET

```
LET var = value
```

Gives a value to a variable, creating it if needed. `LET` is needed (`a=1`
on its own is an error). `var` can also be an array element (`t(3)`) or a
slice of a string (`a$(2 TO 4)`); when assigning to a slice, the text is cut
or filled with spaces so that the string does not change length.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `var` | a variable, an array element or a slice of a string | Where it is stored. See [Variables](#variables). |
| `value` | a number or a string | Of the type of `var`: a number for numeric variables and a string for `$` variables. |

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | When typing the line: `LET` or `=` is missing, the name is a keyword or is longer than 63 characters, or the value is not of the type of the variable. |
| `2 Variable not found` | Assigning to an element of an array without `DIM`, or to a slice of a string that does not exist. |
| `3 Subscript wrong` | The subscript or the slice is outside the array or the string. |
| `4 Out of memory` | There is no room for the variable. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 LET a=5: LET a=a+1: LET b$="ZX"
20 LET b$(2)="-": PRINT a; b$
```

shows `6Z-`.

**See also:** [Variables](#variables), [DIM](#dim), [Strings](#strings).

### DIM

```
DIM letter(size [, size]...)
DIM letter$(size [, size]...)
```

Creates an array (see [Arrays](#arrays)), replacing any array with the same
name (`DIM a$` also replaces the string variable `a$`). Numeric arrays start
full of zeros and string arrays full of spaces.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `letter` | a letter | The name of the array. |
| `size` | 1–65535; up to 16 dimensions | How many elements each dimension has. In string arrays, the last one is the length of each string. In total, up to 16 777 215 elements, if they fit in memory. |

**Errors**

| Report | When |
|---|---|
| `3 Subscript wrong` | A size is 0, or there are more than 16 dimensions (this one, when typing the line). |
| `B Integer out of range` | A size is negative or greater than 65535. |
| `4 Out of memory` | The array has more than 16 777 215 elements or does not fit in memory. |
| `C Nonsense in BASIC` | When typing the line: the name has more than one letter. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 DIM s(10): LET s(3)=42: PRINT s(3), s(4)
```

**See also:** [Arrays](#arrays), [LET](#let), [CLEAR](#clear).

### READ, DATA, RESTORE

```
DATA value [, value]...
READ var [, var]...
RESTORE [line | @label]
```

`DATA` lines hold a list of values inside the program; `READ` takes them one
by one, in order, and stores them in variables. `RESTORE` makes the next
`READ` start again from the first `DATA` in the program (or from the first
one at or after that line, or from the start of that label's line). `DATA`
does nothing when the program reaches it.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `value` | any expression | It is worked out when it is read, not before: it can use variables (`DATA x*2`). It must be of the type of the variable that reads it. |
| `var` | variables, array elements or slices of strings | Where each value is stored, as in `LET`. |
| `line` | 0–65535; 0 if omitted | Where the search for the next `DATA` starts. |
| `label` | a name | A `@name` label; the search starts at the start of its line. |

**Errors**

| Report | When |
|---|---|
| `E Out of DATA` | `READ` after the last value. |
| `C Nonsense in BASIC` | The value read is not of the type of the variable, or in the `DATA` it is followed by something that is not `,`, `:` or the end of the line. |
| `B Integer out of range` | The `line` of `RESTORE` is negative or greater than 65535. |
| `X Label not found` | There is no `@label` with that name. |
| (common) | And those of working out the value (those of a `DATA` come up at the line of the `READ`). See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 FOR i=1 TO 3
20 READ n$, age: PRINT n$; " is "; age
30 NEXT i
40 DATA "Ann", 12, "Bob", 9, "Eve", 10+1
```

**See also:** [LET](#let), [@label](#label).

### CLEAR

```
CLEAR [address]
```

Deletes all variables and clears the screen, closes the files opened with
`OPEN #`, forgets the pending returns of `GO SUB`, loops and procedures,
turns off `ON ERROR`, `CLIP` and the `PATTERN` texture, moves the starting
point of `DRAW` to (0, 0) and makes `READ` start from the first `DATA`.
Sprites and other stored graphics, the colours and the video mode are kept,
and the music goes on playing. [`RUN`](#run) does a `CLEAR` first.

With an address, it also moves a memory limit (useful only for advanced
programs).

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `address` | 0; 23821–65535; or a real address from 65536 up | With 0 (or without it), nothing more. Below 65536 it sets the "RAMTOP" of the [virtual Spectrum memory](#the-virtual-spectrum-memory) (65367 at switch-on). From 65536 up it is a real address in the BASIC memory (rounded to a multiple of 4), and the graphics store is moved just below it, leaving the memory above it free for your own use. |

**Errors**

| Report | When |
|---|---|
| `M RAMTOP no good` | `address` is between 1 and 23820; or it is a real address above the BASIC memory, or so low that it would leave no room for the variables (1 KB more) and the graphics store. |
| `B Integer out of range` | `address` is negative or greater than 4 294 967 295. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 LET a=1: CLEAR: PRINT a
```

gives `2 Variable not found, 10:3`.

**See also:** [RUN](#run), [NEW](#new), [The virtual Spectrum
memory](#the-virtual-spectrum-memory).

### DEF FN, FN

```
DEF FN letter([param [, param]...]) = expression
DEF FN letter$([param [, param]...]) = string expression
FN letter(values)
FN letter$(values)
```

`DEF FN` defines your own function, and `FN` uses it inside an expression.
`DEF FN` can be anywhere in the program and does nothing when reached; `FN`
looks for the definition in the program each time (if there are two with
the same name, the first one counts). The brackets are always needed, even
with no parameters: `DEF FN r()=INT (RND*6)+1` and `FN r()`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `letter` | a letter, with `$` if it returns a string | The name of the function. |
| `param` | a letter, or a letter with `$` | Inside the expression, it takes the value given in `FN`. Outside it, the variables with that name do not change. |
| `expression` | number or string, according to the name | What it returns. It can use variables of the program and other `FN`. |
| `values` | up to 32, one per parameter | Numbers or strings, of the type of their parameter. |

**Returns** (`FN`): the value of the expression with those parameters.

**Errors**

| Report | When |
|---|---|
| `P FN without DEF` | There is no `DEF FN` with that name (and that type) in the program. |
| `Q Parameter error` | The number or the type of the values do not match the parameters, or there are more than 32 (counting those of the `FN` being worked out one inside another). |
| `C Nonsense in BASIC` | The type of the expression is not that of the name (when typing the `DEF FN`). |
| `4 Out of memory` | Too many `FN` one inside another (one that calls itself without end). |
| (common) | And those of working out the expression. See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 DEF FN h(x,y)=SQR (x*x+y*y)
20 DEF FN u$(a$)=a$+"!"
30 PRINT FN h(3,4); " "; FN u$("hi")
```

shows `5 hi!`.

**See also:** [DEF PROC](#def-proc-proc-end-proc),
[Expressions](#expressions-and-operators).

## Maths

The functions in this section take a number and return a number. Like all
functions, they take only the value that follows them: `SIN x*2` is
`(SIN x)*2` (see [Expressions](#expressions-and-operators)). An argument
that is not a number gives `C Nonsense in BASIC` when typing the line.

### SIN, COS, TAN

```
SIN x     COS x     TAN x
```

Sine, cosine and tangent of an angle in **radians** (a full turn is
`2*PI`). To use degrees, multiply by `PI/180`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `x` | any number | The angle, in radians. |

**Returns:** the sine or the cosine (from -1 to 1), or the tangent (any
number: `TAN (PI/2)` gives a very large number, not an error).

**Errors**

| Report | When |
|---|---|
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT SIN (PI/2); " "; COS (60*PI/180)
```

shows `1 0.5`.

**See also:** [ASN, ACS, ATN](#asn-acs-atn), [PI](#pi).

### ASN, ACS, ATN

```
ASN x     ACS x     ATN x
```

Arcsine, arccosine and arctangent: the angle whose sine, cosine or tangent
is `x`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `x` | -1 to 1 in `ASN` and `ACS`; any number in `ATN` | The sine, the cosine or the tangent. |

**Returns:** the angle in radians: from `-PI/2` to `PI/2` (`ASN` and `ATN`)
or from 0 to `PI` (`ACS`).

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | In `ASN` or `ACS`, `x` is less than -1 or greater than 1. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT 4*ATN 1
```

shows `3.1415927`.

**See also:** [SIN, COS, TAN](#sin-cos-tan).

### EXP, LN

```
EXP x     LN x
```

`EXP x` is e (2.7182818...) to the power `x`. `LN x` is the natural
logarithm. For a logarithm in base 10, use `LN x/LN 10`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `x` | in `EXP`, up to about 88.7; in `LN`, greater than 0 | The exponent or the number. |

**Returns:** e to the power `x`, or its logarithm.

**Errors**

| Report | When |
|---|---|
| `6 Number too big` | In `EXP`, `x` is more than about 88.7 (the result would be more than 1.7E38). |
| `A Invalid argument` | In `LN`, `x` is 0 or negative. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT EXP 1; " "; LN 100/LN 10
```

shows `2.7182818 2`.

**See also:** [SQR](#sqr), [Expressions](#expressions-and-operators) (the
power `^`).

### SQR

```
SQR x
```

Square root.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `x` | 0 or more | The number. |

**Returns:** the square root of `x`.

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `x` is negative. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT SQR 2
```

**See also:** [EXP, LN](#exp-ln), [ABS, SGN, INT](#abs-sgn-int).

### ABS, SGN, INT

```
ABS x     SGN x     INT x
```

- `ABS x`: `x` without its sign (`ABS -3` is 3).
- `SGN x`: -1, 0 or 1 depending on whether `x` is negative, zero or positive.
- `INT x`: the whole number at or below `x`: `INT 2.7` is 2, and `INT -2.5`
  is -3. To round to the nearest, use `INT (x+0.5)`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `x` | any number | The number. |

**Returns:** as described above.

**Errors**

| Report | When |
|---|---|
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT ABS -3; " "; SGN -3; " "; INT -2.5
```

shows `3 -1 -3`.

**See also:** [Whole-number arguments](#whole-number-arguments).

### PI

```
PI
```

The number π.

**Parameters:** none.

**Returns:** 3.14159265358979..., which `PRINT` shows as `3.1415927`.

**Errors:** none.

**Example**

```
PRINT "Area: "; PI*10^2
```

**See also:** [SIN, COS, TAN](#sin-cos-tan).

### RND, RANDOMIZE

```
RND
RANDOMIZE [n]
```

`RND` gives a random number. For a whole number from 1 to 6, use
`INT (RND*6)+1`.

`RANDOMIZE n` starts the sequence at a fixed point, so the same "random"
numbers come out again. `RANDOMIZE` or `RANDOMIZE 0` starts it from the time
since the machine was switched on. At switch-on the sequence always starts
at the same point; neither `RUN` nor `NEW` changes it.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0–65535; 0 if omitted | The starting point; 0, from the clock. |

**Returns** (`RND`): a number from 0 up to (not including) 1. There are
65536 different ones, in steps of 1/65536, and the sequence repeats every
65536.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `n` is negative or greater than 65535. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 RANDOMIZE
20 FOR i=1 TO 5: PRINT INT (RND*6)+1;" ";: NEXT i
```

> **If you know the Spectrum:** `RND` gives exactly the same sequence as on
> the 48K (`RANDOMIZE 1: PRINT RND` gives `0.0022735596` on both).

**See also:** [INT](#abs-sgn-int), [TIME](#time).

### BIN

```
BIN digits
```

Writes a number in binary, with zeros and ones: `BIN 1010` is 10. Handy for
designing graphics with [`POKE USR`](#user-defined-graphics-udg). It is a
way of writing a number, not a function: it must be followed by the number,
not an expression.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `digits` | zeros and ones, together | The number in binary. The first thing that is not a 0 or a 1 (a space too) ends the number. With no digits, `BIN` is 0. |

Watch the spaces: `BIN 1010 1010` is 10 followed by another number, 1010,
and in a `PRINT` it comes out as `101010`.

**Returns:** the number.

**Errors:** none while running. What follows the number must fit in the
line: `LET a=BIN 2` gives `C Nonsense in BASIC` when typing it.

**Example**

```
PRINT BIN 11111111; " "; BIN 10000000
```

shows `255 128`.

**See also:** [Numbers](#numbers),
[UDG](#user-defined-graphics-udg).

## Text functions

### LEN

```
LEN a$
```

The number of characters in a string.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `a$` | a string | The text. |

**Returns:** its length; `LEN ""` is 0.

**Errors**

| Report | When |
|---|---|
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT LEN "ZX-V"
```

**See also:** [Strings](#strings), [CHR$, CODE](#chr-code).

### CHR$, CODE

```
CHR$ n     CODE a$
```

`CHR$ n` is the character with code `n`, and `CODE a$` the code of the first
character of `a$`. See the [Character set](#character-set).

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0–255 | The code. |
| `a$` | a string | The text; only its first character counts. |

**Returns:** `CHR$`, a one-character string; `CODE`, the code (0–255), or 0
if `a$` is empty.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | In `CHR$`, `n` is negative or greater than 255. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT CHR$ 65; " "; CODE "a"
```

shows `A 97`.

**See also:** [Character set](#character-set),
[STR$, VAL](#str-val-val).

### STR$, VAL, VAL$

```
STR$ x     VAL a$     VAL$ a$
```

- `STR$ x` turns a number into a string, as `PRINT` would show it:
  `STR$ 3.5` is `"3.5"`.
- `VAL a$` works out the string as a numeric expression: `VAL "2*3"` is 6.
- `VAL$ a$` works out the string as a string expression: `VAL$ """x"""` is
  `"x"`.

`VAL` and `VAL$` understand numbers, variables, quotes, brackets and the
signs `+ - * / ^ = < >`, but not keywords: inside a string, `SIN`, `PI`,
`AND` or `<=` are separate letters and signs, not the function or the
operator. So `VAL "PI"` looks for a variable called `pi`, and `VAL "1<=2"`
is an error. Spaces between the items are ignored: `VAL " 12 "` is 12, but
`VAL "1 2"` is an error.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `x` | a number | What is turned into a string. |
| `a$` | a string | The expression that is worked out. |

**Returns:** `STR$`, a string; `VAL`, a number; `VAL$`, a string.

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | The string of `VAL` or `VAL$` is not a valid expression (also if it is empty, or has an operator written with letters or with two signs, such as `AND` or `<=`), or it is not of the type asked for (`VAL "a$"`). |
| `2 Variable not found` | The expression uses a variable that does not exist (also a keyword, as in `VAL "PI"`). |
| (common) | And those of working out the expression. See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 LET a$="12": LET b$="30"
20 PRINT a$+b$; " "; VAL a$+VAL b$
```

shows `1230 42`.

**See also:** [Numbers](#numbers), [LEN](#len), [INPUT](#input).

## The screen and printing

The program screen has an **upper part**, where `PRINT` and the drawing
commands work, and a **lower part** of two lines at the very bottom, used by
`INPUT` and by the reports (it grows if needed). In the normal mode, mode 0,
the screen has 24 rows of 32 characters (256 × 192 pixels), and `PRINT` can
use rows 0 to 21; in mode 1, 48 rows of 64 characters, and `PRINT` uses rows
0 to 45. On a page larger than the screen (see [MODE](#mode)), the rows and
columns are those of the page.

`PRINT` and the drawing commands work on the drawing page (see
[PAGE, SCREEN](#page-screen)), which is normally the one on view.

When printing goes past the last row, the screen scrolls up. Each time a
screenful has scrolled, it stops and asks `scroll?`: press N, Space or BREAK
to stop the program (`D BREAK - CONT repeats`), or any other key to go on.

### PRINT

```
PRINT [item] [separator] [item]...
```

Shows numbers and text on the screen, or somewhere else with `#stream`.

**Parameters**

The items can be:

| Item | Values | Effect |
|---|---|---|
| a number or string expression | — | Prints its value. Numbers, as explained in [Numbers](#numbers). The control characters of a string (`CHR$ 22` is `AT`...) work as on the Spectrum (see [Character set](#character-set)). |
| `AT row, column` | `row`: 0–21 (mode 1: 0–45); `column`: 0–31 (mode 1: 0–63) | Moves the print position. Row 0 is at the top. |
| `TAB column` | 0–65535 | Moves on to that column (counted wrapping round the line: `TAB 33` is `TAB 1` in mode 0), filling with spaces. If it is already past it, it goes to the next line. |
| `INK n`, `PAPER n`, `FLASH n`, `BRIGHT n`, `INVERSE n`, `OVER n` | those of each statement | Change the colours for the rest of this `PRINT` only (see [Colours](#colours)). |
| `#stream` | 0–15, open | What follows goes through that stream: 0 and 1, the lower part of the screen; 2, the upper part; 3, the printer; or a file opened with `OPEN #` (see [Streams](#streams-and-channels)). |

And between them, the separators:

| Separator | Effect |
|---|---|
| `;` | nothing: the next item goes right after |
| `,` | moves to the next column that is a multiple of 16 (in mode 0, to the half of the line or to the next line) |
| `'` | moves to the next line |

If the `PRINT` does not end with a separator, it moves to the next line at
the end. `PRINT` on its own prints an empty line.

**Errors**

| Report | When |
|---|---|
| `5 Out of screen` | `AT` with a row or a column that does not fit (see the table), or the lower part of the screen cannot grow any more. |
| `B Integer out of range` | The row or the column of `AT` is negative or greater than 255, the column of `TAB` is negative or greater than 65535, or the stream, negative or greater than 15. |
| `K Invalid colour` | A colour out of its range (see [INK, PAPER](#ink-paper) and the others). |
| `O Invalid stream` | The stream is not open. |
| `D BREAK - CONT repeats` | N, Space or BREAK was the answer to `scroll?`. |
| `J Invalid I/O device` | The stream is a file opened for reading (`IN`). |
| `U`, `V`... | Those of writing to a file (see [Reading and writing text files](#reading-and-writing-text-files)). |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PRINT "Name", "Score"
20 PRINT "Ann", 120
30 PRINT AT 10,5; "at row 10"; TAB 25; "tab"
40 PRINT "one"'"two"; " and three"
50 PRINT INK 2; "red"; INK 0; " black"
```

**See also:** [LPRINT](#lprint-llist), [INPUT](#input),
[Colours](#colours), [Streams](#streams-and-channels).

### CLS

```
CLS
```

Clears the drawing page with the permanent `PAPER` colour (and `BRIGHT` and
`FLASH`), including what was drawn with the fast graphics commands and the
sprites. The lower part is cleared with the border colour and has two lines
again, and the print position goes back to the top left corner. It does not
change the starting point of `DRAW`.

**Parameters:** none.

**Errors:** none.

**Example**

```
10 PAPER 1: CLS: PRINT "Blue background"
```

**See also:** [INK, PAPER](#ink-paper), [PAGE, SCREEN](#page-screen),
[CLEAR](#clear).

### SCREEN$, ATTR

```
SCREEN$ (row, column)
ATTR (row, column)
```

`SCREEN$` gives the character printed at that position on the screen.
`ATTR` gives the colours of that cell as one number. Both look at the
drawing page, including the lower part.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `row` | 0–23 (mode 1: 0–47) | The character row, counting from the top and including the lower part. |
| `column` | 0–31 (mode 1: 0–63) | The column. |

**Returns:** `SCREEN$`, the character (32 to 127, in the current font,
normal or inverse), or `""` if it does not recognise what is there (a
drawing, a UDG or a block graphic). `ATTR`, the last colours written in the
cell: `FLASH`×128 + `BRIGHT`×64 + `PAPER`×8 + `INK`, from 0 to 255.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | The row or the column is outside the screen (or negative). |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PRINT AT 0,0; INK 2; PAPER 6; "A"
20 PRINT SCREEN$ (0,0); " "; ATTR (0,0)
```

shows `A 50` (6×8 + 2).

**See also:** [POINT](#point), [PIXEL](#pixel), [Colours](#colours).

### LPRINT, LLIST

```
LPRINT [items]
LLIST [#stream [, | ;]] [line]
```

Like `PRINT` and `LIST`, but to the **printer**. ZX-V has no paper printer:
the printer is the text file `printer.txt` at the root of the SD card, and
everything printed is added to its end, line by line (what is left of the
last one is written when the program stops). Colours and `AT` (except its
column) are ignored. Lines are up to 64 characters long (32 in mode 0).
Characters that are not ASCII are written as `?` (the pound sign as `#`,
the © as `c`), and keywords as words. If there is no card, what is printed
is lost with no error.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `items` | those of [`PRINT`](#print) | What is printed. |
| `stream` | 0–15, open; 3 if omitted | In `LLIST`, where the listing goes. |
| `line` | 0–65535; 0 if omitted | In `LLIST`, the first line listed. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | A value of `AT`, `TAB`, the stream or the line out of its range (as in `PRINT`). |
| `K Invalid colour` | A colour out of its range (even though it is not used). |
| `O Invalid stream` | In `LLIST #`, the stream is not open. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
LPRINT "Shopping list": LPRINT "- milk"
```

**See also:** [PRINT](#print), [LIST](#list), [COPY](#copy),
[Streams](#streams-and-channels).

## Colours

Each character cell of the screen has an ink (`INK`, the colour of the
character) and a paper (`PAPER`, the background). Colours are numbered from
0 to 7, and `BRIGHT 1` makes them brighter:

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
from 0 to 15: 0–7 are the colours in the table and 8–15 their bright
versions (10 is bright red). Colour 8 is black too, and is used as
"transparent" in sprites. With [`PALETTE`](#palette), each of the 16 can be
any colour you like.

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

All of them read their value as a number from 0 to 255: a negative value or
one greater than 255 gives `B Integer out of range`, and one from 0 to 255
that is not one of theirs, `K Invalid colour` (see [Whole-number
arguments](#whole-number-arguments)).

```
10 PAPER 1: INK 6: CLS
20 PRINT "yellow on blue"
30 PRINT PAPER 2; INK 7; "white on red, only here"
40 PRINT "yellow on blue again"
```

### INK, PAPER

```
INK n
PAPER n
```

Choose the ink colour and the paper colour for what is printed and drawn
afterwards. Inside a `PRINT` (`PRINT INK 2; "Hello"`) they apply to that
`PRINT` only. `PAPER` takes effect on the whole screen with the next `CLS`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0–7, 8 or 9 | The colour: 0 to 7, those of the [colour table](#colours); 8, transparent (each cell keeps the one it has); 9, contrast (white or black, whichever shows best against the other colour). |

**Errors**

| Report | When |
|---|---|
| `K Invalid colour` | `n` is between 10 and 255. |
| `B Integer out of range` | `n` is negative or greater than 255. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PAPER 1: INK 6: CLS
20 PRINT "Yellow on blue"
```

**See also:** [BORDER](#border), [BRIGHT, FLASH](#bright-flash),
[ATTR](#screen-attr).

### BORDER

```
BORDER n
```

Colour of the border around the screen. The lower part of the screen (the
lines of `INPUT` and the reports) uses the border colour as its paper, with
white ink on the dark colours (0 to 3) and black on the light ones (4 to 7).

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0–7 | The colour, from the [colour table](#colours) (without brightness). |

**Errors**

| Report | When |
|---|---|
| `K Invalid colour` | `n` is between 8 and 255. |
| `B Integer out of range` | `n` is negative or greater than 255. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
BORDER 0: PAPER 0: INK 4: CLS
```

**See also:** [RASTER BORDER](#raster), for stripes of colour;
[INK, PAPER](#ink-paper).

### BRIGHT, FLASH

```
BRIGHT n
FLASH n
```

`BRIGHT 1` uses the bright version of the ink and paper; `BRIGHT 0` the
normal one. `FLASH 1` makes the cell swap its ink and paper every 16
fiftieths of a second (about a third of a second); `FLASH 0` turns it off.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0, 1 or 8 | 0 turns it off, 1 turns it on and 8 is transparent (each cell keeps what it had). |

**Errors**

| Report | When |
|---|---|
| `K Invalid colour` | `n` is between 2 and 7, or between 9 and 255. |
| `B Integer out of range` | `n` is negative or greater than 255. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
PRINT FLASH 1; "GAME OVER"
```

**See also:** [INK, PAPER](#ink-paper), [ATTR](#screen-attr).

### INVERSE, OVER

```
INVERSE n
OVER n
```

`INVERSE 1` prints with ink and paper swapped. `OVER 1` prints over what is
already there, changing each pixel of the character between ink and paper
(printing the same thing twice with `OVER 1` rubs it out). With `PLOT`,
`DRAW` and `CIRCLE`, `OVER 1` changes each point (if it was painted, it rubs
it out) and `INVERSE 1` paints with the paper, that is, rubs out.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `n` | 0 or 1 | 0 turns it off and 1 turns it on. There is no 8 here. |

**Errors**

| Report | When |
|---|---|
| `K Invalid colour` | `n` is between 2 and 255. |
| `B Integer out of range` | `n` is negative or greater than 255. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PRINT AT 5,5; "X": PRINT AT 5,5; OVER 1; "O"
```

**See also:** [PLOT](#plot), [INK, PAPER](#ink-paper).

## Drawing with PLOT and DRAW

These commands draw on the drawing page with the ink colour. Their
coordinates are in pixels, with **(0, 0) at the bottom left** corner: `x`
goes from 0 to 255 to the right and `y` from 0 to 175 upwards (the two
bottom rows belong to the lower part of the screen). In mode 1, `x` goes to
511 and `y` to 367; on a page larger than the screen (see [MODE](#mode)),
`x` goes to the width of the page minus 1 and `y` to its height minus 17.

A point outside the screen gives `B Integer out of range`. A line or a
circle that goes outside gives the report when it reaches the edge: what has
been drawn up to there stays. Watch out: `PLOT` and `POINT` ignore the sign
of the coordinates: `PLOT -10,20` draws at (10, 20).

You can put colour items at the start, followed by `;`, as in
`PLOT INK 2; 10,10`, which apply to that statement only. Without them, each
point only changes the ink of its cell (the one `ATTR` gives): the paper,
brightness and flashing are kept.

`PLOT`, `DRAW` and `CIRCLE` leave the last point drawn as the starting point
of the next `DRAW`. `CLEAR` and `RUN` move it to (0, 0).

> The [fast graphics commands](#sprites-tile-maps-and-fast-graphics) (`BOX`,
> `SPRITE`...) count `y` from the top instead, like `PRINT AT`.

### PLOT

```
PLOT [colours;] x, y
```

Draws a point, which becomes the starting point of the next `DRAW`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `colours` | `INK`, `PAPER`, `FLASH`, `BRIGHT`, `INVERSE`, `OVER` with their value | Colours for this point only (see [Colours](#colours)). |
| `x` | 0–255 (mode 1: 0–511) | The pixel column, from the left. The sign is ignored. |
| `y` | 0–175 (mode 1: 0–367) | The pixel row, from the bottom. The sign is ignored. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | The point is outside the screen (not counting the sign), or a colour is negative or greater than 255. |
| `K Invalid colour` | A colour out of its range. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 FOR x=0 TO 255 STEP 4: PLOT x, 88+40*SIN (x/20): NEXT x
```

**See also:** [DRAW](#draw), [POINT](#point), [BOX](#box),
[INVERSE, OVER](#inverse-over).

### DRAW

```
DRAW [colours;] dx, dy [, angle]
```

Draws a line from the last point to the point `dx` pixels to the right and
`dy` pixels up (negative values go left and down). With `angle` it draws an
arc instead, turning that much on the way: `PI` draws half a circle, and a
positive angle curves anticlockwise. The end point is the starting point of
the next `DRAW`.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `colours` | as in [`PLOT`](#plot) | Colours for this line only. |
| `dx`, `dy` | -255 to 255 (mode 1: -511 to 511) | How far it moves. The end point must be on the screen. |
| `angle` | any number, in radians; 0 if omitted | How much the arc turns. 0, or a full turn (`2*PI`), draws a straight line. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `dx` or `dy` is past the maximum, the line goes outside the screen (what has been drawn up to the edge stays), or a colour is negative or greater than 255. |
| `K Invalid colour` | A colour out of its range. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PLOT 50,50: DRAW 100,0: DRAW 0,100: DRAW -100,-100
20 PLOT 150,60: DRAW 60,0,PI
```

**See also:** [PLOT](#plot), [CIRCLE](#circle), [POLY](#poly).

### CIRCLE

```
CIRCLE [colours;] x, y, radius
```

Draws a circle with its centre at `x`, `y`. Afterwards, the next `DRAW`
starts from a point on its right edge.

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `colours` | as in [`PLOT`](#plot) | Colours for this circle only. |
| `x`, `y` | those of the screen | The centre. |
| `radius` | a number; the sign is ignored | In pixels. Below 1, it draws only the centre point. The whole circle must fit on the screen. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | The circle goes outside the screen (what has been drawn up to the edge stays), or a colour is negative or greater than 255. |
| `K Invalid colour` | A colour out of its range. |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 FOR r=10 TO 80 STEP 10: CIRCLE INK r/10; 128,88,r: NEXT r
```

**See also:** [DRAW](#draw), [DISC](#disc).

### POINT

```
POINT (x, y)
```

A function: tells whether a pixel of the drawing page is painted with the
ink. It sees what `PRINT`, `PLOT`, `DRAW` and `CIRCLE` draw, but not the
fast graphics commands (use [`PIXEL`](#pixel) for those).

**Parameters**

| Parameter | Values | Meaning |
|---|---|---|
| `x` | 0–255 (mode 1: 0–511) | The pixel column, from the left. The sign is ignored. |
| `y` | 0–175 (mode 1: 0–367) | The pixel row, from the bottom. The sign is ignored. |

**Returns:** 1 if the pixel is ink, 0 if it is paper.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | The point is outside the screen (not counting the sign). |
| (common) | See [Errors in any statement](#errors-in-any-statement). |

**Example**

```
10 PLOT 10,10: PRINT POINT (10,10); POINT (11,10)
```

shows `10`.

**See also:** [PIXEL](#pixel), [HIT](#hit), [SCREEN$, ATTR](#screen-attr).

## Video modes and screen effects

These commands change how the whole screen looks: its size, the page that
is shown, the scrolling and the colours, all without redrawing anything.

**Common errors.** Numbers are rounded to the nearest integer. A negative
one (except the lines of `RASTER`, which can be), or one greater than the
maximum the command accepts (255 in almost all; 65535 for sizes, scrolls
and the values of `RASTER`), gives
`B Integer out of range` before anything else, even if the command then
checks a narrower range with another report: `PAGE 2` gives `A`, but
`PAGE 300` gives `B`; `REMAP 20,1` gives `K`, but `REMAP 300,1` gives `B`.

### MODE

```
MODE n
MODE n, width, height
```

Chooses the screen mode and clears the screen:

| Mode | Pixels | Text |
|---|---|---|
| 0 | 256 × 192 | 32 columns × 24 rows (the one at start) |
| 1 | 512 × 384 | 64 columns × 48 rows |

`MODE` clears both pages, shows page 0 and draws on it (like `PAGE 0` and
`SCREEN 0`), and removes scrolling, `REMAP`, `PALETTE`, `RASTER` and `CLIP`.
The sprites, the tiles and the rest of the graphics store are kept, and so
is the `PATTERN` pattern.

In mode 0, the page can also be larger than the screen, so that the screen
is a window that you move over it with [`SCROLL`](#scroll):
`MODE 0, 512, 384`, `MODE 0, 1024, 192` or `MODE 0, 256, 768`. Then
`PRINT AT`, `PLOT` and the rest cover the whole page (`PRINT AT` reaches
column 127 on a 1024-pixel-wide page).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0 or 1 | The mode. |
| `width`, `height` | in mode 0: 256, 192 (the normal one), 512, 384, 1024, 192 or 256, 768; in mode 1: 512, 384 (the normal one) | The size of the page, in pixels. Optional; `0, 0` is the same as leaving them out. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `n` is between 2 and 255; or `width`, `height` is not one of the sizes of its mode (`MODE 1, 256, 192`, `MODE 0, 512, 192`...). |
| `B Integer out of range` | `n` is negative or greater than 255; `width` or `height`, negative or greater than 65535. |

**Example**

```
10 MODE 1: CIRCLE 256,184,150: PRINT AT 24,26; "Mode 1"
```

**See also:** [PAGE, SCREEN](#page-screen), [SCROLL](#scroll),
[CLS](#cls), [RASTER](#raster).

### PAGE, SCREEN

```
PAGE n
SCREEN n
```

ZX-V has two screen pages, 0 and 1. `PAGE` chooses the one that is shown;
`SCREEN` chooses the one where `PRINT`, `CLS`, `PLOT`, sprites and so on
draw. Drawing on the hidden page and then showing it gives smooth animation
without flicker (double buffering).

Error reports and `INPUT` always appear on the page being shown. The editor
also uses page 1: when a program uses page 1 again, the text and the `PLOT`
and `DRAW` drawings are restored, but the fast graphics (sprites, maps...)
drawn there are lost.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0 or 1 | The page. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `n` is between 2 and 255. |
| `B Integer out of range` | `n` is negative or greater than 255. |

**Example**

```
10 LET s=1
20 SCREEN s: CLS: CIRCLE 128,88,RND*80
30 PAGE s: LET s=1-s: WAIT 10: GO TO 20
```

**See also:** [MODE](#mode), [WAIT](#wait), [SCREEN$, ATTR](#screen-attr).

### SCROLL

```
SCROLL x, y
SCROLL
```

Moves the picture without redrawing it: pixel (`x`, `y`) of the page appears
at the top left corner of the screen, and the page wraps round at the edges.
`SCROLL` alone puts it back as it was (0, 0). This is how a game scrolls a
large [`MODE 0`](#mode) page. `MODE` also removes the scrolling.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y` | 0 to 65535 | The pixel of the page that goes to the corner. The remainder of dividing by the width or height of the page is used: on a 256-pixel page, `SCROLL 300,0` is the same as `SCROLL 44,0`. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `x` or `y` is negative or greater than 65535. |

**Example**

```
10 MODE 0,1024,192: CIRCLE 600,100,50
20 FOR x=0 TO 1023 STEP 2: SCROLL x,0: WAIT: NEXT x
```

**See also:** [MODE](#mode), [RASTER](#raster) (scrolling by lines).

### REMAP

```
REMAP colour, newcolour [; colour, newcolour]...
REMAP #table; colour, newcolour [; colour, newcolour]...
REMAP #table
REMAP
```

Shows one colour as another everywhere on the screen, instantly and without
redrawing: after `REMAP 1,10`, everything blue is shown bright red. This is
useful for fades, flashing and colour cycling.

There are three remap tables, 0 to 2. Without `#`, `REMAP` changes table 0,
which is the one normally in use. `REMAP #t; ...` changes table `t` without
showing it; `REMAP #t` makes the whole screen use table `t` (so all its
changes appear at once); [`RASTER REMAP`](#raster) uses a table for some
lines only. `REMAP` alone resets the three tables and goes back to table 0.
So does [`MODE`](#mode).

`REMAP` swaps one of the 16 colours for another; what each colour looks like
is set by [`PALETTE`](#palette).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `table` | 0–2 | The remap table. Without `#`, table 0. |
| `colour` | 0–15 | The colour that changes (the one in video memory). |
| `newcolour` | 0–15 | The colour it is shown as. `REMAP c, c` leaves it as it was. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `table` is between 3 and 255. |
| `K Invalid colour` | `colour` or `newcolour` is between 16 and 255. |
| `B Integer out of range` | `table`, `colour` or `newcolour` is negative or greater than 255. |

**Example**

```
10 PRINT INK 1; "blinking"
20 REMAP 1,10: PAUSE 25: REMAP 1,1: PAUSE 25: GO TO 20
```

**See also:** [PALETTE](#palette), [RASTER](#raster), [PIXEL](#pixel)
(which ignores `REMAP`).

### PALETTE

```
PALETTE colour, r, g, b [; colour, r, g, b]...
PALETTE #table; colour, r, g, b [; colour, r, g, b]...
PALETTE #table
PALETTE
```

Changes how one of the 16 colours looks: `r`, `g` and `b` are its red, green
and blue. Everything drawn in that colour changes instantly. After
`PALETTE 1, 255, 128, 0`, blue is shown orange.

The usual colours have 215 in each component (255 the bright ones): red is
`PALETTE 2, 215, 0, 0`. The screen has 65,536 possible colours, and the
nearest one is used.

Each [`REMAP`](#remap) table has its own palette. Without `#`, `PALETTE`
changes table 0's, the one normally in use; `PALETTE #t; ...` changes table
`t`'s. So, with [`RASTER REMAP`](#raster), each band of the screen can have
its own 16 colours. `PALETTE #t` alone gives table `t` back its usual
colours, and `PALETTE` alone does it for all three (without changing the
table in use). [`MODE`](#mode) also resets them.

Colour 8 is still transparent in sprites and in the map cells that ask for
it (it is not drawn), even if the palette gives it another colour.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `table` | 0–2 | The table whose palette changes. Without `#`, table 0. |
| `colour` | 0–15 | The colour that changes. |
| `r`, `g`, `b` | 0–255 | Its red, green and blue. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `table` is between 3 and 255. |
| `K Invalid colour` | `colour` is between 16 and 255. |
| `B Integer out of range` | `table`, `colour`, `r`, `g` or `b` is negative or greater than 255. |

**Example**

```
10 FOR i=0 TO 15: BOX i, i*16, 0, 16, 176: NEXT i
20 FOR v=0 TO 255 STEP 5: PALETTE 7, v, v, 255-v: WAIT: NEXT v
```

**See also:** [REMAP](#remap), [RASTER](#raster),
[colour table](#colours).

### RASTER

```
RASTER BORDER y1, y2, colour
RASTER REMAP y1, y2, table
RASTER SCROLL y1, y2, x [, y]
RASTER
```

Changes things line by line on the screen, without using any processor time
while the program runs:

- `RASTER BORDER`: the lines from `y1` to `y2` have a border of that colour.
- `RASTER REMAP`: those lines use that remap table (see [REMAP](#remap) and
  [PALETTE](#palette)).
- `RASTER SCROLL`: those lines have their own horizontal scroll `x`, and, if
  `y` is given, show the lines of the page from line `y` onwards (for split
  screens that scroll separately, and parallax). Without `y`, they show the
  same lines as without `RASTER`.
- `RASTER` alone turns all this off. So does `MODE`.

Each `RASTER` adds to the earlier ones: the lines that none of them has
touched look as they do without `RASTER`. `y1` and `y2` can be given in
either order.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `y1`, `y2` | −24 to 215 in mode 0; −48 to 431 in mode 1 | The lines, pixel rows of the screen counted from the top (0). Those of the top border are negative and those of the bottom border go past the height. They do not depend on the size of the page. |
| `colour` | 0–15 | The colour of the border. |
| `table` | 0–2 | The remap table. |
| `x` | 0 to 65535 | The horizontal scroll of those lines: the pixel of the page shown on the left (the remainder of dividing by the width of the page is used). |
| `y` | 0 to 65535 | The line of the page shown at `y1`; the following ones come after it (wrapping round at the height of the page). |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `y1` or `y2` outside the lines of the mode; `colour`, `table`, `x` or `y` negative or greater than 65535. |
| `K Invalid colour` | `colour` is between 16 and 65535. |
| `A Invalid argument` | `table` is between 3 and 65535. |
| `C Nonsense in BASIC` | `RASTER` is not followed by `BORDER`, `REMAP` or `SCROLL`. |

**Example**

```
10 FOR y=-24 TO 215 STEP 4: RASTER BORDER y,y+1,2: RASTER BORDER y+2,y+3,6: NEXT y
```

**See also:** [BORDER](#border), [REMAP](#remap), [SCROLL](#scroll),
[MODE](#mode).

## Sprites, tile maps and fast graphics

These commands use the ROM's graphics engine to draw games quickly: sprites
with transparency, scrolling tile maps, perspective floors and rotating
backgrounds ("mode 7") and filled shapes, plain or patterned. They differ
from `PLOT` and `DRAW` in a few ways:

- **Coordinates** are in pixels from the **top left** corner of the page,
  with `y` going down, like `PRINT AT`. They go from −32768 to 32767, and
  can be negative or off the page: whatever falls outside is simply not
  drawn.
- **Colours** are the 16 colours by number, 0 to 15 (see the
  [colour table](#colours)). In sprites, colour 8 is transparent; in maps,
  only in the cells that ask for it (see [MAP POKE](#map-dim-map-poke)).
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

| Store | Numbers |
|---|---|
| Sprites | 0–255 |
| Tiles | 0–4095 in each of the two sets (8 × 8 and 16 × 16) |
| Maps | 0–15 |
| Textures | 0–3 |

**Common errors.** Besides their own, all the commands of this section can
give:

| Report | When |
|---|---|
| `A Invalid argument` | The sprite, map or texture used does not exist, or the map is missing tiles (all the tiles its cells use must exist). |
| `B Integer out of range` | A number is out of its range: a sprite, map, texture or tile number; a coordinate outside −32768 to 32767; a colour that is negative or greater than 255. |
| `K Invalid colour` | A colour is between 16 and 255. |
| `4 Out of memory` | The sprite, tile set, map or texture does not fit in memory. |

### GRAB

```
GRAB n, x, y, width, height [, t]
GRAB n, width, a$
```

Defines sprite `n`.

- The first form copies a rectangle of the drawing page, with its top left
  corner at `x`, `y`. If `t` is given, that colour becomes transparent.
  Whatever falls outside the page is transparent too, and any colour 8 on
  the page is stored as 0 (both are black).
- The second form takes the pixels from a string, one character per pixel,
  row after row, `width` pixels per row: the hexadecimal digits `0`–`9` and
  `a`–`f` (or `A`–`F`) are colours 0 to 15, and any other character (`.`, a
  space...) is transparent, as is `8`. If the last row is short of
  characters, the missing ones are transparent.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–255 | The sprite. |
| `x`, `y` | −32768 to 32767 | The top left corner of the rectangle on the page. |
| `width`, `height` | 1–1024 | The size of the sprite, in pixels. With a string, the height is the number of rows that come out of it (up to 1024). |
| `t` | 0–15 | Optional: the colour that becomes transparent. |
| `a$` | 1 character or more | The pixels. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | With a string: `width` is less than 1 or greater than 1024, the string is empty or more than 1024 rows come out of it. |
| `B Integer out of range` | `n` outside 0–255; with a rectangle, `width` or `height` outside 1–1024; with a string, `width` outside −32768 to 32767. |
| (common) | See [Common errors](#sprites-tile-maps-and-fast-graphics). |

**Example**

```
10 GRAB 0,8,"..2222..22222222.222222...2222..."
20 FOR x=0 TO 240 STEP 16: SPRITE 0,x,80: NEXT x
```

**See also:** [SPRITE](#sprite), [HIT](#hit), [TILE](#tile).

### SPRITE

```
SPRITE n, x, y [, flip [, c$ [, width, height]]]
```

Draws sprite `n` with its top left corner at `x`, `y`. Its transparent pixels
let the background show through.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–255 | The sprite, defined with [GRAB](#grab). |
| `x`, `y` | −32768 to 32767 | Where its top left corner goes. |
| `flip` | 0–3 | 0 normal (the default), 1 mirrored left to right, 2 upside down, 3 both. |
| `c$` | string | Changes colours: colour `i` is drawn as the colour given by character `i` of the string (counting from 0), a hexadecimal digit. A character that is not a hexadecimal digit (or a string too short) leaves that colour as it is: `"0a"` draws colour 1 as 10 and leaves the rest. `""` changes nothing. |
| `width`, `height` | 1–4096 | Draws it stretched or shrunk to that size, for things that come closer or move away. Without them, at its own size. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `n` outside 0–255, `flip` outside 0–3, or `width` or `height` outside 1–4096. |
| `C Nonsense in BASIC` | `c$` is not a string. |
| (common) | See [Common errors](#sprites-tile-maps-and-fast-graphics); above all, `A` if the sprite does not exist. |

**Example**

```
10 GRAB 0,4,"1111122112211111"
20 SPRITE 0,10,10: SPRITE 0,30,10,0,"0a": SPRITE 0,50,10,0,"",32,32
```

**See also:** [GRAB](#grab), [HIT](#hit), [CLIP](#clip).

### HIT

```
HIT(a, ax, ay, b, bx, by)
```

A function: tests whether two sprites collide, without drawing anything. It
checks whether any solid (not transparent) pixel of sprite `a` drawn at
`ax`, `ay` would land on a solid pixel of sprite `b` drawn at `bx`, `by`. It
compares the sprites as they are, without flipping or resizing.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `a`, `b` | 0–255 | The two sprites (they can be the same one). |
| `ax`, `ay`, `bx`, `by` | −32768 to 32767 | Where the top left corner of each would be. |

**Returns:** 1 if they touch, 0 if not.

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `a` or `b` is not defined. |
| `B Integer out of range` | A sprite outside 0–255 or a coordinate outside −32768 to 32767. |

**Example**

```
10 GRAB 0,4,"1111111111111111"
20 PRINT HIT(0,0,0,0,3,3); HIT(0,0,0,0,4,0)
```

shows `10`.

**See also:** [SPRITE](#sprite), [PIXEL](#pixel), [CELL](#cell).

### TILE

```
TILE size, n, x, y [, width, height [, t]]
TILE size, n, a$
```

Defines tiles for tile maps. There are two tile sets, one of 8 × 8 pixel
tiles and one of 16 × 16, each with tiles numbered 0 to 4095.

- The first form copies tile `n` from the drawing page at `x`, `y`. With
  `width` and `height`, it cuts that rectangle into tiles (those that fit
  whole), numbered from `n` onwards row by row. Colour `t`, if given, and
  whatever falls outside the page become colour 8, which is transparent in
  the cells that ask for it (see [MAP POKE](#map-dim-map-poke)); any colour
  8 on the page is stored as 0.
- The second form takes the pixels from a string as in [`GRAB`](#grab)
  (size × size characters per tile; a longer string defines the next tiles,
  and whatever the last one is short of is colour 8).

A tile set grows when you define a higher tile; the new tiles start black.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `size` | 8 or 16 | The tile set. |
| `n` | 0–4095 | The first tile defined. |
| `x`, `y` | −32768 to 32767 | The top left corner on the page. |
| `width`, `height` | 1–32767 | Optional: the rectangle that is cut into tiles. Without them, one tile (`size` × `size`). |
| `t` | 0–15 | Optional: the colour that becomes transparent. |
| `a$` | string | The pixels. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `size` is between 0 and 255 but is neither 8 nor 16; no tile comes out (the rectangle is smaller than a tile, or the string is empty). |
| `B Integer out of range` | `size` is negative or greater than 255; `n` outside 0–4095; `width` or `height` outside 1–32767; the tiles would go past 4095. |
| (common) | See [Common errors](#sprites-tile-maps-and-fast-graphics). |

**Example**

```
10 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,1,0,0
```

**See also:** [MAP DIM, MAP POKE](#map-dim-map-poke), [MAP](#map),
[GRAB](#grab).

### MAP DIM, MAP POKE

```
MAP DIM m, width, height [, size [, wrap]]
MAP POKE m, x, y, value
MAP POKE m, x, y, a$
```

`MAP DIM` creates tile map `m` with `width` × `height` cells, all set to
tile 0. With `wrap` = 1 the map repeats endlessly in both directions. It
takes 2 bytes per cell.

`MAP POKE` sets the cell at column `x`, row `y` (counting from 0). The value
is the tile number plus any of these:

| Add | Effect |
|---|---|
| 4096 | tile mirrored left to right |
| 8192 | tile upside down |
| 16384 | colour 8 of the tile is transparent (lets what is behind show); without this, colour 8 is drawn as just another colour (black, with the usual palette) |
| 32768 | priority (see the filter of [MAP](#map)) |

With a string, it fills the cells from `x` to the right, using the code of
each character as the tile number: `"  ##.."` uses tiles 32, 35 and 46. This
makes it easy to draw levels with text. Whatever goes past the right edge
of the map is not set.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `m` | 0–15 | The map. |
| `width`, `height` | 1–32767 | The size of the map, in cells. |
| `size` | 8 or 16 | The tile set it uses. The default is 8. |
| `wrap` | 0 or 1 | 1: it repeats endlessly. The default is 0. |
| `x`, `y` | 0 to `width` − 1, 0 to `height` − 1 | The cell. |
| `value` | 0–65535 | The tile (0–4095) plus the additions of the table. |
| `a$` | string | One tile per character, by its code (0–255). |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `MAP DIM`: `size` is between 0 and 255 but is neither 8 nor 16. `MAP POKE`: the map does not exist. |
| `B Integer out of range` | `m` outside 0–15; `width` or `height` outside 1–32767; `size` negative or greater than 255; `wrap` that is neither 0 nor 1; `x` or `y` outside the map; `value` outside 0–65535. |
| `4 Out of memory` | The map does not fit. |

**Example**

```
10 BOX 5,0,0,8,8: TILE 8,35,0,0
20 MAP DIM 0,32,24: MAP POKE 0,0,12,"################################"
30 MAP POKE 0,5,11,35+8192: MAP 0
```

**See also:** [MAP](#map), [CELL](#cell), [TILE](#tile).

### MAP

```
MAP m [, mx, my [, x, y, width, height [, filter]]]
```

Draws map `m` starting at its pixel `mx`, `my`, on the whole page or in the
rectangle `x`, `y`, `width`, `height`. Changing `mx` and `my` scrolls the
map. Outside a map that does not wrap, nothing is drawn (whatever was there
stays). `MAP` is also used after [`FLOOR`](#floor) and [`ROTATE`](#rotate).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `m` | 0–15 | The map. |
| `mx`, `my` | −32768 to 32767 | The pixel of the map that goes in the top left corner. The default is 0, 0. |
| `x`, `y` | −32768 to 32767 | The top left corner of the rectangle on the page. The default is 0, 0. |
| `width`, `height` | 0–32767 | The size of the rectangle. The default is the whole page. |
| `filter` | 0–2 | 0 (the default) draws all cells, 1 only those without priority and 2 only those with priority: so you can draw the background, then the sprites and last the parts of the scenery in front. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | The map does not exist, or one of the tiles its cells use is missing. |
| `B Integer out of range` | `m` outside 0–15, `width` or `height` outside 0–32767, `filter` outside 0–2, or a coordinate outside −32768 to 32767. |

**Example**

```
10 BOX 5,0,0,8,8: TILE 8,32,0,0
20 BOX 2,0,0,8,8: POLY 7,0,3,7,3: TILE 8,35,0,0
30 MAP DIM 0,40,1: MAP POKE 0,0,0,"####  ####   ######  #########   ####"
40 FOR x=0 TO 64: MAP 0,x,0,0,100,256,8: WAIT: NEXT x
```

**See also:** [MAP DIM, MAP POKE](#map-dim-map-poke), [TILE](#tile),
[CELL](#cell), [CLIP](#clip).

### CELL

```
CELL(m, x, y)
```

A function: the value of the cell at column `x`, row `y` of map `m`,
including the mirror, transparency and priority bits. Useful to check what
the scenery has at some place: the cell under a sprite at pixel `px`, `py`
is `CELL(m, INT (px/8), INT (py/8))` for 8-pixel tiles. The tiles need not
exist.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `m` | 0–15 | The map. |
| `x`, `y` | −32768 to 32767 | The column and the row. In a map that wraps, they wrap round. |

**Returns:** the value of the cell (0–65535: the tile plus the additions of
[MAP POKE](#map-dim-map-poke)); 0 outside a map that does not wrap.

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | The map does not exist. |
| `B Integer out of range` | `m` outside 0–15, or `x` or `y` outside −32768 to 32767. |

**Example**

```
10 MAP DIM 0,10,10: MAP POKE 0,3,4,7+8192
20 PRINT CELL(0,3,4); " "; CELL(0,20,20)
```

shows `8199 0`.

**See also:** [MAP DIM, MAP POKE](#map-dim-map-poke), [HIT](#hit),
[PIXEL](#pixel).

### TEXTURE

```
TEXTURE n, x, y, size [, c [, t]]
```

Defines texture `n` for [`FLOOR`](#floor) and [`ROTATE`](#rotate): the
square of `size` × `size` pixels of the drawing page with its top left
corner at `x`, `y`. Whatever falls outside the page is black (colour 0).
The texture repeats for ever; if colour `c` is given, everything outside it
is that colour instead (with `c` = −1 it repeats, as without `c`).

If `t` is given, that colour becomes **transparent**: wherever the texture
has it, what was underneath shows through. With `c` the same as `t`, the
outside is not drawn either. With `t`, any colour 8 on the page is stored
as 0.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–3 | The texture. |
| `x`, `y` | −32768 to 32767 | The top left corner of the square on the page. |
| `size` | 2, 4, 8, 16, 32, 64, 128 or 256 | The side, a power of 2. It takes `size` × `size` bytes (64 KB with 256). |
| `c` | −1 or 0–15 | Optional: the outside colour; −1, it repeats. |
| `t` | 0–15 | Optional: the transparent colour. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `size` is between 0 and 256 but is not a power of 2 from 2 upwards (0, 1, 3, 100...). |
| `B Integer out of range` | `n` outside 0–3; `size` negative or greater than 256 (`TEXTURE 0,0,0,512` gives `B`); `c` less than −1 or greater than 255. |
| `K Invalid colour` | `c` is between 16 and 255. |
| (common) | See [Common errors](#sprites-tile-maps-and-fast-graphics) (for `t` and memory). |

**Example**

```
10 PAPER 0: CLS: DISC 6,16,16,15: DISC 2,16,16,8: TEXTURE 1,0,0,32,0,0
```

defines a texture with a sun on black, and the black (and the outside) is
not drawn.

**See also:** [FLOOR](#floor), [ROTATE](#rotate).

### FLOOR

```
FLOOR n, x, y, height, angle, horizon [, focal]
FLOOR MAP m, x, y, height, angle, horizon [, focal]
```

Draws a floor in perspective with texture `n` or, with `MAP`, with tile map
`m` (as the SNES did: a whole race track fits in a map), as seen from point
`x`, `y` of the texture or map at that `height`, looking in direction
`angle`. The floor fills from the line below the horizon to the bottom of
the page (or as much as [CLIP](#clip) allows). This is the "mode 7" effect
of racing and flying games.

With a map, each cell keeps its mirroring and transparency (see [MAP
POKE](#map-dim-map-poke)); a map that wraps also wraps on the floor, and
outside one that does not wrap nothing is drawn. Drawing with a map costs
more than with a texture: a half-screen floor takes a third of a frame in
mode 0. `MAP` goes right after `FLOOR`; anywhere else it is a variable name.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–3 (texture) or 0–15 (with `MAP`) | The texture or the map. |
| `x`, `y` | greater than −32768 and less than 32768; can have decimals | The point of the texture or map, in pixels, above which the viewer is. |
| `height` | greater than 0 and less than 32768 | The height the viewer looks from. |
| `angle` | any, in radians | The direction: 0 looks towards growing `x`. |
| `horizon` | −32768 to 32767 | The line of the page where the horizon is. |
| `focal` | greater than 0 and less than 32768 | The focal distance: smaller values give a wider view. The default is half the width of the page. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | The texture or the map does not exist, or the map is missing a tile. |
| `B Integer out of range` | `n` out of its range; `height` or `focal` is 0 or negative; `x`, `y`, `height` or `focal` reaches 32768 (or −32768); `horizon` outside −32768 to 32767. |

**Example**

```
10 FOR i=0 TO 7: BOX i,i*8,0,8,64: NEXT i: TEXTURE 0,0,0,64
20 FOR a=0 TO 2*PI STEP .05: FLOOR 0,32,32,20,a,80: WAIT: NEXT a
```

A race track with a map of 32 × 32 cells: a black square track, with red
and white kerbs (a pattern) and grass around it:

```
10 BOX 4,0,0,8,8: POLY 12,2,2: POLY 12,6,5: TILE 8,46,0,0
20 BOX 0,0,0,8,8: TILE 8,35,0,0
30 PATTERN 1,15: BOX 2,0,0,8,8: PATTERN: TILE 8,61,0,0
40 MAP DIM 0,32,32,8,1
50 FOR r=0 TO 31: FOR c=0 TO 31: LET a=ABS (c-15.5): LET b=ABS (r-15.5)
60 LET m=a: IF b>a THEN LET m=b
70 LET t=46: IF m>=8 AND m<12 THEN LET t=35
80 IF (m>=7 AND m<8) OR (m>=12 AND m<13) THEN LET t=61
90 MAP POKE 0,c,r,t: NEXT c: NEXT r
100 BOX 5,0,0,256,96: FLOOR MAP 0,204,150,10,1.5*PI,95
```

**See also:** [TEXTURE](#texture), [ROTATE](#rotate), [MAP](#map),
[CLIP](#clip).

### ROTATE

```
ROTATE n, x, y [, angle [, zoom [, cx, cy]]]
ROTATE MAP m, x, y [, angle [, zoom [, cx, cy]]]
```

Draws texture `n` or, with `MAP`, map `m`, **rotated and scaled** over the
whole page (or in the rectangle of [CLIP](#clip)), like the rotating
backgrounds and bosses of the SNES. Point `x`, `y` of the texture or map
appears at point `cx`, `cy` of the page, rotated `angle` radians clockwise
and scaled `zoom` times.

The texture repeats if it has no outside colour. With a texture with a
transparent colour (see [TEXTURE](#texture)) or a map that does not wrap,
what is not drawn lets what is underneath show through. Without rotating or
scaling, `ROTATE` scrolls the texture or map: `ROTATE 0, x, y` puts its
point `x`, `y` in the centre.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–3 (texture) or 0–15 (with `MAP`) | The texture or the map. |
| `x`, `y` | greater than −32768 and less than 32768; can have decimals | The point of the texture or map that goes at `cx`, `cy`. |
| `angle` | any, in radians | The rotation, clockwise. The default is 0. |
| `zoom` | greater than 0 and less than 32768 | How many times it is scaled: 1 (the default), one pixel per pixel; 2, twice as big; 0.5, half. |
| `cx`, `cy` | −32768 to 32767 | Where point `x`, `y` appears on the page. The default is its centre. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | The texture or the map does not exist, or the map is missing a tile. |
| `B Integer out of range` | `n` out of its range; `zoom` is 0 or negative (or so small that it does not reach 1/65536); `x`, `y` or `zoom` reaches 32768 (or −32768); `cx` or `cy` outside −32768 to 32767. |

**Example**

```
10 PAPER 0: CLS: DISC 6,16,16,15: DISC 2,16,16,8: BOX 1,12,4,8,24: TEXTURE 1,0,0,32,0,0
20 LET s=1: FOR a=0 TO 2*PI STEP .05
30 SCREEN s: BOX 5,0,0,256,192: ROTATE 1,16,16,a,2
40 PAGE s: LET s=1-s: WAIT: NEXT a
```

turns a sun over the sky. As each turn clears the page and draws it again,
it does so on the hidden page and then shows it (see
[PAGE, SCREEN](#page-screen)): on the page being shown, the machine could
send it to the screen half drawn and the sun would flicker.

**See also:** [TEXTURE](#texture), [FLOOR](#floor), [MAP](#map),
[CLIP](#clip).

### PATTERN

```
PATTERN n [, paper]
PATTERN b1, b2, b3, b4, b5, b6, b7, b8 [, paper]
PATTERN
```

Chooses a **pattern** for [POLY](#poly), [BOX](#box) and [DISC](#disc):
instead of a plain colour, they draw their colour where the pattern has a 1
and `paper` where it has a 0. Without `paper`, nothing is drawn where the
pattern has a 0 and what is underneath shows through: this gives
"half-there" shadows and glass.

- `PATTERN n` uses one of the built-in patterns (1 to 11):

  | n | Pattern | n | Pattern |
  |---|---|---|---|
  | 1 | Checkerboard (half) | 7 | Rising diagonal |
  | 2 | One quarter | 8 | Falling diagonal |
  | 3 | One eighth | 9 | Grid |
  | 4 | Three quarters | 10 | Bricks |
  | 5 | Horizontal stripes | 11 | Diamonds |
  | 6 | Vertical stripes | | |

- `PATTERN b1, ..., b8` defines one of 8 × 8 pixels: one byte per row, from
  top to bottom, with the highest bit for the leftmost pixel, like
  [UDGs](#user-defined-graphics-udg): `PATTERN BIN 11110000, ...`.
- `PATTERN` alone, or `PATTERN 0`, goes back to plain colour. So do `RUN`
  and `CLEAR` (`MODE` does not).

The pattern is fixed to the page, not to the shape: two shapes that touch
match up. Drawing with a pattern is just as fast.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–11 | One of the patterns of the table; 0, none. |
| `b1` ... `b8` | 0–255 | The eight rows of a pattern of your own. |
| `paper` | 0–15 | Optional: the colour of the pattern's zeros. Without it, they are not drawn. |

With 1, 2, 8 or 9 numbers: `PATTERN n`, `PATTERN n, paper`, the eight rows,
or the eight rows and the paper.

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | There are 3 to 7 numbers, or more than 9. It shows as soon as the line is typed. |
| `B Integer out of range` | `n` is between 12 and 255; any number is negative or greater than 255. |
| `K Invalid colour` | `paper` is between 16 and 255. |

**Example**

```
10 PATTERN 10,2: BOX 15,32,96,192,96
20 PATTERN 1: DISC 0,128,96,40: PATTERN
```

draws a wall of red bricks with white mortar and, on it, a round shadow
that lets the wall show through by half.

**See also:** [POLY](#poly), [BOX](#box), [DISC](#disc), [BIN](#bin).

### POLY

```
POLY colour, x1, y1 [, x2, y2]...
```

With one point, draws a pixel; with two, a line; with three or more, a
filled polygon. It uses the [PATTERN](#pattern) pattern, if there is one.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `colour` | 0–15 | The colour. |
| `x1`, `y1`... | −32768 to 32767 | The points, 1 to 64. |

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | There is no point (`POLY 4`). It shows as soon as the line is typed. |
| `A Invalid argument` | There are more than 64 points. It shows as soon as the line is typed. |
| `B Integer out of range` | A coordinate outside −32768 to 32767; `colour` negative or greater than 255. |
| `K Invalid colour` | `colour` is between 16 and 255. |

**Example**

```
10 POLY 4, 128,20, 200,150, 56,150
```

**See also:** [BOX](#box), [DISC](#disc), [PATTERN](#pattern),
[DRAW](#draw).

### BOX

```
BOX colour, x, y, width, height
```

A filled rectangle with its top left corner at `x`, `y`. It uses the
[PATTERN](#pattern) pattern, if there is one. With `width` or `height` 0 it
draws nothing.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `colour` | 0–15 | The colour. |
| `x`, `y` | −32768 to 32767 | The top left corner. |
| `width`, `height` | 0–32767 | The size, in pixels. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `width` or `height` negative or greater than 32767; a coordinate outside −32768 to 32767; `colour` negative or greater than 255. |
| `K Invalid colour` | `colour` is between 16 and 255. |

**Example**

```
10 BOX 2, 10,10, 100,50
```

**See also:** [DISC](#disc), [POLY](#poly), [PATTERN](#pattern),
[CLIP](#clip).

### DISC

```
DISC colour, x, y, radius
```

A filled circle with its centre at `x`, `y`. It uses the
[PATTERN](#pattern) pattern, if there is one. With `radius` 0, one pixel.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `colour` | 0–15 | The colour. |
| `x`, `y` | −32768 to 32767 | The centre. |
| `radius` | 0–32767 | The radius, in pixels. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `radius` negative or greater than 32767; a coordinate outside −32768 to 32767; `colour` negative or greater than 255. |
| `K Invalid colour` | `colour` is between 16 and 255. |

**Example**

```
10 DISC 6, 128,96, 40
```

**See also:** [BOX](#box), [POLY](#poly), [CIRCLE](#circle),
[PATTERN](#pattern).

### CLIP

```
CLIP x, y, width, height
CLIP
```

From now on, the commands of this section only draw inside that rectangle.
`CLIP` alone removes the limit (as do `MODE`, `RUN` and `CLEAR`).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y` | −32768 to 32767 | The top left corner of the rectangle. |
| `width`, `height` | 0–32767 | Its size, in pixels. With 0, nothing is drawn. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `width` or `height` negative or greater than 32767; `x` or `y` outside −32768 to 32767. |

**Example**

```
10 CLIP 50,50,100,50: DISC 2,100,75,60: CLIP
```

**See also:** [MAP](#map), [FLOOR](#floor), [ROTATE](#rotate),
[SPRITE](#sprite).

### PIXEL

```
PIXEL(x, y)
```

A function: the colour of a pixel of the drawing page, whatever drew it.
Useful for collisions with anything on the screen.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `x`, `y` | −32768 to 32767 | The pixel, counting from the top left of the page. |

**Returns:** the colour (0–15) as it is in video memory, without applying
`REMAP`; −1 if the pixel falls outside the page.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `x` or `y` outside −32768 to 32767. |

**Example**

```
10 BOX 4,10,10,20,20
20 PRINT PIXEL(15,15); " "; PIXEL(5,5); " "; PIXEL(-1,0)
```

**See also:** [HIT](#hit), [POINT](#point), [CELL](#cell).

## Sound and music

ZX-V has a beeper (a simple speaker, used by `BEEP`), two AY sound chips
with three channels each (used by `PLAY`, `SOUND`, `MUSIC` and `SFX`) and
twelve sample channels, which play recorded sounds (those of `SAMPLE` and
those of sample music).

**Common errors.** `LOAD name$ MUSIC`, `LOAD name$ SFX` and
`LOAD name$ SAMPLE n` can give, besides their own:

| Report | When |
|---|---|
| `F Invalid file name` | The name is empty, too long or has characters that are not allowed. |
| `T File not found` | The file does not exist (without an extension, none of the ones looked for). |
| `U No SD card` | The file is on `SD:` and there is no card. |
| `V Disk error` | The card, the `EXT:` drive or the file failed when reading. |
| `O Invalid stream` | There are already 8 files open on that drive. |
| `R Loading error` | The file is not in a format it understands, or it is damaged. |
| `4 Out of memory` | It does not fit in the BASIC's memory or in the sound memory. |
| `C Nonsense in BASIC` | With `SAVE`, `VERIFY` or `MERGE` instead of `LOAD`. |

### BEEP

```
BEEP duration, pitch
```

Plays a note on the beeper, a square wave. The program waits until the note
ends; BREAK cuts it short.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `duration` | 0 to 10 | In seconds; can have decimals. |
| `pitch` | −60 to 69.99... | In semitones above middle C: 0 is middle C, 12 the C above and −12 the C below. Can have decimals. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `duration` is negative or greater than 10, or `pitch` is less than −60 or reaches 70. |
| `L BREAK into program` | BREAK was pressed. |

**Example**

```
10 FOR n=0 TO 12: BEEP 0.1,n: NEXT n
```

**See also:** [PLAY](#play), [SOUND](#sound).

### PLAY

```
PLAY a$ [, b$]...
```

Plays music written as text, with up to six voices at once: the first three
strings play on channels A, B and C of the first AY chip, and the next three
on those of the second. Up to eight strings can be given, as on the 128K
(there the seventh and eighth were for MIDI): the seventh and eighth are
checked, but do not sound.

The program waits until all of them have finished; BREAK stops them. All
the strings are checked before any sound starts. `PLAY` stops the
[`MUSIC`](#music) music, which uses the chips its own way; the `SFX` effects
carry on.

Inside each string (the commands in upper or lower case):

| Text | Meaning |
|---|---|
| `c d e f g a b` | a note (C, D, E, F, G, A, B); upper case `C`...`B` is one octave higher |
| `#`, `$` | sharp or flat (before the note): `#f`, `$b` |
| `&` | a rest (silence) |
| `1` to `9` | length of the following notes: 1 semiquaver (sixteenth note), 2 dotted semiquaver, 3 quaver (eighth note), 4 dotted quaver, 5 crotchet (quarter note, the default), 6 dotted crotchet, 7 minim (half note), 8 dotted minim, 9 semibreve (whole note) |
| `10` `11` `12` | triplets of semiquavers, quavers and crotchets: they apply to the next three notes |
| `n_m` | tied lengths: the next note lasts both (more can be tied: `5_5_3`) |
| `O n` | octave, 0 to 8 (default 5, where `c` is middle C) |
| `T n` | tempo in crotchets per minute, 60 to 240 (default 120); only counts in the first string |
| `V n` | volume, 0 to 15 (default 15); stops using the envelope |
| `U` | use the volume envelope |
| `W n` | envelope shape, 0 to 7 |
| `X n` | envelope period, 0 to 65535 |
| `M n` | mixer, 0 to 63: bits 0–2 turn on the tone of channels A, B and C, bits 3–5 their noise |
| `N` | separator (does nothing) |
| `(` `)` | repeat what is inside once (up to 4 levels); a `)` without its `(` repeats the whole string for ever (until BREAK) |
| `!...!` | a comment |
| `H` | stop all voices and end the `PLAY` |
| `Y n`, `Z n` | MIDI on the 128K: checked (`Y` up to 16, `Z` up to 255) and ignored |

Spaces are ignored. The envelope and mixer belong to each chip, so they are
shared by its three voices. Each `PLAY` starts with the tempo at 120 and the
mixer with the tone of its voices on and the noise off.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `a$`, `b$`... | 1 to 8 strings | The voices: 1st–3rd on the first AY, 4th–6th on the second, 7th–8th do not sound. |

**Errors**

| Report | When |
|---|---|
| `Q Parameter error` | There are more than 8 strings. It shows as soon as the line is typed. |
| `C Nonsense in BASIC` | An argument is not a string. |
| `A Invalid argument` | A string has a character that is not in the table; a command (`O`, `T`, `V`, `W`, `X`, `M`, `Y`, `Z`, or a `_`) without its number; more than 4 nested `(`; or a `(` without its `)`. |
| `B Integer out of range` | A number goes past the maximum of its command (`O` 8, `T` 240, `V` 15, `W` 7, `M` 63, `Y` 16, `Z` 255), or past 65535; `T` less than 60; a length of 0 or greater than 12. |
| `4 Out of memory` | The strings do not fit in memory. |
| `L BREAK into program` | BREAK was pressed. |

**Example**

```
10 PLAY "T140 O4 5cdec 5cdec 5efg&"
20 PLAY "O4 3cegC", "O3 9c", "O5 V10 3(gfed)"
```

**See also:** [BEEP](#beep), [SOUND](#sound), [MUSIC](#music).

### SOUND

```
SOUND register, value [; register, value]...
```

Writes directly to the registers of the AY chips and of the sample
channels, for sound effects. Registers 0 to 15 belong to the first chip and
16 to 31 to the second (same meaning, 16 higher). What `SOUND` writes on
the music's chip lasts until the music writes that register again.

| Register | Use |
|---|---|
| 0–1, 2–3, 4–5 | tone period of channels A, B and C (low byte, high byte 0–15); the frequency is 110 840 / period Hz |
| 6 | noise period, 0–31 |
| 7 | mixer: a bit at **0** turns on: bits 0–2 tone A, B, C; bits 3–5 noise A, B, C |
| 8, 9, 10 | volume of A, B and C, 0–15; 16 uses the envelope |
| 11–12 | envelope period (low, high) |
| 13 | envelope shape (writing it restarts the envelope) |

Registers 64 to 159 are those of the twelve sample channels, eight per
channel: channel `c` (0–11) starts at 64 + 8 × `c`. To play recorded sounds
[`SAMPLE`](#sample) is easier; the details are in the sample channels of
the [SDK reference](../sdk/reference.md).

| Register | Use |
|---|---|
| 64 + 8 × `c` | the sample: an entry of the sample table, 0–255 (the `SAMPLE` ones are 224 to 255) |
| + 1 | the volume, 0–64 |
| + 2 | the pan: 0 left, 128 centre, 255 right |
| + 3 | the control: 1 starts it, 2 stops it (4, without interpolation; 8, direct mode) |
| + 4, + 5, + 6 | the step, from low byte to high (8.16: 65,536 is 48 kHz) |
| + 7 | where it starts, in steps of 256 samples |

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `register` | 0–31 and 64–159 | The register. |
| `value` | 0–255 | What is written. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `register` is between 32 and 63 or past 159; or `register` or `value` is negative or greater than 255. |

**Example**

```
10 SOUND 7,62; 8,15; 0,200; 1,0
20 PAUSE 25: SOUND 8,0
```

plays a tone on channel A for half a second.

**See also:** [PLAY](#play), [SAMPLE](#sample), [`IN` and `OUT`](#in-out),
for the 128K ports.

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
while it plays, as in games.

`LOAD name$ MUSIC` loads a module. Without an extension, the first there is
of `name.zxm`, `name.pt3`, `name.mod`, `name.s3m`, `name.xm`, `name.vgm` and
`name.vgz`; if there is none, the error is the one for not finding
`name.pt3`. It can be:

- ZX-V's own format (`.zxm`, with both AY chips and the sample channels:
  the one
  [`zxv-tracker`](../sdk/README.md#the-tracker-for-the-pc-zxv-tracker) or
  [`zxv-music`](../sdk/README.md#the-own-format-zxm-and-zxv-music) saves);
- a MOD, an S3M (Scream Tracker 3) or an XM (FastTracker 2), which it
  converts when loading and which play on the sample channels (up to 12: of
  those with more, the 12 that play the most notes);
- Pro Tracker 3 or Vortex Tracker II (`.pt3`, which `zxv-tracker` also
  saves), a TurboSound (two modules, one on each AY chip: six voices), Pro
  Tracker 2, Sound Tracker, ASC Sound Master or a PSG dump;
- a VGM (`.vgm`, or compressed, `.vgz`: the registers of the AY chips, as
  emulators record them; see [VGM](../sdk/README.md#vgm-the-ay-chips)).

The samples of a MOD, an S3M, an XM or a `.zxm` go from the file to the
sound memory (96 KB) when it is loaded, and only the song stays in the
BASIC's memory (repeated patterns, once): the file can be bigger than that
memory (a 300 KB MOD or a 1.6 MB XM fit, for example). With more than
96 KB of samples, the biggest ones go in ADPCM, and if they still do not
fit, also at half the rate (with less treble): modules with up to about
96 KB of samples sound best, and almost the same up to about 170 KB.

- `MUSIC PLAY` plays it from the start and, at the end, goes back to the
  module's loop position, for ever.
- `MUSIC STOP` stops it; `MUSIC PAUSE` silences it and `MUSIC CONTINUE`
  carries on where it was.
- The `MUSIC` function is 1 while it plays and 0 if not (also when paused).

A `.zxm` with sample tracks plays them on the sample channels from 0 on (one
per track; the rest stay free), and when it starts it uploads its samples to
the beginning of the sound memory and of its table.

An AY module plays on the first chip (a TurboSound or a VGM for two AY
chips, on both); a MOD, an S3M or an XM, on the sample channels; and a
`.zxm`, on the chips and sample channels its tracks use. It is played by the
same code as the tracker and C programs. It stops with `MUSIC STOP`,
BREAK, `RUN`, `NEW`, [`PLAY`](#play) (which uses the chips its own way),
when another module is loaded and when a native program is loaded; if the
program ends or gives an error, it keeps playing. What `SOUND` writes on the
music's chip lasts until the music writes that register again.

In the BASIC's memory, an AY module or a VGM takes the size of its file (2
to 15 KB for a normal PT3; a `.vgz` also takes 32 KB to decompress it while
it plays), and a MOD, an S3M, an XM or a `.zxm` only its song: its samples
go to the sound memory. The module goes with the graphics: `RUN` and `CLEAR`
keep it and `NEW` deletes it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | file name | The module, with or without an extension. |

**Returns** (the `MUSIC` function): 1 while the music plays; 0 if it is
stopped, paused or there is none.

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `MUSIC PLAY` without a module loaded. |
| `C Nonsense in BASIC` | `MUSIC` as a command without `PLAY`, `STOP`, `PAUSE` or `CONTINUE`; `SAVE`, `VERIFY` or `MERGE` with `MUSIC`. |
| `R Loading error` | The file is in none of those formats, or it is damaged. |
| `4 Out of memory` | The song does not fit in the BASIC's memory; the samples of a `.zxm` do not fit in the sound memory left by the `SAMPLE` ones; or the song has more than 224 samples and there are `SAMPLE` samples loaded. |
| (common) | See [Common errors](#sound-and-music). |

**Example**

```
10 LOAD "tune" MUSIC: MUSIC PLAY
20 PRINT "Playing..."
30 IF INKEY$="" THEN GO TO 30
40 MUSIC STOP
```

**See also:** [SFX](#sfx), [SAMPLE](#sample), [PLAY](#play),
[LOAD](#load).

### SFX

```
LOAD name$ SFX
SFX n [, channel]
SFX STOP
SFX                            (function)
```

Sound effects as in games, on the AY chips: an AYFX effect bank (`.afb`, the
one the effects tab of `zxv-tracker` or the AY Sound FX Editor saves;
without an extension, `name.afb`) or a single effect (`.afx`, with its
extension), which is taken as a bank of one effect.

- `SFX n` plays effect `n` of the bank (the first is 0) without waiting, on
  a free channel of the chip the music does not use (if there is none, on
  the one that has been playing longest). With `channel`, on that one; an
  effect on a music channel covers it while it plays.
- `SFX STOP` stops all the effects.
- The `SFX` function is 1 while any effect plays.

They play at the same time as the music, `BEEP` and everything else; `PLAY`
does not stop them. BREAK, `RUN` and `NEW` stop them; the bank is kept like
the `MUSIC` module (with `RUN` and `CLEAR`; `NEW` deletes it). Loading
another bank replaces it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | file name | The bank (`.afb`) or the effect (`.afx`). |
| `n` | 0 to the number of effects minus 1 (at most 255) | The effect. |
| `channel` | 0–5 | Optional: 0 to 2, channels A, B and C of the first chip; 3 to 5, those of the second. |

**Returns** (the `SFX` function): 1 if any effect plays; 0 if not.

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `SFX n` without a bank loaded. |
| `B Integer out of range` | Effect `n` is not in the bank; `n` negative or greater than 255; `channel` negative or greater than 5. |
| `C Nonsense in BASIC` | `SFX PLAY`, `SFX PAUSE` or `SFX CONTINUE`; `SAVE`, `VERIFY` or `MERGE` with `SFX`. |
| `R Loading error` | The file is not an effect bank. |
| (common) | See [Common errors](#sound-and-music). |

**Example**

```
10 LOAD "effects" SFX
20 IF INKEY$=" " THEN SFX 0
30 GO TO 20
```

**See also:** [MUSIC](#music), [SAMPLE](#sample), [LOAD](#load).

### SAMPLE

```
LOAD name$ SAMPLE n
SAMPLE n [, pitch [, volume [, pan]]] [; channel]
SAMPLE STOP [channel]
SAMPLE                         (function)
```

Recorded sounds (hits, voices, instruments) on the twelve sample channels,
without waiting, together with the music and everything else.

- `LOAD name$ SAMPLE n` loads a WAV (without an extension, `name.wav`) into
  sample `n`: 8- or 16-bit PCM, mono or stereo (mixed down), at any rate up
  to 1 MHz. It goes to the sound memory (96 KB) in 8 bits; it takes nothing
  from the BASIC's memory. If the WAV has a loop (the one Audacity and sound
  editors save), the sample repeats it until it is stopped. Loading a sample
  stops those that are playing.
- `SAMPLE n` plays it at its own pitch, at full volume and in the centre.
- Without `; channel`, it plays on a channel the music does not use (if
  there is none left, on the one that has had a sample the longest). With
  `; channel`, on that one: if it is a music channel, it covers the music
  while it plays, and the music carries on there at its next note.
- `SAMPLE STOP` stops them all; with `channel`, the one on that channel.
- The `SAMPLE` function is 1 while any of them plays.

**With the music:** the song's samples go into the sound memory from the
bottom and the `SAMPLE` ones from the top. Load the `SAMPLE` ones first: a
song loaded afterwards fits into the room left (in ADPCM, or with less
treble if need be), and a sample loaded after a song only uses what the
song leaves free. A `.zxm` is already converted and doesn't adjust: if it
doesn't fit, `4 Out of memory`; make it with
[`zxv-music --free`](../sdk/README.md#the-own-format-zxm-and-zxv-music) so
that it leaves room. BREAK, `RUN` and `NEW` stop the samples; the loaded ones
are kept like the `SFX` bank, and `NEW` clears them.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | file name | The WAV. |
| `n` | 0–31 | The sample (in the sample table, entries 224 to 255). |
| `pitch` | in semitones; can have decimals | Above the pitch it was recorded at: 0 (the default) the same, 12 an octave higher, −12 an octave lower. |
| `volume` | 0–64 | The default is 64. |
| `pan` | 0–255 | 0 left, 128 (the default) the centre, 255 right. |
| `channel` | 0–11 | Optional: the sample channel. |

**Returns** (the `SAMPLE` function): 1 if any `SAMPLE` sample plays; 0 if
not.

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `SAMPLE n` with sample `n` not loaded. |
| `B Integer out of range` | `n` negative or greater than 31 (also in `LOAD`); `volume` outside 0–64; `pan` outside 0–255; `channel` outside 0–11; a `pitch` so low or so high that the sample cannot play (the step must stay between 1/65536 and 256 times 48 kHz: for a 22,050 Hz sample, between about −178 and +109 semitones). |
| `R Loading error` | The file is not an 8- or 16-bit PCM WAV with one or two channels, or it has no sound. |
| `4 Out of memory` | The sample does not fit in the sound memory left; or the song loaded has more than 224 samples. |
| `C Nonsense in BASIC` | `SAVE`, `VERIFY` or `MERGE` with `SAMPLE`. |
| (common) | See [Common errors](#sound-and-music). |

**Example**

```
10 LOAD "hit" SAMPLE 0: LOAD "theme" MUSIC: MUSIC PLAY
20 IF INKEY$=" " THEN SAMPLE 0, RND*4-2
30 GO TO 20
```

**See also:** [MUSIC](#music), [SFX](#sfx), [SOUND](#sound),
[LOAD](#load).

## Keyboard, joystick and mouse

### INPUT

```
INPUT [items] variable [, variable]...
INPUT [items] LINE a$
INPUT #stream; variable [, variable]...
```

Stops the program and lets the user type values into variables, in the lower
part of the screen. The items are as in `PRINT`: string literals and
expressions in brackets are shown as a prompt, `AT`, `TAB` and colours work,
and each variable waits for a value.

- For a **number**, the user types a number or a numeric expression (even
  `2*3`), then Enter. If it has a syntax error, a `?` marks where, so the
  user can correct it. If it is correct but fails when worked out (a
  variable that does not exist, `1/0`), the program stops with that report.
- For a **string**, the input starts with a pair of quotes and the cursor
  inside them, because what the user types is taken as a string expression.
- `LINE a$` takes the text exactly as typed, with no quotes.
- Pressing Esc (BREAK), or typing `STOP` as the value, stops the program
  with `H STOP in INPUT`.

While typing, the left and right cursor keys, Home, End, Backspace and
Delete work. The cursor shows `L`, or `C` when Caps Lock is on. Whatever was
pressed earlier for `INKEY$`, `IN` or `KEY` does not appear: the input starts
empty.

The program's texts cannot hold accented letters, `ñ` or `¿`, and neither can
what is typed in the `INPUT`: letters lose their accent, so `Peña` arrives as
`Pena` (see [Character set](#character-set)).

With `#stream` it reads from a file, one line per variable (see
[Reading and writing text files](#reading-and-writing-text-files)).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| items | `"text"`, `(expression)`, `AT row, col`, `TAB n`, `INK`...`OVER`, `#stream`, `;`, `,`, `'` | What is printed before asking for the value, as in [PRINT](#print). A prompt text must be a string literal or an expression in brackets. |
| `variable` | a numeric or string variable, or an array element | Where what is typed is stored. |
| `LINE a$` | a string variable | Takes the text as it is, without quotes. |
| `stream` | 0–15 | The stream it reads from (see [Streams and channels](#streams-and-channels)). |

**Errors**

| Report | When |
|---|---|
| `H STOP in INPUT` | Esc is pressed, or `STOP` is typed as the value. |
| `C Nonsense in BASIC` | A prompt that is neither a string literal nor in brackets; `LINE` with a numeric variable. |
| `5 Out of screen` | `AT` outside the lower screen. |
| `B Integer out of range` | A value of `AT`, `TAB`, a colour or a stream out of its range (over 255, or for the stream, over 15). |
| `K Invalid colour` | A colour that is not valid (as in [INK, PAPER](#ink-paper)). |
| `O Invalid stream` | `#stream` of a stream that is not open. |
| (others) | Any error while working out the typed value (`2 Variable not found`, `6 Number too big`...) stops the program. Those of a file: see [Reading and writing text files](#reading-and-writing-text-files). |

**Example**

```
10 INPUT "Your name? "; LINE n$
20 INPUT "Your age? "; a
30 PRINT "Hello, "; n$; ". Next year you will be "; a+1
```

**See also:** [PRINT](#print), [INKEY$](#inkey), [KEY](#key),
[Reading and writing text files](#reading-and-writing-text-files).

### INKEY$

```
INKEY$
INKEY$ #stream
```

A function: the key being pressed at this moment, as a one-character string,
or `""` if there is none (or more than two at once). It does not wait. It
reads the keyboard like the Spectrum's:

- Letters come out in lower case (upper case with Shift or Caps Lock), digits
  as digits and Enter as `CHR$ 13`.
- The cursor keys give `CHR$ 8` (left), `CHR$ 9` (right), `CHR$ 10` (down)
  and `CHR$ 11` (up), and Backspace, `CHR$ 12`.
- Shift works as the Spectrum's CAPS SHIFT and Ctrl as its SYMBOL SHIFT:
  punctuation comes out as the keyboard gives it (`,`, `"`...), and Ctrl with
  a letter gives the same as SYMBOL SHIFT with it on the Spectrum, which is
  sometimes a keyword (Ctrl+A gives `CHR$ 226`, `STOP`).
- A joystick set up as keys with [`JOY`](#joy) also counts.

Whatever is pressed while a program reads the keyboard this way (with
`INKEY$`, `IN` or [`KEY`](#key)) is not kept for later: when the program ends
it does not appear typed in the editor, and an `INPUT` starts empty.

`INKEY$ #stream` reads the next character of a file open for reading (see
[Reading and writing text files](#reading-and-writing-text-files)).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `stream` | 0–15 | Optional: the stream of a file to read from. |

**Returns:** a one-character string, or `""` if there is no key (or two that
do not combine, or three or more). With `#stream`: the next character of the
file, or `""` at the end of the file or if the stream does not go to a file
(the screen or the printer).

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `stream` is negative or greater than 15. |
| `O Invalid stream` | The stream is not open. |
| `J Invalid I/O device` | The stream's file is open for writing (`OUT`). |
| (files) | If reading fails, those of [Files and the SD card](#files-and-the-sd-card). |

**Example**

```
10 LET k$=INKEY$: IF k$="" THEN GO TO 10
20 PRINT "You pressed "; k$: IF k$<>"q" THEN GO TO 10
```

**See also:** [KEY](#key), which is better for games, [PAUSE](#pause),
[IN, OUT](#in-out).

### KEY

```
KEY n
```

A function: 1 if key `n` is held down right now, 0 if not. Several keys held
at once can be checked, so this is the way to read the keyboard in games.
It looks at the PC keys, not at the Spectrum's matrix: it does not see
a joystick set up as keys with [`JOY`](#joy).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–511 (see the table) | The key. |

| `n` | Key |
|---|---|
| `CODE "a"` to `CODE "z"`, or `CODE "A"` to `CODE "Z"` | a letter (both give the same key) |
| `CODE "0"` to `CODE "9"` | a digit |
| 8, 9, 10, 11 | the cursor keys left, right, down, up |
| 12 | Backspace |
| 13 | Enter |
| 32 | Space |
| 256 to 511 | 256 + the USB key code of any key: 297 Esc, 298 Backspace, 299 Tab, 314 to 325 F1 to F12, 480 left Ctrl, 481 left Shift, 482 left Alt, 484 right Ctrl, 485 right Shift, 486 right Alt (AltGr) |

**Returns:** 1 if the key is held down, 0 if not (also 0 with a code from 256
to 511 that is not any key of the keyboard).

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `n` is between 0 and 255 and is not in the table: 0 to 7, 14 to 31, 33 to 47, 58 to 64, 91 to 96 and 123 to 255. |
| `B Integer out of range` | `n` is negative or greater than 511. |

**Example**

```
10 LET x=128
20 LET x=x+KEY 9-KEY 8: PLOT x,88: WAIT: GO TO 20
```

**See also:** [INKEY$](#inkey), [JOY](#joy), [MOUSE](#mouse).

### JOY

```
JOY n
JOY n, mode
```

The function `JOY n` gives the state of joystick `n` (0 for the first one
connected, 1 for the second, and so on up to 3) as a number made of these
bits:

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
| 9 | 512 | fire 4 |
| 10 | 1024 | L (upper left shoulder button) |
| 11 | 2048 | R (upper right shoulder button) |

To test a button, divide and look at the remainder: for example,
`INT (JOY 0/16)-2*INT (JOY 0/32)` is 1 while fire 1 is held.

The statement `JOY n, mode` makes joystick `n` also act as keys, for programs
that read the keyboard with `INKEY$` or `IN` (any fire button counts as fire):

| Mode | Left | Right | Down | Up | Fire |
|---|---|---|---|---|---|
| 0 | (not as keys) | | | | |
| 1 Sinclair 1 | 6 | 7 | 8 | 9 | 0 |
| 2 Sinclair 2 | 1 | 2 | 3 | 4 | 5 |
| 3 Cursor | 5 | 8 | 6 | 7 | 0 |

When the machine is switched on, joystick 0 is Sinclair 1, joystick 1 is
Sinclair 2, and joysticks 2 and 3 do not act as keys (`NEW` and `RUN` do not
change it). `KEY` does not see these
keys.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–3 | The joystick (up to 0.11, 0 or 1). |
| `mode` | 0–3 | Statement only: how it acts as keys (table above). |

**Returns** (the function): the buttons held, from 0 to 4095 (the sum of the
values in the table); 0 if the joystick is not connected.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `n` or `mode` is not 0 to 3 (also in the function). |

**Example**

```
10 PRINT AT 0,0; JOY 0; "   ": GO TO 10
```

**See also:** [KEY](#key), [INKEY$](#inkey), [IN, OUT](#in-out).

### MOUSE

```
MOUSE n
MOUSE x, y
```

The function `MOUSE n` reads the mouse; the statement `MOUSE x, y` places the
pointer. The pointer starts at the centre of the screen when the machine is
switched on and stays inside it: 0 to 255 and 0 to 191 in mode 0, 0 to 511
and 0 to 383 in mode 1. A value of the statement that falls outside the
screen is taken to the edge. ZX-V does not draw the pointer: your program
draws it, for example with a sprite.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–3 | What is read (table below). |
| `x`, `y` | 0–65535 | Statement only: where the pointer goes, in pixels from the top left. |

| `n` | Value |
|---|---|
| 0 | x of the pointer, in pixels from the left of the screen |
| 1 | y of the pointer, in pixels from the top |
| 2 | buttons: 1 left, 2 right, 4 middle (added together) |
| 3 | wheel: starts at 0, grows when turned up and shrinks when turned down |

**Returns** (the function): the value from the table.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `n` is negative or greater than 3; `x` or `y` are negative or greater than 65535 (even though the pointer is then clipped to the screen). |

**Example**

```
10 GRAB 0,3,"ff.f.f..f"
20 CLS: SPRITE 0,MOUSE 0,MOUSE 1: IF MOUSE 2 THEN PRINT "click"
30 WAIT: GO TO 20
```

**See also:** [SPRITE](#sprite), [KEY](#key), [IN, OUT](#in-out) (the
Kempston mouse).

## Files and the SD card

Programs and data are kept on the SD card. There is also a read-only drive,
`ROM:`, with the examples that come with the machine, and the external
drive, `EXT:`: a folder served by another device (through the board's J5
header, or in the emulator with `--ext`), which is read and written like the
card (see [The EXT: drive](../hardware/firmware.md#the-ext-drive)).

Errors from the card (or from `ROM:` or `EXT:`) come from the firmware, and
BASIC turns them into reports: a file that does not exist gives `T`, a name
that is not valid gives `F`, no card gives `U`, too many open files give
`O`, and anything else gives `V Disk error`, which adds the reason when it is
known.

**Common errors.** Besides those of each statement, any file statement can
give:

| Report | When |
|---|---|
| `F Invalid file name` | The name is empty, over 255 characters or has a control character; a part of the path is over 63 characters, has `\ : * ? " < > \|` or ends in a space or a dot; or there are wildcards where they are not allowed. |
| `T File not found` | A folder of the path does not exist, or the file where it must exist. |
| `U No SD card` | The path goes to the card and there is none in the slot (or it has been taken out). |
| `O Invalid stream` | There are already 8 files open on that drive (with `OPEN #`) and another one has to be opened. |
| `V Disk error` | The card failed to read or write, or its format is not understood (with no reason after it). |
| `V Disk error (not a folder)` | A part of the path that should be a folder is a file. |
| `V Disk error (no server for EXT:)` | The path goes to `EXT:` and there is no server, or it has stopped answering. |

Statements that write also give `V Disk error (not allowed)` on `ROM:`, and
`V Disk error (card full)` if there is no room left.

### File names and drives

- A name has up to 63 characters, without `\ : * ? " < > |`, without
  accented letters or `ñ`, and does not end in a space or a dot. Upper and
  lower case are the same. A whole path has up to 255 characters.
- Folders are separated with `/`. A path starting with `/` begins at the root
  of the drive; otherwise it starts in the current folder (see [CD](#cd-dir)).
  `..` is the folder above (at the root, it stays there) and `.`, the same
  one.
- `ROM:`, `SD:` and `EXT:` at the start (in capitals or not) choose a drive,
  from its root: `LOAD "ROM:hello"`, `CAT "SD:/games"`, `SAVE "EXT:copy"`.
  Without a drive, the drive of the current folder is used. Writing to `ROM:`
  (`SAVE`, `ERASE`, `MKDIR`...) gives `V Disk error (not allowed)`. With no
  server connected, everything on `EXT:` gives
  `V Disk error (no server for EXT:)`.
- **Wildcards** in `CAT`, `ERASE`, `MOVE` and `COPY`: `*` stands for any
  group of characters (also none) and `?` for one, with upper and lower case
  the same, in the last part of the path only: `"*.zvb"`,
  `"games/level?.dat"`. A wildcard in a folder of the path gives
  `F Invalid file name`. If nothing matches, `ERASE`, `MOVE` and `COPY` give
  `T File not found`; `CAT` lists nothing.
- A file that is open (with `OPEN #`) cannot be opened again for writing,
  deleted or renamed: `V Disk error (not allowed)`.

### SAVE

```
SAVE name$ [LINE line [SCREEN$] [PROTECT]]
SAVE name$ CODE start, length
SAVE name$ SCREEN$
SAVE name$ DATA letter()
SAVE name$ DATA letter$()
SAVE name$ GRAPHICS
```

Saves to the SD card (or to `EXT:`). An existing file is replaced.

- The **program** (with no other words) and its variables, in a `.zvb` file,
  the format of ZX-V programs: both as they are in memory, with no limit but
  the memory. With `LINE`, the program starts by itself from that line when
  loaded, with its variables. With `LINE` and `SCREEN$`, it also carries the
  screen you see as a **loading screen**: `LOAD` puts it up first, and the
  program starts with it. With `PROTECT` at the end, it is saved
  **protected** (see [Protected programs](#protected-programs)).
- `CODE`: `length` bytes of memory from address `start`, in a `.bin` file
  (just the bytes). The addresses are those of [`PEEK`](#peek-poke): below
  65536, the virtual Spectrum.
- `SCREEN$`: the program's screen, as you see it (its mode, its border and
  the colour of each pixel; if they are not the usual colours, also the
  colours it is shown with, see [`PALETTE`](#palette)), in a `.gfx` file with
  a screen (which you can edit in `zxv-gfx`). With the extension `.scr`, the
  Spectrum's one (`CODE 16384,6912`: the ink and paper shadow of each cell,
  without what is drawn with ZX-V's colours), in the format of its tools.
- `DATA`: an array (numbers or characters), in a `.zvd` file, the format of
  ZX-V data: as it is in memory, with no limit but the memory.
- `GRAPHICS`: the graphics store (sprites, tiles, maps, textures) as a
  `.gfx` file.

If the name has no extension, each kind gets its own: `.zvb`, `.bin`,
`.zvd` or `.gfx` (`GRAPHICS` and `SCREEN$`). With another extension, `CODE`
is saved the same way, as plain bytes, and a program, an array or a screen in
its format.

With the extension `.tap`, it is saved as a **Spectrum tape**, to take it to a
Spectrum emulator: the program and its variables, the `CODE` block (with its
address, so that `LOAD` puts it back), the screen (the Spectrum's, as with
`.scr`) or the array, in the Spectrum's format. The loading screen and the
protection only go in a `.zvb`. ZX-V's own keywords show there as odd
characters, and a block cannot be over 65535 bytes, nor a string over 65535
characters. A `.tzx` is not saved.

With the extension `.bas`, the program is saved as a **listing in plain text**
(see [Listings in text](#listings-in-text)): `SAVE "game.bas"`. A listing has
neither the variables nor the start line.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | path (see [File names](#file-names-and-drives)) | The file. It cannot be `""`. |
| `line` | 0–9999 (up to 65535 in a `.tap`) | The line the program starts from when loaded. |
| `start` | 0–4294967295 | The first address of `CODE`. |
| `length` | 0–4294967295 (up to 65535 in a `.tap`) | How many bytes. |
| `letter` | a letter | The array for `DATA`: `a()` of numbers, `a$()` of characters. |

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | `CODE` without `length`; `SAVE ... MUSIC`, `SFX` or `SAMPLE`; extension `.tzx`; `.zvb` with anything but a program, or `.zvd` with anything but an array; `LINE` (or `CODE`, `DATA`) in a `.bas`; a loading `SCREEN$` or `PROTECT` in a `.tap` or a `.bas`; `PROTECT` without `LINE`; a protected program saved without `LINE`. |
| `B Integer out of range` | `line` greater than 9999 in a `.zvb` (or than 65535); a `CODE` address that cannot be read (see [PEEK, POKE](#peek-poke)): then nothing is saved; in a `.tap`, a block (`CODE`, array, program with variables) over 65535 bytes, or a string over 65535 characters. |
| `2 Variable not found` | `DATA` of an array that does not exist. |
| `6 Number too big` | In a `.tap`, a number that does not fit the Spectrum's format. |
| `V Disk error (not allowed)` | On `ROM:`, or the file is open with `OPEN #`. |
| `V Disk error (it is a folder)` | `name$` is a folder. |
| `V Disk error (card full)` | There is no room left. |
| (common) | See [Common errors](#files-and-the-sd-card). |

**Example**

```
10 PRINT "I was saved"
SAVE "hello" LINE 10
```

**See also:** [LOAD](#load), [VERIFY](#verify),
[Protected programs](#protected-programs),
[Listings in text](#listings-in-text),
[Tape files](#tape-files).

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
LOAD name$ SAMPLE n
```

Loads from the SD card (or from `ROM:` or `EXT:`):

- A **program**: replaces the program in memory, and its variables replace
  the ones there were. If it was saved with `LINE`, it runs from that line
  without clearing them, as with `GO TO`. Without an extension, ZX-V tries
  `name.zvb`, then `name.tap`, then `name.zxv` (a [native
  program](#native-programs), which runs), then `name.bas` (a [listing in
  text](#listings-in-text)) and finally the name as it is. Spectrum `.tap`
  and `.tzx` files with BASIC programs load directly, with their variables
  (see [Tape files](#tape-files)); ZX-V `.tap` files from before 0.7 carry no
  variables. The old program is deleted before the new one is read (so one
  as big as the memory fits): if the file turns out not to be valid, memory
  is left with no program and no variables, but the editor's **Undo** option
  brings the lines back.
- `CODE`: a file of bytes (without an extension, `name.bin` and then
  `name.tap`), at `start` and at most `length` bytes (without `length`, the
  whole file). A file that is not a `.tap` does not know where it goes, so
  `start` is needed: `LOAD "data" CODE 40000`; the `CODE` block of a `.tap`
  goes back without it to the address it was saved from.
- `SCREEN$`: a screen (without an extension, `name.gfx` and then a block of
  `name.tap`). The one in a `.gfx` goes up as it is, with its mode (if it is
  not the current one, it changes as with `MODE`) and its border; if it
  carries its colours, they go into table 0's palette, without `REMAP`
  changes, and that table becomes the one in use (as with `REMAP #0`).
  `SCREEN$()` does not recognise the text on it. A `.scr` or a `.tap` is a
  Spectrum screen, shown as it was.
- `DATA`: an array (without an extension, `name.zvd` and then `name.tap`),
  replacing the array of that name, as `DIM` does. The type must match.
- `GRAPHICS`: the objects of a `.gfx` file (without an extension, it is
  added) replace those with the same number; the others stay, so you can
  load sprites from one file and a level from another.
- `MUSIC`, `SFX` and `SAMPLE`: a music module, an effect bank or a recorded
  sound (see [`MUSIC`](#music), [`SFX`](#sfx) and [`SAMPLE`](#sample)).
  Without an extension, `MUSIC` tries `.zxm`, `.pt3`, `.mod`, `.s3m`, `.xm`,
  `.vgm` and `.vgz` (if there is none, `.pt3`); `SFX` adds `.afb`, and
  `SAMPLE`, `.wav`.

`LOAD ""` (an empty name) loads the next block from the inserted tape (see
[Tape files](#tape-files)); with `GRAPHICS`, `MUSIC`, `SFX` or `SAMPLE` the
empty name is not allowed.

**The program's folder.** ZX-V remembers the folder the program was loaded
from. A name with no drive and no leading `/` is looked for in the current
folder first and, if it is not there, in the program's folder. So a game
loaded with `LOAD "ROM:game"` finds its graphics and music with
`LOAD "game" GRAPHICS`, without writing `ROM:`; and a file with the same
name in the card's current folder comes first, so you can replace them
without touching the game. It also works for `VERIFY`, `MERGE` and `OPEN #`
for reading (not for writing: `SAVE` always goes to the current folder).
`NEW` forgets it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | path, or `""` (the inserted tape) | The file. |
| `start` | 0–4294967295 | Where `CODE` is loaded. Required if the file is not a tape. |
| `length` | 0–4294967295 | At most, how many bytes. By default, all of them. |
| `letter` | a letter | The array for `DATA`. |
| `n` | 0–31 | The number of the `SAMPLE` sound. |

**Errors**

| Report | When |
|---|---|
| `R Loading error` | The file is not valid or not the kind asked for (a `.zvb` with `CODE`, a `.zxv` or a `.bas` not loaded as a program, an array of another type...); `LOAD ""` with no tape inserted, or with no block of that kind on it; a damaged tape block; a `.zxv` that is not valid or is for a newer ROM. If it is a program, memory is left with no program (see above). |
| `T File not found` | The file is not there with any of its extensions; with a tape inserted, there is no block with that name on it either. |
| `C Nonsense in BASIC` | `CODE` without `start` from a file that is not a `.tap`. |
| `B Integer out of range` | `SAMPLE` `n` greater than 31; a `CODE` address that cannot be written to (see [PEEK, POKE](#peek-poke)). |
| `4 Out of memory` | The program, the array, the graphics or the music do not fit; a `.zxv` that does not fit in user memory. |
| `V Disk error (it is a folder)` | `name$` is a folder. |
| (common) | See [Common errors](#files-and-the-sd-card). |

**Example**

```
LOAD "hello"
LOAD "ROM:graphics"
```

**See also:** [SAVE](#save), [MERGE](#merge), [VERIFY](#verify),
[Tape files](#tape-files),
[Native programs](#native-programs).

### VERIFY

```
VERIFY name$
VERIFY name$ CODE [start [, length]]
VERIFY name$ SCREEN$
VERIFY name$ DATA letter()
VERIFY name$ GRAPHICS
```

Checks that a file is the same as what is in memory, with the same forms and
the same extensions as `LOAD`, without changing anything. A program is
compared together with its variables: if the program has run since it was
saved, its variables are no longer the same. A program without an extension
is looked for as `.zvb` and `.tap`, not as `.zxv` or `.bas`.

**Parameters:** those of [LOAD](#load).

**Errors**

| Report | When |
|---|---|
| `R Loading error` | There are differences; or the file is not valid or not the kind asked for (a `.zxv` or a `.bas` is not checked). |
| `C Nonsense in BASIC` | `VERIFY ... MUSIC`, `SFX` or `SAMPLE`; `CODE` without `start` from a file that is not a `.tap`. |
| `B Integer out of range` | A `CODE` address that cannot be read. |
| (those of LOAD) | `T`, `4`... as in [LOAD](#load), and the [common ones](#files-and-the-sd-card). |

**Example**

```
SAVE "test" CODE 40000,100: VERIFY "test" CODE 40000
```

**See also:** [SAVE](#save), [LOAD](#load).

### MERGE

```
MERGE name$
```

Adds the lines of a saved program to the one in memory; lines with the same
number are replaced. The same goes for the variables, as on the Spectrum:
those in the file replace the ones with the same name, and the others stay.
It works with a `.zvb`, a program from a tape (`.tap`, `.tzx`) and a
[listing in text](#listings-in-text) (`.bas`, which has no variables);
without an extension they are tried in that order. It does not start the
program even if it was saved with `LINE`, and it can be undone with the
editor's **Undo** option.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | path, or `""` (the next program on the inserted tape) | The file. |

**Errors**

| Report | When |
|---|---|
| `R Loading error` | The file is not valid or is not a program; the program is [protected](#protected-programs). |
| `C Nonsense in BASIC` | `MERGE ... DATA`, `GRAPHICS`, `SCREEN$` (of a `.gfx`), `MUSIC`, `SFX` or `SAMPLE`, or a `.zvd`. |
| `4 Out of memory` | The lines do not fit. |
| `G No room for line` | In a `.bas`, a line over 1024 characters, or one that does not fit in memory. |
| (those of LOAD) | `T`... as in [LOAD](#load), and the [common ones](#files-and-the-sd-card). |

**Example**

```
MERGE "routines"
```

**See also:** [LOAD](#load), [Listings in text](#listings-in-text).

### Protected programs

```
SAVE name$ LINE line [SCREEN$] PROTECT
```

If you do not want the people who play your program to see how it is made,
save it with `PROTECT`. The program in memory does not change: you can go on
working on it and also save it unprotected, for yourself.

- A protected program starts by itself when it is loaded, so it needs
  `LINE`. It is only saved in a `.zvb` (in a `.tap` or a `.bas`, `C Nonsense
  in BASIC`).
- While it runs, it is like any other: `ON ERROR` catches errors and BREAK.
- When it stops, for any reason (an error, BREAK, `STOP` or because it
  ends), its report appears and it **is deleted**, with its variables, as
  with `NEW`. It cannot be listed, the deletion cannot be undone, and the
  debugger does not connect to it.
- `MERGE` does not accept it (`R Loading error`), and if it saves the
  program itself, it is saved protected (and needs `LINE`).
- If it loads another program, that one is not protected (unless it was saved
  that way).

It is a protection like the Spectrum's: it stops the curious, but the file
format is open, and someone who knows can read it on a PC. ZX-V's tools do
not show its listing either.

### Listings in text

A program can also live in a plain text file with the extension `.bas`, one
line of the program per line, each with its number, as `LIST` shows it. You
can write it in any text editor on your computer, copy it to the card and
load it with `LOAD "game.bas"` (or `MERGE`); `SAVE "game.bas"` writes the one
in memory, with the indentation of `LIST`.

```
10 REM Written outside ZX-V
20 for i = 1 to 10: print i: next i
```

- Each line is read as if you typed it in the editor: keywords in capitals or
  not, and the same syntax check. A line with an error stops the loading with
  its report and its line number (`C Nonsense in BASIC, 20:1`); the lines
  before it stay in memory.
- The number goes at the start (it can have spaces before it) and is from 1
  to 9999; otherwise `C Nonsense in BASIC` (with the number of the last good
  line). A line with only a number deletes that line, and blank lines are
  ignored. A `.bas` file is a program, not a list of commands (leave `RUN`
  out).
- `LOAD` of a `.bas` first deletes the program and the variables; `MERGE`
  merges it with the one in memory and keeps the variables. Both can be
  undone (the program lines, not the variables).
- The text is UTF-8. Accented letters lose the accent, as when you type them
  (`año` becomes `ano`); `£` and `©` are those of the Spectrum, and the block
  graphics are written with the Unicode quadrant characters
  (`▘ ▝ ▀ ▖ ▌ ▞ ▛ ▗ ▚ ▐ ▜ ▄ ▙ ▟ █`). UDG characters are saved as single bytes,
  which a text editor may show oddly but which load back correctly.
- Lines can end in LF (Linux, macOS) or CR LF (Windows); a line can be up to
  1024 characters long (otherwise `G No room for line`).
- A listing carries no variables, no start line and no protection.

### CAT

```
CAT [name$]
```

Lists a folder on the program screen (stream 2): first its name, then each
file with its size in bytes, and each folder with `/` and `<DIR>`. Without a
name, the current folder. With wildcards or a file name, only what matches
(if nothing matches, only the name appears).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | path, with wildcards in the last part | Optional: the folder, the file or the pattern. |

**Errors**

| Report | When |
|---|---|
| `T File not found` | The folder does not exist. |
| (common) | See [Common errors](#files-and-the-sd-card). |

**Example**

```
CAT "ROM:"
CAT "*.zvb"
```

**See also:** [CD, DIR$](#cd-dir), [File names and
drives](#file-names-and-drives).

### CD, DIR$

```
CD folder$
DIR$
```

`CD` changes the current folder: `CD "games"`, `CD ".."` (the folder above),
`CD "/"` (the root of the drive), `CD "ROM:"` (the root of the built-in
examples drive), `CD "EXT:"` (that of the external drive) and `CD "SD:"` (the
root of the card). The function `DIR$` gives the current folder, such as
`"/games"` (on the card), `"ROM:/"` or `"EXT:/"`.

A program that loads more files (like the platform game) looks for them in
the current folder and, if they are not there, in its own folder (see
[LOAD](#load)): with `LOAD "ROM:platformer"` it finds its graphics without
changing folder.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `folder$` | path of a folder | The new current folder. |

**Returns** (`DIR$`): the path of the current folder, from the root of its
drive, with `ROM:` or `EXT:` in front if it is not the card.

**Errors**

| Report | When |
|---|---|
| `T File not found` | The folder does not exist. |
| `V Disk error (not a folder)` | `folder$` is a file. |
| (common) | See [Common errors](#files-and-the-sd-card). `DIR$` gives no errors. |

**Example**

```
CD "ROM:": PRINT DIR$: CAT
```

**See also:** [CAT](#cat), [MKDIR, RMDIR](#mkdir-rmdir).

### MKDIR, RMDIR

```
MKDIR folder$
RMDIR folder$
```

`MKDIR` creates a folder; `RMDIR` deletes an empty folder.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `folder$` | path | The folder. The ones before it in the path must exist. |

**Errors**

| Report | When |
|---|---|
| `V Disk error (already exists)` | `MKDIR` of something that already exists (folder or file). |
| `V Disk error (folder not empty)` | `RMDIR` of a folder with something in it. |
| `V Disk error (not a folder)` | `RMDIR` of a file. |
| `T File not found` | `RMDIR` of something that does not exist. |
| `V Disk error (not allowed)` | On `ROM:`; `RMDIR` of the root. |
| `V Disk error (card full)` | `MKDIR` with no room. |
| (common) | See [Common errors](#files-and-the-sd-card). |

**Example**

```
MKDIR "games": CD "games"
```

**See also:** [CD, DIR$](#cd-dir), [ERASE](#erase).

### ERASE

```
ERASE name$
```

Deletes a file, or with wildcards every file that matches (`ERASE "*.bak"`);
folders are not deleted with wildcards (for that, `RMDIR`). Without
wildcards, the name of an empty folder deletes it too. There is no undo.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `name$` | path, with wildcards in the last part | What is deleted. |

**Errors**

| Report | When |
|---|---|
| `T File not found` | It does not exist, or no file matches the wildcards. |
| `V Disk error (not allowed)` | On `ROM:`; the file is open with `OPEN #`; the root. |
| `V Disk error (folder not empty)` | It is a folder with something in it. |
| (common) | See [Common errors](#files-and-the-sd-card). |

**Example**

```
ERASE "scores.dat"
```

**See also:** [MKDIR, RMDIR](#mkdir-rmdir), [MOVE](#move).

### MOVE

```
MOVE from$ TO to$
```

Renames a file or folder, or moves it to another folder on the same drive:
if `to$` is a folder, it goes inside it with the same name. With wildcards,
`to$` must be a folder and every file that matches is moved (not folders).
Unlike `COPY`, it does not replace a file that already exists. To take
something to another drive, copy it with `COPY` and delete it with `ERASE`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `from$` | path, with wildcards in the last part | What is moved. |
| `to$` | path | The new name, or the folder it goes to. |

**Errors**

| Report | When |
|---|---|
| `T File not found` | `from$` does not exist, or no file matches the wildcards. |
| `V Disk error (already exists)` | There is already something else with the new name; with wildcards, `to$` is the same folder. |
| `V Disk error (not a folder)` | With wildcards, `to$` is not a folder. |
| `V Disk error (not allowed)` | From one drive to another; on `ROM:`; a file open with `OPEN #`; the root. |
| `V Disk error` | A folder into itself. |
| (common) | See [Common errors](#files-and-the-sd-card). |

**Example**

```
MOVE "game.zvb" TO "old game.zvb"
MOVE "*.zvb" TO "games"
```

**See also:** [COPY](#copy), [ERASE](#erase).

### COPY

```
COPY from$ TO to$
COPY
```

With two names, copies a file, also from one drive to another (if `to$`
exists, it is replaced; if it is a folder, the copy goes inside with the same
name). With wildcards, `to$` must be a folder and every file that matches is
copied; if one fails, it stops there. To copy an example to the card:
`COPY "ROM:hello.zxv" TO "hello.zxv"`.

`COPY` alone sends the text on the program screen to the printer (the file
`printer.txt` on the card, see [LPRINT](#lprint-llist)): each cell that is
not a known character comes out as `?`. In this form it gives no errors.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `from$` | path, with wildcards in the last part | What is copied. |
| `to$` | path | The copy, or the folder it goes in. |

**Errors**

| Report | When |
|---|---|
| `T File not found` | `from$` does not exist, or no file matches the wildcards. |
| `V Disk error (it is a folder)` | `from$` is a folder (without wildcards). |
| `V Disk error (already exists)` | Copying a file onto itself. |
| `V Disk error (not a folder)` | With wildcards, `to$` is not a folder. |
| `V Disk error (not allowed)` | `to$` is on `ROM:`, or is open with `OPEN #`. |
| `V Disk error (card full)` | There is no room left (the copy is left half done). |
| (common) | See [Common errors](#files-and-the-sd-card). |

**Example**

```
COPY "ROM:hello.zxv" TO "hello.zxv"
```

**See also:** [MOVE](#move), [LPRINT, LLIST](#lprint-llist).

### Native programs

Programs written in C with the [SDK](../sdk/README.md) are `.zxv` files.
`LOAD "name"` runs them (the extension is optional). A native program uses the
memory of BASIC: **when it ends, the BASIC program and its variables are
gone**, so save your BASIC program first. Before loading it, the machine
checks the whole file (a damaged or half-copied file is not loaded); then it
closes the files open with `OPEN #`.

```
LOAD "ROM:hello"
```

When the program ends, BASIC shows a report, as after a command (`ON ERROR`
does not catch it):

- `0 OK, 0:1` if it ended normally.
- `W Native code error (3), 0:1` if it ended with an error code (3 in this
  example), or `W` followed by the program's own message if it gave one:
  `W Data file damaged (5), 0:1`.
- `W Native code error, 0:1` if it crashed (for example, by reading from a
  forbidden address).
- None if you stopped it with Ctrl+Alt+Delete: the machine starts again, as
  when you switch it on.

The report is the native program's: your BASIC program is gone in every
case. These can appear before it starts, and then the BASIC program is still
in memory:

| Report | When |
|---|---|
| `R Loading error` | It is not a valid `.zxv`, it is for a newer ROM, or it was asked for with `VERIFY` or `MERGE`. |
| `4 Out of memory` | It does not fit in user memory (compiled for another memory, or too big). |
| (common) | See [Common errors](#files-and-the-sd-card). |

If the card fails, or the file changes, once it is already loading, you get
its error (for example, `V Disk error`) and the BASIC program is gone too.

## Tape files

ZX-V reads the tape files of Spectrum emulators, `.tap` and `.tzx` (their
standard-speed blocks: programs, `CODE`, screens and arrays; the other blocks
of a `.tzx` are skipped), to use BASIC code made for the Spectrum. Spectrum
machine code inside them cannot run. ZX-V programs are saved in their own
format, `.zvb` (see [SAVE](#save)); a `.tap` is only saved when asked for, to
take a program to a Spectrum emulator.

A tape file that has been loaded stays **inserted**, like a cassette in a
tape recorder, so programs in several parts work:

- `LOAD ""` (also `LOAD "" SCREEN$`, `LOAD "" CODE`, `LOAD "" DATA a()`,
  `VERIFY ""`, `MERGE ""`) reads the next block of that kind from where the
  tape stopped. When it reaches the end it rewinds once. With no tape
  inserted, or no block of that kind, `R Loading error`.
- `LOAD "name"` uses the file `name` (with the usual extensions) if it exists
  on the card, and if it is a tape it becomes the inserted tape; if it does
  not exist, it looks on the inserted tape for a block with that name (up to
  10 characters, with upper and lower case the same), and if it is not there
  either, `T File not found`.
- A damaged block (with a wrong checksum, or cut short) gives
  `R Loading error`.
- The tape stays inserted after `NEW` and `CD`, until another tape file is
  loaded or the machine is reset.

So `LOAD "game"` loads the loader from `game.tap`, which can then bring its
title screen and the main program with `LOAD "" SCREEN$: LOAD ""`.

When a `.tap` is saved, the block name is the file name without the folder
or the extension, its first 10 characters: `SAVE "games/ship.tap"` is called
`ship`, as on the Spectrum.

## Streams and channels

Text goes in and out through numbered **streams**, 0 to 15. Streams 0 to 3
are ready from the start:

| Stream | Goes to |
|---|---|
| 0, 1 | the lower part of the screen (channel `"k"`) |
| 2 | the upper part of the screen (channel `"s"`), used by `PRINT` |
| 3 | the printer, the file `printer.txt` on the card (channel `"p"`), used by `LPRINT` |

`PRINT`, `LIST` and `INPUT` take a stream with `#`: `PRINT #1; "bottom"`
prints in the lower part and `LIST #3` lists to the printer. Streams 4 to 15
can be connected to text files. A stream that is not open gives
`O Invalid stream`.

**Common errors.** Besides those of each statement, the ones in this section
can give:

| Report | When |
|---|---|
| `B Integer out of range` | The stream number is negative or greater than 15. |
| `O Invalid stream` | The stream is not open (except in `OPEN #` and `CLOSE #`). |
| (files) | When opening, reading or writing a file, the [common file errors](#files-and-the-sd-card). |

### OPEN, CLOSE

```
OPEN #stream, name$ IN
OPEN #stream, name$ OUT
OPEN #stream, channel$
CLOSE #stream
```

`OPEN # ... OUT` creates a text file (emptying it if it exists) to write to;
`OPEN # ... IN` opens one to read (also in the [program's folder](#load)).
Files can only use streams 4 to 15, and each drive has at most 8 files open
at once. Opening a stream that already has a file closes it first.

`OPEN #stream, "k"`, `"s"` or `"p"` (in capitals or not) connects a stream,
0 to 15, to the lower screen, the upper screen or the printer.

`CLOSE #stream` closes the file (writing whatever is pending) and frees the
stream; streams 0 to 3 go back to their original channels. Closing a stream
that is not open does nothing. `RUN`, `CLEAR` and `NEW` close all files;
`NEW` also puts streams 0 to 3 back on their channels.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `stream` | 0–15 (4–15 for a file) | The stream. |
| `name$` | path | The text file. |
| `channel$` | `"k"`, `"s"` or `"p"` | The channel of the screen or the printer. |

**Errors**

| Report | When |
|---|---|
| `O Invalid stream` | A file on streams 0 to 3; there are already 8 files open on that drive. |
| `J Invalid I/O device` | `channel$` is not `"k"`, `"s"` or `"p"` (without `IN` or `OUT`). |
| `T File not found` | `IN` of a file that does not exist. |
| `V Disk error (not allowed)` | `OUT` on `ROM:`; `OUT` of a file that is already open. |
| `V Disk error (it is a folder)` | `name$` is a folder. |
| `V Disk error (card full)` | `OUT`, or `CLOSE` when writing what is pending, with no room. |
| (common) | See [Common errors](#streams-and-channels). |

**Example**

```
10 OPEN #4, "notes.txt" OUT: PRINT #4; "Hello": CLOSE #4
```

**See also:** [Reading and writing text files](#reading-and-writing-text-files),
[PRINT](#print), [LPRINT, LLIST](#lprint-llist).

### Reading and writing text files

```
PRINT #stream; items
LIST #stream [, line]
INPUT #stream; var [, var]... [LINE a$]
INKEY$ #stream
```

- `PRINT #n` writes like `PRINT`, without colours or `AT`; `TAB` writes
  spaces and keywords come out as text. In a file, **a comma starts a new
  line**, so that what `PRINT #n; a, b$` writes can be read back with
  `INPUT #n; a, b$`. What is written reaches the file at each line break,
  when it is closed and at the end of each statement.
- `LIST #n` writes the program as text.
- `INPUT #n` reads one line for each variable (lines can end in LF, CR or
  CR LF; of a line over 1024 characters, the rest is lost). A string variable
  takes the whole line; a number is worked out as an expression. Nothing is
  written to the file in `INPUT #n`: prompts are not allowed.
- `INKEY$ #n` gives the next character of the file, or `""` at the end.

**Errors**

| Report | When |
|---|---|
| `8 End of file` | `INPUT #` after the last line. |
| `C Nonsense in BASIC` | `INPUT #` of a number, and the line is not a valid numeric expression. |
| `J Invalid I/O device` | Writing (`PRINT #`, `LIST #`, an `INPUT #` prompt) to a file opened with `IN`, or reading (`INPUT #`, `INKEY$ #`) from one opened with `OUT`. |
| `V Disk error (card full)` | When writing, with no room. |
| (common) | See [Common errors](#streams-and-channels). |

**Example**

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

**See also:** [OPEN, CLOSE](#open-close), [INPUT](#input),
[INKEY$](#inkey), [LIST](#list).

### FORMAT

```
FORMAT ...
```

`FORMAT` exists only for compatibility with old Spectrum programs: whatever
follows it is not looked at, and when run it always gives
`O Invalid stream`.

**See also:** [OPEN, CLOSE](#open-close).

## Memory and system

### PEEK, POKE

```
PEEK address
POKE address, value
```

`PEEK` (a function) reads a byte from memory and `POKE` writes one.

- Addresses **below 65536** go to a **virtual Spectrum memory**, so that
  Spectrum programs that `PEEK` and `POKE` the screen, the colours or the
  system variables still work (see [The virtual Spectrum
  memory](#the-virtual-spectrum-memory)). Everything there can be read, and
  `POKE` into the ROM (below 16384) does nothing.
- Addresses **from 65536 up** read and write the real memory of ZX-V, byte by
  byte (for example the [machine registers](../machine.md)). `PEEK` reads the
  flash up to the end of the ROM (268435456 to 269221887) and the RAM
  (536870912 to 537333759); `POKE` only writes in that RAM. Careful: BASIC
  and your program are there too, and a wrong `POKE` can hang the machine.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `address` | 0–65535 (the virtual Spectrum); 268435456–269221887 (`PEEK` only) and 536870912–537333759 (the RAM) | The byte. |
| `value` | −255–255 | What `POKE` writes. A negative value is written as 256 plus that value (−1 is 255). |

**Returns** (`PEEK`): the byte, from 0 to 255.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `address` is negative, greater than 4294967295, or from 65536 up outside the areas in the table; `value` outside −255 to 255. |

**Example**

```
PRINT PEEK 536872704
```

prints the low byte of the ZX-V frame counter (60 per second; the
Spectrum's, `FRAMES`, is at 23672).

**See also:** [The virtual Spectrum
memory](#the-virtual-spectrum-memory), [USR](#usr), [IN, OUT](#in-out).

### The virtual Spectrum memory

| Addresses | Contents |
|---|---|
| 0–16383 | Spectrum ROM: reads as 0, except the character set at 15616–16383; `POKE` does nothing |
| 16384–22527 | the screen pixels, laid out as on the Spectrum (`POKE` draws, using the colours of each cell) |
| 22528–23295 | the colours of each cell (`POKE` changes them) |
| 23296–65535 | normal memory: what you `POKE` you can `PEEK`, except in the system variables (see below) |
| 23552–23733 | system variables (see below) |
| 65368–65535 | the user-defined graphics A to U |

Only the first 256 × 192 pixels of the screen and the first 32 × 24 cells are
visible at these addresses.

System variables that work: `LAST K` (23560, the last key), `CHARS`
(23606, where the character set is, minus 256; you can point it to your own
font in virtual memory), `BORDCR` (23624), `PROG` (23635, always 23755),
`FLAGS2` (23658, bit 3 is Caps Lock), `DF SZ` (23659, can be set from 0 to
2), `SEED` (23670, the `RND` seed), `FRAMES` (23672, three bytes counting
fiftieths of a second, like the Spectrum's frames), `UDG` (23675), `COORDS`
(23677 and 23678, the last point of `PLOT`), `S POSN` (23688, read only),
`SCR CT` (23692), `ATTR P`, `MASK P`, `ATTR T`, `MASK T` and `P FLAG` (23693
to 23697) and `RAMTOP` (23730).

### User-defined graphics (UDG)

The character codes 144 to 164 are 21 characters, A to U, that you can draw
yourself. `USR "a"` gives the address of the first byte of character A (and
so on to `USR "u"`); each character is 8 bytes, one per row from the top, and
each bit is a pixel (1 = ink). At start, and after `NEW`, they look like the
letters A to U. The system variable `UDG` (23675) says where they start; you
can move it elsewhere in virtual memory.

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

A function. With an address of 65536 or more, calls native RISC-V machine
code at that address and gives back the value it returns (in register `a0`).
Spectrum Z80 machine code cannot run: an address below 65536 gives
`S Z80 code not supported`. To call code without using the result,
`RANDOMIZE USR address` (which puts whatever it returns into the `RND` seed;
if that is over 65535, `B Integer out of range`).

If the native code fails (an illegal instruction, a forbidden address), the
program stops with `W Native code error`; `ON ERROR` does not catch that
report, but the BASIC program stays in memory.

`USR "a"` to `USR "u"` give the address of a [user-defined
graphic](#user-defined-graphics-udg).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `address` | 65536–4294967295 | Where the code starts (in RAM, usually loaded with `LOAD ... CODE`). |
| `"letter"` | `"a"` to `"u"` (or upper case) | The user-defined graphic. |

**Returns:** what the code leaves in `a0`, from 0 to 4294967295; or the
address of the graphic.

**Errors**

| Report | When |
|---|---|
| `S Z80 code not supported` | `address` is less than 65536. |
| `B Integer out of range` | `address` is negative or greater than 4294967295. |
| `A Invalid argument` | A string that is not a single letter from `a` to `u`. |
| `W Native code error` | The code failed. |

**Example**

```
PRINT USR "a"
```

**See also:** [PEEK, POKE](#peek-poke),
[User-defined graphics](#user-defined-graphics-udg),
[Native programs](#native-programs).

### IN, OUT

```
IN port
OUT port, value
```

Read (`IN`, a function) and write (`OUT`) the ports of the Spectrum, as
emulated by ZX-V:

| Port | `IN` | `OUT` |
|---|---|---|
| 254 (and any even port) | the keyboard half-rows selected by the high byte, as on the Spectrum (bits at 0 = pressed; also joysticks acting as keys) | bits 0–2 border colour, bit 4 beeper |
| 31 (and any odd port ending in 31) | Kempston joystick (joystick 0): bits 0–4 right, left, down, up, fire | |
| 64479, 65503 | Kempston mouse: x and y counters (y grows upwards) | |
| 64223 | Kempston mouse buttons: bit 0 right, 1 left, 2 middle (0 = pressed) | |
| 65533 | read the selected AY register | select an AY register (0–15); 255 selects the first chip and 254 the second |
| 49149 | | write the selected AY register |

Other ports read 255 and ignore writes. Programs written for ZX-V are better
off with `KEY`, `JOY`, `MOUSE`, `BORDER` and `SOUND`.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `port` | 0–65535 | The port. |
| `value` | −255–255 | What is written (a negative one, as 256 plus it). |

**Returns** (`IN`): the byte read, from 0 to 255.

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `port` is negative or greater than 65535; `value` outside −255 to 255. |

**Example**

```
10 PRINT AT 0,0; IN 65278; "   ": GO TO 10
```

**See also:** [KEY](#key), [JOY](#joy), [MOUSE](#mouse),
[BORDER](#border), [SOUND](#sound).

### TIME

```
TIME
```

A function: the number of frames since the machine was switched on, 60 per
second. To time something, subtract two readings.

**Parameters:** none.

**Returns:** 0 to 4294967295 (it goes back to 0 after about 828 days).

**Errors:** none.

**Example**

```
10 LET t=TIME: PAUSE 100: PRINT (TIME-t)/60; " seconds"
```

**See also:** [PAUSE](#pause), [WAIT](#wait), [CLOCK](#clock).

### CLOCK

```
CLOCK n
CLOCK year, month, day, hour, minutes [, seconds]
```

The function `CLOCK n` reads the date and time from the machine's clock. The
statement sets the clock. Files saved on the card are stamped with this
date. Without a program, the editor's menu sets it too: [Date and
time](editor.md#the-date-and-time).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0–6 | Function only: 0 year, 1 month, 2 day, 3 hour, 4 minutes, 5 seconds, 6 day of the week. |
| `year` | 2000–2099 | |
| `month` | 1–12 | |
| `day` | 1–31 (as many as the month has) | |
| `hour` | 0–23 | |
| `minutes` | 0–59 | |
| `seconds` | 0–59 | Optional; 0 by default. |

**Returns** (the function): the value asked for (the day of the week, from
1 Monday to 7 Sunday); 0 if the clock has not been set.

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | A date or time that does not exist (a year outside 2000–2099, 30 February, hour 24...), or the machine has no clock. |
| `B Integer out of range` | `n` is negative or greater than 6; a value of the statement negative or greater than 65535. |

**Example**

```
CLOCK 2026,11,24,18,30
PRINT CLOCK 2; "/"; CLOCK 1; "/"; CLOCK 0; " "; CLOCK 3; ":"; CLOCK 4
```

**See also:** [TIME](#time).

### SPEED

```
SPEED n
```

`SPEED 1` (the default) runs BASIC at full speed, about 200 times faster than
a Spectrum. `SPEED 0` makes each statement take about as long as on a 48K
Spectrum, for old programs that use loops as delays or would be unplayable at
full speed. The new commands (sprites, maps...) are not slowed down.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `n` | 0 or 1 | 0, at the Spectrum's speed; 1, at full speed. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `n` is between 2 and 255. |
| `B Integer out of range` | `n` is negative or greater than 255. |

**Example**

```
10 SPEED 0: FOR i=1 TO 100: PRINT i: NEXT i: SPEED 1
```

**See also:** [PAUSE](#pause), [WAIT](#wait).

### LANGUAGE

```
LANGUAGE code$
```

Sets the language of the machine with a two-letter code: `"en"` English or
`"es"` Spanish (in capitals or not). Error reports and the editor's bar and
menu use it from then on, and the machine remembers it (on the real
hardware, in its battery-backed clock; in the emulator, in its settings).
Any other two-letter code is accepted and shows English (a program can read
it to choose its own texts: `PEEK 536872736` and `PEEK 536872737` are the
codes of the two letters, in lower case).

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `code$` | two letters from `a` to `z` | The language. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `code$` is not two letters. |
| `C Nonsense in BASIC` | `code$` is not a string. |

**Example**

```
LANGUAGE "es"
```

**See also:** [Error messages](#error-messages).

### MONITOR

```
MONITOR
```

Opens the test monitor, a screen for checking the hardware. It has a console
with the commands `CAT` (or `DIR`), `CD`, `TYPE` (shows a text file) and
`LOAD` (runs a native `.zxv` program), and these keys: F1 changes the video
mode, F2 puts stripes in the border, F3 scrolls the screen, F4 plays a chord
on the AY, F6 a sweep on the beeper and F5 tests an exception. It does not go
back to BASIC: to leave it, reset the machine (Ctrl+Alt+Del). The BASIC
program is lost.

**Parameters:** none. **Errors:** none.

**Example**

```
MONITOR
```

**See also:** [Native programs](#native-programs).

## Editing commands

These commands are mostly typed as direct commands while you write a program.
See [the editor](editor.md) for the full picture. Those that change the
program (`RENUM`, `DELETE`, `REPLACE`, `NEW`) can be undone with the **Undo**
option of the EDIT menu (the Tab key), like a change made by hand (see
[Undo](editor.md#undo)).

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
further right for each level (the spaces are not stored). BREAK (Esc) stops
it.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `line` | 0–65535 | Where from. By default, from the start. |
| `stream` | 0–15 | Where it is written. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `line` greater than 65535, or `stream` greater than 15 (or negative). |
| `O Invalid stream` | The stream is not open. |
| `J Invalid I/O device` | The stream is a file opened with `IN`. |
| `L BREAK into program` | Esc is pressed while it lists. |

**Example**

```
LIST #2, 100
```

**See also:** [LLIST](#lprint-llist), [EDIT](#edit).

### EDIT

```
EDIT [line]
```

Puts the editor's cursor on that line (or the next one that exists); without
a number, on the current line. Inside a program, it stops it and opens the
editor at that line, with no report. Not to be confused with the EDIT key
(Tab), which opens the editor's options menu.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `line` | 0–9999 | The line. |

**Errors**

| Report | When |
|---|---|
| `B Integer out of range` | `line` is negative or greater than 9999. |

**Example**

```
EDIT 100
```

**See also:** [LIST](#list), [FIND](#find).

### AUTO

```
AUTO [first [, step]]
```

Automatic line numbers: each new line in the editor starts with the next
number. Without values (or with `first` 0), it starts at the last line of the
program plus `step`; and each line is `step` more than the one before (10
if not given). It ends when you press Enter on a
line with only the number, press Esc, or move to another line. Only as a
direct command.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `first` | 0–9999 | The number of the first line. By default (or with 0), the last one plus `step`. |
| `step` | 1–9999 | How much it goes up by. By default, 10. |

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | Inside a program. |
| `B Integer out of range` | A value greater than 9999 or negative; `step` 0; the first line would go past 9999. |

**Example**

```
AUTO 100, 5
```

**See also:** [RENUM](#renum).

### RENUM

```
RENUM [first [, step]]
```

Renumbers the whole program, and corrects the line numbers after `GO TO`
(also that of `ON ERROR GO TO`), `GO SUB`, `RESTORE`, `RUN`, `LIST`, `LLIST`
and `LINE`. Only numbers written directly at the end of a statement are
corrected: in `GO TO n*10` the expression is not changed. A number pointing
to a line that does not exist ends up pointing to the next one there is (if
there is none, it stays as it is). Labels do not need correcting. Only as a
direct command. The editor's options menu has the same thing, and it can be
undone.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `first` | 1–9999 | The new number of the first line. By default, 10. |
| `step` | 1–9999 | How much it goes up by. By default, 10. |

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | Inside a program. |
| `B Integer out of range` | A value of 0, negative or greater than 9999; the numbers would go past 9999. |
| `4 Out of memory` | There is no memory to rebuild the program. |

**Example**

```
RENUM 100, 10
```

**See also:** [AUTO](#auto), [DELETE](#delete).

### DELETE

```
DELETE first [, last]
```

Deletes the lines from `first` to `last` (or just line `first`). If there
are none in that range, it does nothing. Only as a direct command; it can be
undone.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `first` | 0–9999 | The first line deleted. |
| `last` | 0–9999 | The last one. By default, the same as `first`. |

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | Inside a program. |
| `B Integer out of range` | A value negative or greater than 9999. |

**Example**

```
DELETE 100, 190
```

**See also:** [RENUM](#renum), [NEW](#new).

### FIND

```
FIND [text$]
```

Moves the editor's cursor to the next program line where `text$` appears,
starting with the line after the cursor and going round at the end, with the
cursor on the text. Upper and lower case are the same, and it searches the
listing as you see it (keywords, quoted texts and `REM` included, but not the
line numbers). Without text, it searches for the same as last time, like the
F3 key. If it is not there, the status line says `Not found`. Only as a
direct command.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `text$` | a string | Optional: what is searched for. |

**Errors**

| Report | When |
|---|---|
| `C Nonsense in BASIC` | Inside a program; `text$` is not a string. |

**Example**

```
FIND "score"
```

**See also:** [REPLACE](#replace).

### REPLACE

```
REPLACE old$, new$
```

Changes the text `old$` to `new$` in the whole program, with upper and lower
case the same. Each changed line is checked as if it were typed, and lines
that would have a mistake (or go over 1024 characters) are left alone. The
status line says how many changes were made and how many lines were left
unchanged. Only as a direct command; it can be undone.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `old$` | a non-empty string | What is searched for. |
| `new$` | a string (can be empty) | What it is changed to. |

**Errors**

| Report | When |
|---|---|
| `A Invalid argument` | `old$` is empty. |
| `C Nonsense in BASIC` | Inside a program; a value that is not a string. |

**Example**

```
REPLACE "lives", "energy"
```

**See also:** [FIND](#find).

### NEW

```
NEW
```

Deletes the program, the variables and the graphics store (with the loaded
music, effects and sounds, which go quiet), closes the files and starts
afresh: the colours, the border, the video mode, the UDGs and the streams go
back to the usual ones, and the program's folder is forgotten. The inserted
tape stays.

It can be undone: the **Undo** option of the EDIT menu (the Tab key) brings
back the program lines (not its variables, nor the graphics). For that the
editor keeps a copy of the program in free memory: if the program is so big
that the copy does not fit, or if something uses that memory afterwards, it
can no longer be undone. So it is better to save first.

**Parameters:** none. **Errors:** none.

**Example**

```
NEW
```

**See also:** [CLEAR](#clear), [DELETE](#delete),
[Undo](editor.md#undo).

## Error handling

### Reports

When a program stops, a report appears at the bottom of the program screen:

```
2 Variable not found, 20:1
```

It gives a code (a digit or a letter), the message, and where it happened:
the line number and the statement within the line (`20:1` is the first
statement of line 20; line 0 means a direct command). `0 OK` means the
program ended normally. Press a key to go back to the editor (that key also
counts for the editor).

`V Disk error` adds the reason in brackets if it is known, and `W Native code
error` the number or the message of the native program (see [Native
programs](#native-programs)). The reports appear in the language of the
machine (see [LANGUAGE](#language)). A BREAK also stops the music and the
effects.

### ON ERROR

```
ON ERROR GO TO line
ON ERROR GO TO @label
ON ERROR GO TO 0
```

From now on, if an error stops the program, it jumps to that line or label
instead. There, `ERR` and `ERL` say what happened. `ON ERROR GO TO 0` turns it
off, as do `RUN`, `CLEAR` and `NEW`.

- The trap turns itself off when it is used, so that an error inside the
  handler does not loop for ever: write `ON ERROR` again to keep catching
  errors.
- `CONTINUE` repeats the statement that failed (after BREAK, it goes on with
  the next one).
- It catches all errors of a program, BREAK and `H STOP in INPUT` included,
  except `9 STOP statement`, errors in direct commands, the `W` of native
  code that fails with `USR` and the report of a native program.
- When it jumps, any `FN` in progress is forgotten and, if the program was
  printing on the lower screen, it goes back to the upper one.

**Parameters**

| Parameter | Values | What it is |
|---|---|---|
| `line` | 0–65535 | Where it jumps to (if it does not exist, to the next one there is). 0 turns the trap off. |
| `@label` | a label in the program | Where it jumps to. |

**Errors**

| Report | When |
|---|---|
| `X Label not found` | There is no such `@label` in the program. |
| `B Integer out of range` | `line` negative or greater than 65535. |
| `C Nonsense in BASIC` | `GO TO` is missing. |

**Example**

```
10 ON ERROR GO TO @oops
20 PRINT 1/0
30 PRINT "carrying on"
40 STOP
50 @oops: PRINT "Error "; ERR; " at line "; ERL
60 ON ERROR GO TO @oops: GO TO 30
```

**See also:** [ERR, ERL](#err-erl), [CONTINUE](#continue),
[Error messages](#error-messages).

### ERR, ERL

```
ERR
ERL
```

Two functions. After an error caught by `ON ERROR`, `ERR` is the error code
as a number (the digits are themselves, and the letters count on from 10:
`A` is 10, `B` 11... `Z` 35; the [table below](#error-messages) gives them
all) and `ERL` is the line where it happened.

**Parameters:** none.

**Returns:** `ERR`, 1 to 35; `ERL`, 1 to 9999. Both keep the last error
caught (even if the program is run again), and are 0 if none has been caught
yet.

**Errors:** none.

**Example**

```
10 ON ERROR GO TO 30
20 PRINT SQR -1
30 PRINT ERR; " "; ERL
```

**See also:** [ON ERROR](#on-error).

## Error messages

The reports in English and Spanish, with the number that `ERR` gives:

| Code | `ERR` | English | Spanish | When it appears |
|---|---|---|---|---|
| 0 | 0 | OK | OK | Not an error: the program or command ended normally. |
| 1 | 1 | NEXT without FOR | NEXT sin FOR | `NEXT v`, and `v` is an ordinary variable, not that of a `FOR` (if it does not exist, `2`). |
| 2 | 2 | Variable not found | Variable no encontrada | Using a variable before giving it a value, or an array before its `DIM`; `NEXT` of a variable that does not exist; `SAVE ... DATA` of an array that does not exist. |
| 3 | 3 | Subscript wrong | Subíndice incorrecto | A subscript out of the array's size, the wrong number of subscripts or a slice outside the string; a `DIM` with a size of 0 or with more than 16 dimensions. |
| 4 | 4 | Out of memory | Sin memoria | No memory left (for a variable, a line, a file being loaded...); too many nested `GO SUB` (512) or loops and procedures (64); too much to keep for `PROC` and `LOCAL` (8 KB); an expression nested too deep; a `DIM` of more than 16777215 elements; a `.zxv` that does not fit. |
| 5 | 5 | Out of screen | Fuera de la pantalla | `AT` outside the screen (in `PRINT` or in `INPUT`), or the lower screen cannot grow any more. |
| 6 | 6 | Number too big | Número demasiado grande | A result above 1.7E38; often, a division by zero. Also a number that does not fit the Spectrum's format when saving a `.tap`. |
| 7 | 7 | RETURN without GOSUB | RETURN sin GO SUB | `RETURN` with no `GO SUB` to go back to. |
| 8 | 8 | End of file | Fin del fichero | `INPUT #` after the last line of a file. |
| 9 | 9 | STOP statement | Sentencia STOP | The program reached a `STOP`. `CONTINUE` carries on. |
| A | 10 | Invalid argument | Argumento no válido | A value a function or a statement does not accept: `SQR -1`, `LN 0`, `ASN 2`, a negative number raised to a non-whole one; `KEY` of a code that is not a key; `USR` of a string that is not `"a"` to `"u"`; a sprite, a map or a texture that does not exist; a badly written `PLAY` string; an impossible date (or no clock) in `CLOCK`; `SPEED` that is not 0 or 1; `LANGUAGE` without two letters; `REPLACE` with empty text; more than 64 corners in `POLY`; a `MODE` mode that does not exist. |
| B | 11 | Integer out of range | Entero fuera de rango | A whole number out of its range: a coordinate off the screen, `CHR$ 300`, `POKE` with 256 or at an address that is not RAM, a stream greater than 15, `JOY 4`, `MOUSE 4`, `CLOCK 7`, `KEY 512`, a line greater than 9999 in `RENUM`, `AUTO` or `SAVE`; a block over 65535 bytes in a `.tap`. A value that is negative or greater than 255 where a byte is expected gives `B` even if the statement then checks a different range with a different report. |
| C | 12 | Nonsense in BASIC | Sintaxis incorrecta | The line is not valid BASIC (a typing mistake, a missing bracket); a value of the wrong type; a number of values that is not allowed (`PATTERN` takes 1, 2, 8 or 9; `POLY`, at least one point); `RENUM`, `DELETE`, `AUTO`, `FIND` or `REPLACE` inside a program; a form of `SAVE`, `LOAD`, `VERIFY` or `MERGE` that is not accepted (a `.tzx` in `SAVE`, `PROTECT` without `LINE`, `MERGE ... GRAPHICS`...); in `INPUT #`, a line that is not a number. |
| D | 13 | BREAK - CONT repeats | BREAK - CONT repite | You answered N, Space or Esc to `scroll?`. `CONTINUE` repeats. |
| E | 14 | Out of DATA | No quedan DATA | `READ` with no `DATA` left. |
| F | 15 | Invalid file name | Nombre de fichero no válido | An empty name, a name too long or with characters not allowed; wildcards in the folder of a path. |
| G | 16 | No room for line | No cabe la línea | The line is too long (over 2048 bytes once converted), or memory is full when it is stored; in a `.bas`, a line over 1024 characters or one that does not fit. |
| H | 17 | STOP in INPUT | STOP en INPUT | `STOP` typed, or Esc pressed, in an `INPUT`. |
| I | 18 | FOR without NEXT | FOR sin NEXT | A `FOR` loop that runs no times and has no `NEXT` to jump to. |
| J | 19 | Invalid I/O device | Dispositivo no válido | Writing to a file opened for reading, or reading from one opened for writing; an unknown channel in `OPEN #`. |
| K | 20 | Invalid colour | Color no válido | A colour out of range (`INK 10`, `BORDER 8`, `BOX 16,...`). |
| L | 21 | BREAK into program | BREAK en el programa | You pressed BREAK (Esc). `CONTINUE` carries on with the next statement. |
| M | 22 | RAMTOP no good | RAMTOP no válido | A `CLEAR` address that is too low or too high. |
| N | 23 | Statement lost | Sentencia perdida | A jump back to a statement that no longer exists (with `CONTINUE`, `RETURN` or `NEXT`, after changing the line); `CONTINUE` after a stop in a direct command. |
| O | 24 | Invalid stream | Canal no válido | A stream not open; a file on streams 0–3; more than 8 open files on a drive; `FORMAT`. |
| P | 25 | FN without DEF | FN sin DEF FN | `FN` of a function with no `DEF FN` in the program. |
| Q | 26 | Parameter error | Error en los parámetros | The number or type of the values of `FN` or `PROC` do not match their `DEF`; more than 32 `FN` values in progress; more than 16 parameters in `DEF PROC`, `PROC` or `LOCAL`; more than 8 strings in `PLAY`. |
| R | 27 | Loading error | Error al cargar | A file that is not valid or not the kind asked for; `LOAD ""` with no tape inserted or no block of that kind; `VERIFY` found differences; `MERGE` of a protected program; a `.zxv` for a newer ROM. |
| S | 28 | Z80 code not supported | Código Z80 no admitido | `USR` with an address below 65536: Spectrum machine code. |
| T | 29 | File not found | Fichero no encontrado | No file or folder with that name; nothing matches the wildcards of `ERASE`, `MOVE` or `COPY`; neither the file nor a block with that name on the inserted tape. |
| U | 30 | No SD card | No hay tarjeta SD | The path goes to the card and there is no card in the slot. |
| V | 31 | Disk error | Error de disco | The card failed or the operation is not possible; the reason follows in brackets, if it is known (see below). |
| W | 32 | Native code error | Error en el código nativo | Native code called with `USR` failed, or a native program (`.zxv`) ended with an error. It may carry the program's code or its own message (see [Native programs](#native-programs)). |
| X | 33 | Label not found | Etiqueta no encontrada | `GO TO @name` (or `GO SUB`, `RUN`, `RESTORE`, `ON ERROR GO TO`) with no `@name` in the program. |
| Y | 34 | Structure error | Error de estructura | `WEND`, `UNTIL` or `END PROC` without their opening; `LOCAL` outside a procedure; a block (`WHILE`, `IF`, `SELECT CASE`, `DEF PROC`...) without its end. |
| Z | 35 | PROC not found | PROC no encontrado | `PROC` of a procedure with no `DEF PROC`. |

The reasons of `V Disk error`: `already exists` / `ya existe`,
`folder not empty` / `la carpeta no está vacía`, `it is a folder` /
`es una carpeta`, `not a folder` / `no es una carpeta`, `card full` /
`la tarjeta está llena`, `not allowed` / `no se puede`, `no server for EXT:` /
`EXT: sin servidor`. With no reason, the card failed to read or write.

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

In the editor you can type the codes 32 to 127 (`£` and `©` with their keys,
if your keyboard has them; the `` ` `` key types nothing). Accented letters,
`ñ`, `¿` and `¡` are **not** BASIC characters, so the texts of your programs
have to do without them. If you type them, in the editor or in an `INPUT`, a
letter with an accent, a diaeresis or a cedilla comes out without it (`á` as
`a`, `Ñ` as `N`, `ç` as `c`), `¿` and `¡` come out as `?` and `!`, and any
other character does nothing. The ROM does use them in its own messages, such
as the error reports in Spanish, but your programs cannot. Block graphics and
user-defined graphics are written with `CHR$`.

## Limits

| What | Limit |
|---|---|
| Line numbers | 1 to 9999 |
| Length of a line in the editor | 1024 characters (2048 bytes once converted) |
| Numbers | about ±2.9E-39 to ±1.7E38, 8 digits shown |
| Numeric variable names | up to 63 letters and digits |
| String variables, arrays, `FOR` variables, `DEF FN` | one letter (26 of each) |
| Strings | as long as memory allows (65535 characters in a `.tap`) |
| Array dimensions | up to 16, each up to 65535, and up to 16777215 elements |
| Nested `GO SUB` | 512 |
| Nested `WHILE`, `REPEAT` and `PROC` | 64 |
| Nested block IFs and `SELECT CASE` | no limit |
| Parameters of a procedure, and variables of a `LOCAL` | 16 |
| What `PROC` and `LOCAL` keep (the earlier values of their variables) | 8 KB |
| `FN` values in progress (adding up nested ones) | 32 |
| Nested brackets in an expression | about 170 (20 nested `FN`) |
| Memory for program and variables | about 94 KB (shown in the editor's bar) |
| Open files | 8 per drive (the card, `ROM:` and `EXT:`), on streams 4 to 15 |
| Sprites | 256 (0–255), each up to 1024 × 1024 pixels |
| Tiles | 4096 of 8 × 8 and 4096 of 16 × 16 |
| Tile maps | 16 (0–15), up to 32767 × 32767 cells |
| Textures | 4 (0–3) |
| Polygon corners | 64 |
| `PLAY` | 8 strings (6 play), 4 levels of brackets |
| `SAMPLE` sounds | 32 (0–31) |
| File names | 63 characters; paths 255 |

## Keyword index

| Keyword | Kind | Entry |
|---|---|---|
| `@` | statement | [@label](#label) |
| `<=`, `>=`, `<>` | operator | [Expressions and operators](#expressions-and-operators) |
| `ABS` | function | [ABS, SGN, INT](#abs-sgn-int) |
| `ACS` | function | [ASN, ACS, ATN](#asn-acs-atn) |
| `AND` | operator | [True and false](#true-and-false) |
| `ASN` | function | [ASN, ACS, ATN](#asn-acs-atn) |
| `AT` | in `PRINT`, `INPUT` | [PRINT](#print), [INPUT](#input) |
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
| `CODE` | function; in `SAVE`, `LOAD`, `VERIFY` | [CHR$, CODE](#chr-code), [SAVE](#save), [LOAD](#load) |
| `CONTINUE` | statement | [CONTINUE](#continue) |
| `COPY` | statement | [COPY](#copy) |
| `COS` | function | [SIN, COS, TAN](#sin-cos-tan) |
| `DATA` | statement; in `SAVE`, `LOAD`, `VERIFY` | [READ, DATA, RESTORE](#read-data-restore), [SAVE](#save), [LOAD](#load) |
| `DEF FN` | statement | [DEF FN, FN](#def-fn-fn) |
| `DEF PROC` | statement | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `DELETE` | statement | [DELETE](#delete) |
| `DIM` | statement | [DIM](#dim) |
| `DIR$` | function | [CD, DIR$](#cd-dir) |
| `DISC` | statement | [DISC](#disc) |
| `DRAW` | statement | [DRAW](#draw) |
| `EDIT` | statement | [EDIT](#edit) |
| `ELSE` | in `IF`, `CASE ELSE` | [IF, THEN, ELSE](#if-then-else), [SELECT CASE](#select-case) |
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
| `GO TO` | statement; in `ON ERROR` | [GO TO](#go-to), [ON ERROR](#on-error) |
| `GRAB` | statement | [GRAB](#grab) |
| `GRAPHICS` | in `SAVE`, `LOAD`, `VERIFY` | [SAVE](#save), [LOAD](#load), [VERIFY](#verify) |
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
| `MAP` | statement; in `FLOOR`, `ROTATE` | [MAP](#map), [MAP DIM, MAP POKE](#map-dim-map-poke), [FLOOR](#floor), [ROTATE](#rotate) |
| `MERGE` | statement | [MERGE](#merge) |
| `MKDIR` | statement | [MKDIR, RMDIR](#mkdir-rmdir) |
| `MODE` | statement | [MODE](#mode) |
| `MONITOR` | statement | [MONITOR](#monitor) |
| `MOUSE` | function, statement | [MOUSE](#mouse) |
| `MOVE` | statement | [MOVE](#move) |
| `MUSIC` | function, statement; in `LOAD` | [MUSIC](#music), [LOAD](#load) |
| `NEW` | statement | [NEW](#new) |
| `NEXT` | statement | [FOR, NEXT](#for-next) |
| `NOT` | operator | [True and false](#true-and-false) |
| `ON ERROR` | statement | [ON ERROR](#on-error) |
| `OPEN #` | statement | [OPEN, CLOSE](#open-close) |
| `OR` | operator | [True and false](#true-and-false) |
| `OUT` | statement; in `OPEN #` | [IN, OUT](#in-out), [OPEN, CLOSE](#open-close) |
| `OVER` | statement, item | [INVERSE, OVER](#inverse-over) |
| `PAGE` | statement | [PAGE, SCREEN](#page-screen) |
| `PALETTE` | statement | [PALETTE](#palette) |
| `PAPER` | statement, item | [INK, PAPER](#ink-paper) |
| `PATTERN` | statement | [PATTERN](#pattern) |
| `PAUSE` | statement | [PAUSE](#pause) |
| `PEEK` | function | [PEEK, POKE](#peek-poke) |
| `PI` | function | [PI](#pi) |
| `PIXEL` | function | [PIXEL](#pixel) |
| `PLAY` | statement; in `MUSIC` | [PLAY](#play), [MUSIC](#music) |
| `PLOT` | statement | [PLOT](#plot) |
| `POINT` | function | [POINT](#point) |
| `POKE` | statement; in `MAP` | [PEEK, POKE](#peek-poke), [MAP DIM, MAP POKE](#map-dim-map-poke) |
| `POLY` | statement | [POLY](#poly) |
| `PRINT` | statement | [PRINT](#print) |
| `PROC` | statement | [DEF PROC, PROC, END PROC](#def-proc-proc-end-proc) |
| `PROTECT` | in `SAVE` | [Protected programs](#protected-programs) |
| `RANDOMIZE` | statement | [RND, RANDOMIZE](#rnd-randomize) |
| `RASTER` | statement | [RASTER](#raster) |
| `READ` | statement | [READ, DATA, RESTORE](#read-data-restore) |
| `REM` | statement | [REM](#rem) |
| `REMAP` | statement; in `RASTER` | [REMAP](#remap), [RASTER](#raster) |
| `RENUM` | statement | [RENUM](#renum) |
| `REPEAT` | statement | [REPEAT, UNTIL](#repeat-until) |
| `REPLACE` | statement | [REPLACE](#replace) |
| `RESTORE` | statement | [READ, DATA, RESTORE](#read-data-restore) |
| `RETURN` | statement | [GO SUB, RETURN](#go-sub-return) |
| `RMDIR` | statement | [MKDIR, RMDIR](#mkdir-rmdir) |
| `RND` | function | [RND, RANDOMIZE](#rnd-randomize) |
| `ROTATE` | statement | [ROTATE](#rotate) |
| `RUN` | statement | [RUN](#run) |
| `SAMPLE` | function, statement; in `LOAD` | [SAMPLE](#sample), [LOAD](#load) |
| `SAVE` | statement | [SAVE](#save) |
| `SCREEN` | statement | [PAGE, SCREEN](#page-screen) |
| `SCREEN$` | function; in `SAVE`, `LOAD`, `VERIFY` | [SCREEN$, ATTR](#screen-attr), [SAVE](#save), [LOAD](#load) |
| `SCROLL` | statement; in `RASTER` | [SCROLL](#scroll), [RASTER](#raster) |
| `SELECT CASE` | statement | [SELECT CASE](#select-case) |
| `SFX` | function, statement; in `LOAD` | [SFX](#sfx), [LOAD](#load) |
| `SGN` | function | [ABS, SGN, INT](#abs-sgn-int) |
| `SIN` | function | [SIN, COS, TAN](#sin-cos-tan) |
| `SOUND` | statement | [SOUND](#sound) |
| `SPEED` | statement | [SPEED](#speed) |
| `SPRITE` | statement | [SPRITE](#sprite) |
| `SQR` | function | [SQR](#sqr) |
| `STEP` | in `FOR` | [FOR, NEXT](#for-next) |
| `STOP` | statement | [STOP](#stop) |
| `STR$` | function | [STR$, VAL, VAL$](#str-val-val) |
| `TAB` | in `PRINT`, `INPUT` | [PRINT](#print), [INPUT](#input) |
| `TAN` | function | [SIN, COS, TAN](#sin-cos-tan) |
| `TEXTURE` | statement | [TEXTURE](#texture) |
| `THEN` | in `IF` | [IF, THEN, ELSE](#if-then-else) |
| `TILE` | statement | [TILE](#tile) |
| `TIME` | function | [TIME](#time) |
| `TO` | in `FOR`, slices, `CASE`, `MOVE`, `COPY` | [FOR, NEXT](#for-next), [Strings](#strings), [SELECT CASE](#select-case), [MOVE](#move), [COPY](#copy) |
| `TRACE` | statement | [TRACE](#trace) |
| `UNTIL` | statement | [REPEAT, UNTIL](#repeat-until) |
| `USR` | function | [USR](#usr) |
| `VAL` | function | [STR$, VAL, VAL$](#str-val-val) |
| `VAL$` | function | [STR$, VAL, VAL$](#str-val-val) |
| `VERIFY` | statement | [VERIFY](#verify) |
| `WAIT` | statement | [WAIT](#wait) |
| `WEND` | statement | [WHILE, WEND](#while-wend) |
| `WHILE` | statement | [WHILE, WEND](#while-wend) |
