**English** · [Español](../../../es/basic/tutorial/07-procedures.md)

# 7. Procedures

**In this chapter you will learn** to split a program into named pieces
with `DEF PROC` and `PROC`, to give them values to work with, to keep their
variables to themselves with `LOCAL`, and to make your own functions with
`DEF FN`. You will also meet `GO SUB`, the classic way to do it.

## Why split a program

As programs grow, it is easy to get lost among line numbers. The answer is
to give names to the pieces: "draw the board", "move the player", "show the
score". Each piece is a **procedure**. The main program becomes a short list
of names, and each procedure can be written and tested on its own.

**If you know the Spectrum:** procedures, `LOCAL` and labels are ZX-V
extensions. `GO SUB` and `DEF FN` work as on the 48K.

## `DEF PROC` and `PROC`

```
10 PROC title("ZX-V")
20 PROC title("BASIC")
30 PRINT "Done"
40 DEF PROC title(t$)
50 PRINT "*** ";t$;" ***"
60 END PROC
```

- `DEF PROC title(t$)` starts the definition of a procedure called `title`,
  and `END PROC` ends it.
- `PROC title("ZX-V")` **calls** it: the program jumps to the procedure, with
  `t$` set to `"ZX-V"`, runs it and, at `END PROC`, comes back to the
  statement after the call.
- When the program reaches a `DEF PROC` by itself (line 40, after line 30),
  it skips the whole procedure. That is why you can place procedures after
  the main program without anything strange happening.

The values in brackets are **parameters**. A procedure can have several,
numbers or strings, separated by commas: `DEF PROC box(n,c$)`. A string
parameter is a single letter followed by `$`. A procedure can also have no
parameters at all: `DEF PROC banner` and `PROC banner`.

Procedure names follow the same rules as number variables, so a name that is
exactly a keyword does not work: call it `dashes`, not `line`. Calling a
procedure that does not exist gives `Z PROC not found`.

## `LOCAL`: variables of your own

The parameters only exist inside the procedure: when it ends, a variable
with the same name outside gets its old value back. To have more variables
like that, list them after `LOCAL`:

```
10 LET i=99
20 PROC dashes(10)
30 PRINT "i is still ";i
40 DEF PROC dashes(n)
50 LOCAL i
60 FOR i=1 TO n: PRINT "-";: NEXT i
70 PRINT
80 END PROC
```

The procedure uses `i` for its loop, but the `i` of the main program is
still 99. Without `LOCAL i`, it would be 11.

## A program made of procedures

This is the shape of a well-organised program: the main part at the top,
which reads almost like a description, and the procedures below.

```
10 REM Main program
20 PROC banner
30 FOR i=1 TO 3: PROC stars(i*3): NEXT i
40 PROC banner
50 REM Procedures
60 DEF PROC banner
70 PRINT "=========="
80 END PROC
90 DEF PROC stars(n)
100 LOCAL i
110 FOR i=1 TO n: PRINT "*";: NEXT i
120 PRINT
130 END PROC
```

## A procedure that calls itself

A procedure can call other procedures, and even itself. That is called
**recursion**, and it solves some problems in a surprisingly short way. The
classic example is the Tower of Hanoi: move a pile of discs from peg 1 to peg
3, one disc at a time, never putting a big disc on a smaller one.

```
10 PROC hanoi(3,1,3,2)
20 DEF PROC hanoi(n,f,t,v)
30 IF n=0 THEN GO TO 70
40 PROC hanoi(n-1,f,v,t)
50 PRINT "Move a disc from ";f;" to ";t
60 PROC hanoi(n-1,v,t,f)
70 END PROC
```

To move `n` discs from `f` to `t`, using `v` as the spare peg: move `n-1`
discs out of the way to `v`, move the biggest one to `t`, and move the `n-1`
discs from `v` on top of it. It prints the 7 moves for 3 discs. Try
`PROC hanoi(4,1,3,2)`.

## Your own functions: `DEF FN`

A procedure does something; a **function** gives back a value. `DEF FN`
defines a function in one line. Its name is a single letter (with `$` if it
gives back a string), and you use it with `FN`:

```
10 DEF FN a(r)=PI*r*r
20 DEF FN m$(a$,b$)=a$+" & "+b$
30 PRINT FN a(10)
40 PRINT FN m$("salt","pepper")
```

## The classic way: `GO SUB`

Before procedures existed, BASIC programs used **subroutines**. `GO SUB`
jumps to a line, and `RETURN` comes back to just after the `GO SUB`:

```
10 GO SUB 100
20 PRINT "Back in the main program"
30 STOP
100 PRINT "In the subroutine"
110 RETURN
```

Here the `STOP` in line 30 is needed: without it, the program would carry on
into the subroutine. A subroutine has no name and no parameters, and all its
variables are shared, so for new programs procedures are easier. You will
find `GO SUB` in many old Spectrum programs.

## Try it

1. Write a procedure `square(n)` that prints a square of `n` × `n` stars,
   and call it with 3 and with 5.
2. Write a function `FN c(f)` that turns degrees Fahrenheit into Celsius
   (C = (F − 32) × 5 / 9) and print the value for 212.

<details>
<summary>Hint</summary>

For the first, use two `FOR` loops inside the procedure (like the triangle
of chapter 5), and declare their variables with `LOCAL`. For the second:
`DEF FN c(f)=(f-32)*5/9`.

</details>

<details>
<summary>A solution for the first one</summary>

```
10 PROC square(3): PRINT
20 PROC square(5)
30 DEF PROC square(n)
40 LOCAL r,c
50 FOR r=1 TO n
60 FOR c=1 TO n: PRINT "*";: NEXT c
70 PRINT
80 NEXT r
90 END PROC
```

</details>

---

**Reference:** [`DEF PROC`, `PROC`, `END PROC`](../reference.md#def-proc-proc-end-proc),
[`LOCAL`](../reference.md#local), [`DEF FN`, `FN`](../reference.md#def-fn-fn),
[`GO SUB`, `RETURN`](../reference.md#go-sub-return).

**Next:** [8. Colours and the screen](08-colours-and-the-screen.md) ·
**Previous:** [6. Arrays and data](06-arrays-and-data.md)
