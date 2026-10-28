**English** · [Español](../../../es/basic/tutorial/04-decisions.md)

# 4. Decisions

**In this chapter you will learn** to make the program choose what to do
with `IF`, `THEN`, `ELSE` and `SELECT CASE`, to compare numbers and strings, and to jump to
another place with `GO TO` and labels. At the end you will write a
number-guessing game.

## `IF` ... `THEN`

`IF` checks a **condition**. If it is true, the machine carries out what
comes after `THEN`; if not, it goes on with the next line.

```
10 INPUT "How old are you? ";age
20 IF age>=18 THEN PRINT "You can vote."
30 IF age<18 THEN PRINT "You can vote in ";18-age;" years."
```

With 12, it prints `You can vote in 6 years.`

These are the signs for comparing:

| Sign | Means |
|---|---|
| `=` | equal to |
| `<>` | different from |
| `<` | less than |
| `>` | greater than |
| `<=` | less than or equal to |
| `>=` | greater than or equal to |

They work with strings too: `"apple"<"banana"` is true, because it compares
in alphabetical order (capitals come before small letters).

## `ELSE`: one thing or the other

`ELSE` says what to do when the condition is false:

```
10 INPUT "Password? ";p$
20 IF p$="zxv" THEN PRINT "Welcome!" ELSE PRINT "Wrong password."
```

## Several things at once

Everything after `THEN` up to the end of the line depends on the condition,
even if there are several statements separated by `:`:

```
10 INPUT "A number from 1 to 10? ";n
20 IF n<1 OR n>10 THEN PRINT "That is not from 1 to 10.": STOP
30 IF n=7 THEN PRINT "Lucky seven!": PRINT "Well done."
40 PRINT "You chose ";n
```

`STOP` ends the program there, with the message `9 STOP statement`.

## Blocks: an `IF` over several lines

When what depends on the condition does not fit comfortably on one line,
leave `THEN` at the end and write the statements on the following lines, up
to `END IF`. `ELSE` and `ELSE IF` (another condition, tried if the one before
was not true) separate the parts:

```
10 INPUT "Mark? ";n
20 IF n>=9 THEN
30 PRINT "Excellent"
40 PRINT "Well done!"
50 ELSE IF n>=5 THEN
60 PRINT "Pass"
70 ELSE
80 PRINT "Fail"
90 END IF
100 PRINT "The end"
```

Only one of the parts runs, and then the program carries on after `END IF`.
There is no need to indent the lines: the editor does it when it stores them,
so that you can see what goes inside what.

## Choosing among many: `SELECT CASE`

To compare one value with many, `SELECT CASE` is clearer than a chain of
`ELSE IF`. Each `CASE` has the values it matches: one, several separated by
commas, a range with `TO` or a comparison such as `< 0`. `CASE ELSE` catches
the rest:

```
10 INPUT "Month (1-12)? ";m
20 SELECT CASE m
30 CASE 12, 1, 2: PRINT "Winter"
40 CASE 3 TO 5: PRINT "Spring"
50 CASE 6 TO 8: PRINT "Summer"
60 CASE 9 TO 11: PRINT "Autumn"
70 CASE ELSE: PRINT "There is no such month"
80 END SELECT
```

It works with strings too: `SELECT CASE r$` with `CASE "y", "Y"`.

## `AND`, `OR`, `NOT`

To join conditions:

- `a AND b` is true when both are true.
- `a OR b` is true when at least one is true.
- `NOT a` is true when `a` is false.

```
10 LET t=20
20 IF t>15 AND t<25 THEN PRINT "Nice weather"
30 IF NOT t>30 THEN PRINT "Not too hot"
```

Behind the scenes, a true condition is the number 1 and a false one is 0:
`PRINT 3>2` writes `1`. You will make use of this later.

## Jumping: `GO TO` and labels

You saw `GO TO` in chapter 1: it sends the program to another line. Together
with `IF`, it lets you repeat something until it comes out right.

Instead of a line number you can jump to a **label**: a name that starts with
`@`, placed at the start of a statement. Labels make the program easier to
read, and they keep working if you renumber the lines.

Here is a complete game. The machine thinks of a number from 1 to 100 and
you have to guess it:

```
10 RANDOMIZE
20 LET secret=INT (RND*100)+1
30 LET tries=0
40 @ask: INPUT "Guess my number (1-100): ";g
50 LET tries=tries+1
60 IF g<secret THEN PRINT g;" is too low": GO TO @ask
70 IF g>secret THEN PRINT g;" is too high": GO TO @ask
80 PRINT "Yes! It was ";secret
90 PRINT "You needed ";tries;" tries"
```

The line with `@ask` marks the place to go back to. Every wrong guess sends
the program back there; when the guess is right, neither line 60 nor line 70
jumps, and the program reaches line 80.

## Try it

1. Write a program that asks for a number and says whether it is even or
   odd.
2. Add a limit to the guessing game: if you have not guessed it after 7
   tries, the program tells you the number and stops.

<details>
<summary>Hint</summary>

A number `n` is even when `n/2` has no decimals, that is, when
`INT (n/2)=n/2`. For the second, check `tries` before going back to `@ask`:
a new line `55 IF tries=7 AND g<>secret THEN ...` will do.

</details>

<details>
<summary>A solution for the first one</summary>

```
10 INPUT "A number? ";n
20 IF INT (n/2)=n/2 THEN PRINT n;" is even" ELSE PRINT n;" is odd"
```

</details>

---

**Reference:** [`IF`, `THEN`, `ELSE`](../reference.md#if-then-else),
[`SELECT CASE`](../reference.md#select-case),
[True and false](../reference.md#true-and-false),
[`GO TO`](../reference.md#go-to), [`@label`](../reference.md#label),
[`STOP`](../reference.md#stop).

**Next:** [5. Loops](05-loops.md) ·
**Previous:** [3. Text and input](03-text-and-input.md)
