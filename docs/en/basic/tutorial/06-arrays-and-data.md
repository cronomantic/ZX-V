**English** · [Español](../../../es/basic/tutorial/06-arrays-and-data.md)

# 6. Arrays and data

**In this chapter you will learn** to keep many values under one name with
**arrays** (`DIM`), and to store lists of values inside the program with
`READ`, `DATA` and `RESTORE`.

## Arrays: a row of boxes

Suppose you want to keep five scores. Five variables (`s1`, `s2`...) would
work, but they cannot be used in a loop. An **array** is a row of numbered
boxes that share one name. You create it with `DIM`, saying how many boxes
it has:

```
10 DIM s(5)
20 FOR i=1 TO 5
30 INPUT "Score? ";s(i)
40 NEXT i
50 LET total=0
60 FOR i=1 TO 5: LET total=total+s(i): NEXT i
70 PRINT "Scores: ";
80 FOR i=1 TO 5: PRINT s(i);" ";: NEXT i
90 PRINT: PRINT "Average: ";total/5
```

`DIM s(5)` makes the boxes `s(1)` to `s(5)`, all starting at 0. The number in
brackets is the **index**, and it can be a variable: `s(i)` is "box number
`i`". That is what makes arrays so useful with `FOR`.

Some rules:

- The name of an array is a single letter: `s`, `a`, `b`.
- The boxes are numbered from 1. Asking for a box that does not exist, like
  `s(6)` or `s(0)`, gives `3 Subscript wrong`.
- An array can have more dimensions: `DIM b(8,8)` is a board of 8 × 8 boxes,
  and `b(2,3)` is row 2, column 3.

Here an array counts how many times each face of a dice comes up in 600
throws:

```
10 RANDOMIZE
20 DIM c(6)
30 FOR i=1 TO 600
40 LET d=INT (RND*6)+1
50 LET c(d)=c(d)+1
60 NEXT i
70 FOR d=1 TO 6: PRINT d;": ";c(d): NEXT d
```

Each face comes up about 100 times.

## Arrays of strings

String arrays need one more number: how long each string is. `DIM d$(7,9)`
makes 7 strings of exactly 9 characters each. A shorter text is filled with
spaces at the end, and a longer one is cut.

## `READ` and `DATA`: lists inside the program

`DATA` lines hold a list of values. `READ` takes the next value from the list
and puts it in a variable:

```
10 DIM d$(7,9)
20 FOR i=1 TO 7: READ d$(i): NEXT i
30 FOR i=1 TO 7: PRINT i;" ";d$(i);"|": NEXT i
40 DATA "Monday","Tuesday","Wednesday","Thursday"
50 DATA "Friday","Saturday","Sunday"
```

The `|` at the end shows the spaces that fill each day up to 9 characters.

`DATA` lines can be anywhere in the program; the machine skips them when it
runs it. Values are read in order, line after line. A `DATA` line can mix
numbers and strings, as long as each `READ` uses the right kind of variable:

```
10 FOR i=1 TO 3
20 READ n$,age
30 PRINT n$;" is ";age
40 NEXT i
50 RESTORE
60 READ n$: PRINT "First again: ";n$
70 DATA "Ada",36,"Alan",41,"Grace",85
```

`RESTORE` goes back to the beginning of the list, so the next `READ` takes
the first value again. `RESTORE 100` (or `RESTORE @label`) goes to the
`DATA` in that line. Reading more values than there are gives
`E Out of DATA`.

## Try it

1. Keep the names of the months in `DATA`, and write a program that asks for
   a number from 1 to 12 and prints the name of that month.
2. Fill an array with 10 random numbers from 1 to 100 and print the largest.

<details>
<summary>Hint</summary>

For the first, read the months into `DIM m$(12,9)` with a loop, then print
`m$(n)`. For the second, keep the largest so far in a variable, start it at
0 and compare each number with it: `IF a(i)>big THEN LET big=a(i)`.

</details>

<details>
<summary>A solution for the second one</summary>

```
10 RANDOMIZE
20 DIM a(10): LET big=0
30 FOR i=1 TO 10
40 LET a(i)=INT (RND*100)+1: PRINT a(i);" ";
50 IF a(i)>big THEN LET big=a(i)
60 NEXT i
70 PRINT: PRINT "The largest is ";big
```

</details>

---

**Reference:** [`DIM`](../reference.md#dim),
[Arrays](../reference.md#arrays),
[`READ`, `DATA`, `RESTORE`](../reference.md#read-data-restore).

**Next:** [7. Procedures](07-procedures.md) ·
**Previous:** [5. Loops](05-loops.md)
