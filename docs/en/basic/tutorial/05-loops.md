**English** · [Español](../../../es/basic/tutorial/05-loops.md)

# 5. Loops

**In this chapter you will learn** to repeat things with `FOR` ... `NEXT`,
`WHILE` ... `WEND` and `REPEAT` ... `UNTIL`, to put one loop inside another,
and to wait with `PAUSE`.

## Counting with `FOR` ... `NEXT`

A **loop** repeats a group of lines. `FOR` is the loop that counts:

```
10 FOR i=1 TO 5
20 PRINT "Lap ";i
30 NEXT i
40 PRINT "Finished!"
```

`FOR i=1 TO 5` gives `i` the value 1 and runs the lines that follow. When it
reaches `NEXT i`, it adds 1 to `i` and goes back, until `i` has gone past 5.
So it prints `Lap 1` to `Lap 5`, and then `Finished!`.

The variable that counts (here `i`) must have a **one-letter name**: `FOR
lap=1 TO 5` is a mistake.

`STEP` changes how much is added each time, and it can be negative or have
decimals:

```
10 FOR i=10 TO 0 STEP -2
20 PRINT i;" ";
30 NEXT i
40 PRINT
50 FOR x=0 TO 1 STEP 0.25: PRINT x;" ";: NEXT x
```

This prints `10 8 6 4 2 0` and, on the next line, `0 0.25 0.5 0.75 1`. A
`PRINT` with nothing after it just ends the line.

Any of the numbers can be a variable or a sum. Here is a times table:

```
10 INPUT "Which times table? ";t
20 FOR i=1 TO 10
30 PRINT i;" x ";t;" = ";i*t
40 NEXT i
```

## Loops inside loops

You can put one loop inside another. Each one needs its own letter:

```
10 FOR r=1 TO 5
20 FOR c=1 TO r
30 PRINT "*";
40 NEXT c
50 PRINT
60 NEXT r
```

The outside loop counts the rows (`r`); for each row, the inside loop prints
`r` stars. The result is a triangle:

```
*
**
***
****
*****
```

## `WHILE` ... `WEND`: repeat while something is true

Sometimes you do not know how many times to repeat. `WHILE` repeats while a
condition is true, and checks it **before** each turn:

```
10 LET money=100: LET years=0
20 WHILE money<200
30 LET money=money*1.1: LET years=years+1
40 WEND
50 PRINT "Doubled after ";years;" years"
```

With 10 % interest every year, your money doubles in 8 years. If the
condition is false from the start, the lines inside are never run.

## `REPEAT` ... `UNTIL`: repeat until something happens

`REPEAT` checks its condition **after** each turn, so the lines inside run
at least once. It repeats until the condition becomes true:

```
10 RANDOMIZE
20 LET rolls=0
30 REPEAT
40 LET d=INT (RND*6)+1: LET rolls=rolls+1
50 PRINT d;" ";
60 UNTIL d=6
70 PRINT: PRINT "A six after ";rolls;" rolls"
```

With `REPEAT`, the guessing game from chapter 4 no longer needs `GO TO`:

```
10 RANDOMIZE
20 LET secret=INT (RND*100)+1: LET tries=0
30 REPEAT
40 INPUT "Guess my number (1-100): ";g
50 LET tries=tries+1
60 IF g<secret THEN PRINT g;" is too low"
70 IF g>secret THEN PRINT g;" is too high"
80 UNTIL g=secret
90 PRINT "Yes! ";tries;" tries"
```

Which loop should you use? `FOR` when you know how many times; `WHILE` when
it may not need to run at all; `REPEAT` when it has to run at least once.

## Waiting: `PAUSE`

`PAUSE n` waits `n` fiftieths of a second, so `PAUSE 50` is one second.
`PAUSE 0` waits until a key is pressed. A pressed key also cuts any pause
short.

```
10 FOR n=10 TO 1 STEP -1
20 PRINT n
30 PAUSE 50
40 NEXT n
50 PRINT "Lift off!"
```

**If you know the Spectrum:** `WHILE`, `WEND`, `REPEAT` and `UNTIL` are ZX-V
extensions; `FOR`, `NEXT` and `PAUSE` work as on the 48K.

## Try it

1. Print the numbers from 1 to 20 with their squares (the number times
   itself).
2. Write a program that asks for a word and prints it backwards: with
   `HELLO` it prints `OLLEH`.

<details>
<summary>Hint</summary>

For the second, `w$(i)` is letter number `i`. Go through the letters from
`LEN w$` down to 1 with `STEP -1`, and print each one followed by `;`.

</details>

<details>
<summary>A solution for the second one</summary>

```
10 INPUT "A word? ";w$
20 FOR i=LEN w$ TO 1 STEP -1
30 PRINT w$(i);
40 NEXT i
```

</details>

---

**Reference:** [`FOR`, `NEXT`](../reference.md#for-next),
[`WHILE`, `WEND`](../reference.md#while-wend),
[`REPEAT`, `UNTIL`](../reference.md#repeat-until),
[`PAUSE`](../reference.md#pause).

**Next:** [6. Arrays and data](06-arrays-and-data.md) ·
**Previous:** [4. Decisions](04-decisions.md)
