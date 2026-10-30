**English** · [Español](../../../es/basic/tutorial/13-files-and-the-sd-card.md)

# 13. Files and the SD card

**In this chapter you will learn** to save and load your programs, to
organise them in folders, to use the built-in examples of the `ROM:` drive,
and to write and read your own text files, for example to keep the best
score of a game. You will also learn to catch errors with `ON ERROR`.

## The SD card

ZX-V keeps its files on an **SD card**, as the Spectrum kept them on
cassette tapes. In the emulator, the SD card is a folder on your PC: the
**ZX-V** folder inside your *Documents* folder (the web version keeps it in
the browser). See [the emulator guide](../../emulator.md) for the details.

## Saving and loading programs

Type a short program and save it with `SAVE` and a name:

```
10 PRINT "I was saved and loaded!"
SAVE "hello"
```

The program is now in the file `hello.tap` on the card: ZX-V adds the
`.tap` ending by itself. `CAT` lists the files of the card:

```
NEW
CAT
```

`NEW` erases the program from memory, but the file is still on the card.
Load it back and run it:

```
LOAD "hello"
RUN
```

If you add `LINE` and a line number when you save, the program will start by
itself, from that line, every time it is loaded:

```
SAVE "hello" LINE 10
```

**If you know the Spectrum:** `.tap` files are the same as the Spectrum's
tape files, so you can open your ZX-V programs in a Spectrum emulator, and
load BASIC programs from Spectrum `.tap` and `.tzx` files.

## Folders

As you save more programs, you can put them in **folders**:

| Command | What it does |
|---|---|
| `MKDIR "games"` | Creates the folder `games` |
| `CD "games"` | Goes into it: from now on, `SAVE`, `LOAD` and `CAT` work there |
| `CD ".."` | Goes back up one folder |
| `CD "/"` | Goes back to the main folder of the card |
| `PRINT DIR$` | Shows the current folder |
| `CAT "*.tap"` | Lists only the files whose names end in `.tap` (`*` means "anything") |
| `ERASE "old.tap"` | Deletes a file. Careful: it cannot be undone |

You can also give the folder in the name: `LOAD "games/hello"`.

## The `ROM:` drive: the built-in examples

Besides the card, ZX-V has a second drive that cannot be written on, the
**`ROM:` drive**, with example programs that come with the machine:

```
CAT "ROM:"
```

To load one, put `ROM:` in front of its name: `LOAD "ROM:graphics"`. A program
that loads more files, like the platform game (`LOAD "ROM:platformer"`),
looks for them in the current folder first and, if they are not there, in
its own folder: that is how it finds its graphics on the `ROM:` drive, and a
file with the same name in your folder comes before its own (see
[LOAD](../reference.md#load)). You can also go into the drive with
`CD "ROM:"`; `CD "SD:"` takes you back to the card. Trying to save on the
`ROM:` drive gives `V Disk error (not allowed)`.

## Text files

A program can also write its own files: a list, a diary, the best scores.
Files are opened on a **stream**, a numbered channel from 4 to 15.

- `OPEN #4,"notes.txt" OUT` creates the file (or empties it, if it exists)
  to write in it.
- `PRINT #4;...` writes in the file as `PRINT` writes on the screen. A comma
  starts a new line in the file.
- `OPEN #4,"notes.txt" IN` opens it to read it.
- `INPUT #4;...` reads from the file, one line for each variable. With `LINE
  a$`, the whole line goes into `a$`.
- `CLOSE #4` closes it. Always close the files you open.

```
10 OPEN #4,"notes.txt" OUT
20 PRINT #4;"Shopping list"
30 PRINT #4;"bread","milk"
40 CLOSE #4
50 OPEN #4,"notes.txt" IN
60 INPUT #4;LINE a$,LINE b$,LINE c$
70 CLOSE #4
80 PRINT a$: PRINT b$: PRINT c$
```

The file `notes.txt` is a normal text file: you can open it on your PC with
any text editor.

## When something goes wrong: `ON ERROR`

Reading a file that does not exist stops the program with `T File not
found`. A good program should not stop for that. `ON ERROR GO TO line` tells
the machine where to jump if there is an error, instead of stopping:

```
10 ON ERROR GO TO 100
20 LOAD "nothing"
30 STOP
100 PRINT "Error ";ERR;" in line ";ERL
```

`ERR` is the code of the error (29 is `T File not found`; the
[reference](../reference.md#error-messages) has the full list) and `ERL` is
the line where it happened. `ON ERROR GO TO 0` turns error catching off. The
catching works only once: after an error, put `ON ERROR` again if you want to
keep catching them.

## Keeping the best score

Here is everything together: a program that remembers the best score
between one run and the next, in the file `best.txt`:

```
10 LET best=0
20 ON ERROR GO TO 40
30 OPEN #4,"best.txt" IN: INPUT #4;best
40 ON ERROR GO TO 0: CLOSE #4
50 PRINT "Best score so far: ";best
60 INPUT "Your score? ";s
70 IF s>best THEN PRINT "A new record!": OPEN #4,"best.txt" OUT: PRINT #4;s: CLOSE #4
```

The first time, `best.txt` does not exist: line 30 fails, the program jumps
to line 40 and `best` stays at 0. From then on, it reads the score saved
last time. Run it twice and see.

## Try it

1. Create a folder `tests`, save a program in it and load it from the main
   folder.
2. Write a "guest book": the program asks for a name and adds it to a file;
   then it prints all the names in the file.

<details>
<summary>Hint</summary>

`OUT` empties the file, so to add a name, read all the names first (into an
array, for example), then write them all again plus the new one. To read
until the end of the file, use `ON ERROR`: when there are no more lines,
`INPUT #4` gives error 8, `End of file`.

</details>

---

**Reference:** [Files and the SD card](../reference.md#files-and-the-sd-card),
[`SAVE`](../reference.md#save), [`LOAD`](../reference.md#load),
[`CAT`](../reference.md#cat), [`CD`, `DIR$`](../reference.md#cd-dir),
[`OPEN`, `CLOSE`](../reference.md#open-close),
[`ON ERROR`](../reference.md#on-error), [`ERR`, `ERL`](../reference.md#err-erl).

**Next:** [14. A complete game](14-a-complete-game.md) ·
**Previous:** [12. Sprites and tile maps](12-sprites-and-tile-maps.md)
