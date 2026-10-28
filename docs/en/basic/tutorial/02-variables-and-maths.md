**English** · [Español](../../../es/basic/tutorial/02-variables-and-maths.md)

# 2. Variables and maths

**In this chapter you will learn** to keep numbers in **variables** with
`LET`, to do sums of all kinds, to arrange what `PRINT` writes, and to get
random numbers to roll dice.

## Variables: boxes with a name

A **variable** is a box with a name where the machine keeps a number. You put
a value in it with `LET`:

```
10 LET apples=5
20 LET pears=3
30 PRINT "Fruit: ";apples+pears
40 LET apples=apples-1
50 PRINT "Apples left: ";apples
```

This prints `Fruit: 8` and `Apples left: 4`. Line 40 looks strange if you
read it as maths, but in BASIC it means "take the value of `apples`,
subtract 1 and put the result back into `apples`". That is the way to make a
variable change: count points, lose lives, move things.

Rules for the names of number variables:

- They start with a letter and can go on with letters and digits: `x`,
  `score`, `level2`.
- Capitals and small letters are the same: `Score` and `SCORE` are the same
  box. Spaces inside the name are ignored too.
- A name cannot be just a keyword: `LET print=1` is a mistake.
- A variable must be given a value before you use it. Otherwise you get
  `2 Variable not found`, as you saw in chapter 1.

## Doing sums

These are the signs for maths:

| Sign | Meaning | Example | Result |
|---|---|---|---|
| `+` | add | `7+3` | `10` |
| `-` | subtract | `7-3` | `4` |
| `*` | multiply | `7*3` | `21` |
| `/` | divide | `7/2` | `3.5` |
| `^` | power | `2^10` | `1024` |

As in school maths, `^` goes first, then `*` and `/`, and finally `+` and
`-`. So `2+3*4` is 14. Use brackets to change the order: `(2+3)*4` is 20.

The machine also knows many **functions**, which take a number and give
back another one:

| Function | What it gives | Example | Result |
|---|---|---|---|
| `SQR` | square root | `SQR 16` | `4` |
| `INT` | the whole number below | `INT 7.9` | `7` |
| `ABS` | the number without its sign | `ABS -5` | `5` |
| `PI` | the number π | `PI` | `3.1415927` |

Here is a program that uses some of them to work out the area of a circle:

```
10 LET r=10
20 LET area=PI*r^2
30 PRINT "Radius ";r
40 PRINT "Area ";area
50 PRINT "Rounded ";INT (area+0.5)
```

It prints `Radius 10`, `Area 314.15927` and `Rounded 314`. Adding 0.5 before
`INT` rounds to the nearest whole number. Notice the brackets after `INT`:
without them, `INT area+0.5` would mean `(INT area)+0.5`, because a function
only takes what comes right after it.

Very big or very small numbers are written with an `E`: `PRINT 2^40` shows
`1.0995116E+12`, which means 1.0995116 × 10¹².

## Arranging what `PRINT` writes

You can put several things in one `PRINT`, separated by these signs:

| Sign | Effect |
|---|---|
| `;` | Writes the next thing right after, with no gap |
| `,` | Jumps to the next half of the line (to column 0 or 16) |
| `'` | Starts a new line |

```
10 PRINT "A";"B";"C"
20 PRINT "A","B"
30 PRINT "A"'"B"
40 PRINT "Total: ";10*3
```

Line 10 writes `ABC`, line 20 writes `A` and `B` in two columns, line 30
writes them on two lines, and line 40 writes `Total: 30`.

You can also put several statements on the same line, separated by `:`.
They run from left to right:

```
10 LET total=10: LET total=total*2: PRINT total
```

## Random numbers

`RND` gives a different number each time, between 0 and 1 (never 1 itself).
To roll a dice, multiply by 6, keep the whole part and add 1, so that you get
1 to 6:

```
10 RANDOMIZE
20 LET dice=INT (RND*6)+1
30 PRINT "You rolled a ";dice
```

Run it several times. `RANDOMIZE` shuffles the random numbers, so that each
run starts somewhere different. Put it once at the start of programs that use
`RND`.

## Try it

1. Write a program that turns a temperature in degrees Celsius into
   Fahrenheit. The formula is F = C × 9 / 5 + 32. With 25 °C you should get
   77.
2. Change the dice program to roll two dice and print both and their total.

<details>
<summary>Hint</summary>

For the first one, keep the Celsius value in a variable (`LET c=25`) and
work out the other with `LET f=c*9/5+32`. For the second, use two variables,
for example `d1` and `d2`, each with its own `INT (RND*6)+1`.

</details>

<details>
<summary>A solution for the first one</summary>

```
10 LET c=25
20 LET f=c*9/5+32
30 PRINT c;" C is ";f;" F"
```

</details>

---

**Reference:** [`LET`](../reference.md#let),
[Variables](../reference.md#variables),
[Expressions and operators](../reference.md#expressions-and-operators),
[Maths](../reference.md#maths),
[`RND`, `RANDOMIZE`](../reference.md#rnd-randomize),
[`PRINT`](../reference.md#print).

**Next:** [3. Text and input](03-text-and-input.md) ·
**Previous:** [1. First steps](01-first-steps.md)
