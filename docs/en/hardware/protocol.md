**English** · [Español](../../es/hardware/protocol.md)

# The protocol of the EXT: drive and of remote loading

This document is for anyone who wants to write **a server for the `EXT:`
drive** (a program that serves a folder to the ZX-V: on a PC, a Raspberry Pi,
a phone, another machine...) or to talk to the board through its **J5**
header on their own. `zxv-link` (in [`sdk/tools/link`](../../../sdk/tools/link/),
MIT) is the reference implementation of the two sides that belong to the PC:
`zxvlink.c` (the frames), `linkcore.c` (remote loading) and `extserve.c` (the
`EXT:` server).

What `EXT:` is for the machine's programs (the paths, the names, the file
numbers) is in [the specification, §7.7](../../../dev/spec.md#77-ficheros-tarjeta-sd-disco-rom-y-unidad-ext) (in Spanish). Here, what
travels.

## The link

- **Today, the board's J5 header:** the CPU Pico's UART0 (GP12 goes out of
  the machine, GP13 comes in), at **921,600 baud**, 8 bits, no parity, 1 stop
  bit, no flow control, at **3.3 V**. With a 3.3 V USB-serial adapter: its RX
  to the board's TX, its TX to the RX, and ground to ground.
- **Later, others** (the Wi-Fi of a Pico 2 W): the same messages by another
  way. If there is a server on each one at the same time, the machine uses
  the one on the cable.

## The frames

Each message goes in a frame: its bytes in **COBS**, followed by a **zero**.
The receiver picks up again by itself at the next zero even if something is
lost. Before COBS, in little endian:

| Offset | What |
|---|---|
| +0 | The channel (below) |
| +1 | The type: the request's; in its reply, the same with bit 7 set (plus `0x80`) |
| +2 | The request number (0-255): set by the one who asks, and the reply repeats it |
| +3 | The data: up to **516 bytes** |
| ... | The **CRC-32** of everything before it (zlib's: polynomial `0xEDB88320`, starting and ending with `0xFFFFFFFF`) |

A frame with a wrong CRC, cut short or longer than fits is thrown away
without a reply: the one who asked repeats the request when its wait runs
out.

**Replies** always start with a **status** (i32): 0 or more, fine (in some,
a number: bytes, a position); negative, a machine error (`ZXV_E_*`, see
[Error codes](../sdk/reference.md#error-codes); for example, −1 does not
exist).

## The channels

| Channel | Value | Who asks | What for |
|---|---|---|---|
| `CTRL` | 0 | The outside device | Remote loading: sending files to the card, running, debugging; and announcing itself as the `EXT:` server |
| `LOG` | 1 | Nobody: the machine sends it on its own | The machine's report (UTF-8 text, type `0x40`): it is not answered |
| `EXT` | 2 | The machine | What programs ask of the `EXT:` drive |

Each side numbers its own requests, and on each channel there is **at most
one pending request**: the next one is not sent until the reply has arrived
(or until giving up).

## Announcing itself as a server

A server tells the machine it is there with a request on the `CTRL` channel:

| Type | Data | Reply |
|---|---|---|
| `SERVE` (10) | `u8`: 1 I serve `EXT:`, 0 I stop serving it | status; the machine's boot number (u32) |

- **Every second** while it serves. The machine considers the drive served
  as long as a `SERVE` arrives at least every **3 seconds**; if not, `EXT:`
  gives `ZXV_E_OFFLINE` without waiting for anyone.
- **The boot number** changes every time the machine starts (when switched
  on, with Ctrl+Alt+Del or when it restarts to run another program). If it
  changes, the server **closes everything it had open** for it: the machine
  no longer knows anything about that.
- When it stops serving (`SERVE` with 0, or when the port is closed), the
  same.

## What the machine asks (EXT channel)

The machine asks on the `EXT` channel and the server replies with the same
type plus `0x80` and the same number. **If the same request arrives again**
(the same number, after a reply that was lost), the server **repeats the
reply without doing it again**: writing, deleting or renaming twice would not
be the same. The machine waits **half a second** for each reply, asks up to
**three times** and, if there is still nothing, considers the drive lost
(`ZXV_E_OFFLINE`) until the next `SERVE`.

Open files and folders go by **slot**: the machine has 8 file slots (0-7)
and 4 folder slots (0-3) on `EXT:`, and says which one in each request. The
server keeps what is in each one.

| Type | Data | Reply, after the status |
|---|---|---|
| `OPEN` (1) | slot (u8), mode (u32: the `ZXV_O_*` of `file_open`), path | — |
| `READ` (2) | slot (u8), how many (u16, up to 512) | Status = bytes read (0, the end); after it, those bytes |
| `WRITE` (3) | slot (u8), the bytes (up to 512) | Status = bytes written |
| `CLOSE` (4) | slot (u8) | — |
| `SEEK` (5) | slot (u8), offset (i32), from (u8: 0 the start, 1 where it is, 2 the end) | Status = the new position |
| `STAT` (6) | path | Size (u32) and attributes (u32: bit 0, folder) |
| `REMOVE` (7) | path | — (deletes a file or an empty folder) |
| `RENAME` (8) | source path, a 0, destination path | — |
| `MKDIR` (9) | path | — |
| `OPENDIR` (10) | folder slot (u8), path | — |
| `READDIR` (11) | folder slot (u8) | Status = 1 and the entry: size (u32), attributes (u32) and name (without the zero); 0, no more |
| `CLOSEDIR` (12) | folder slot (u8) | — |

The machine keeps the current folder: changing folder is a `STAT` that
checks that it exists and is a folder.

## The paths

- **Absolute from the root** of what the server serves, with `/`
  (`/games/a.bas`; `/` is the root). The machine has already resolved `.`,
  `..` and the current folder: they do not arrive.
- **The server does not leave its folder.** A path that would try to (with
  `..`, `\` or a drive letter) is `ZXV_E_NAME`.
- **Names**, as on the card ([spec §7.7](../../../dev/spec.md#77-ficheros-tarjeta-sd-disco-rom-y-unidad-ext)): up to 63 printable ASCII
  characters, without `\ : * ? " < > |`, not ending in a space or a dot;
  paths of up to 255. **Case-insensitive**: `GAME.BAS` opens `game.bas`;
  when creating, the spelling the machine asks for is kept.
- **What does not follow the rules** (a name with accents, for example) does
  not appear in listings and cannot be opened. Hidden or system files do not
  appear either.
- **`READDIR`** gives the entries in alphabetical order, case-insensitive,
  like the card.

## What is recommended for a server

- **Not replacing what the machine did not write:** if an `OPEN` for
  writing, a `REMOVE` or a `RENAME` would spoil a file in the folder that the
  machine did not write (what was already there, with other contents), reply
  `ZXV_E_ACCESS`, unless whoever runs the server allows it. The reference one
  notes what the machine writes.
- **Writing whole or not at all:** keep what arrives aside and give it its
  name when it is closed, so that a cut does not leave the file half done.
- **Replying quickly:** the machine waits half a second for each reply.

## What the outside device asks (CTRL channel)

Remote loading, for anyone who wants to do it on their own (`zxv-link` is the
reference one):

| Type | Data | Reply, after the status |
|---|---|---|
| `HELLO` (1) | — | Protocol version (u16), whether BASIC is waiting for a command (u8), specification version (u32) and firmware version (text) |
| `OPEN` (2) | size (u32), path on the card | — (what arrives is written aside and gets its name when closed) |
| `DATA` (3) | position (u32), up to 512 bytes | The next position (u32). Out of order: `ZXV_E_INVAL` and the one it expects; with no file open, `ZXV_E_BADF` |
| `CLOSE` (4) | CRC-32 of everything (u32) | — (if it does not match, `ZXV_E_IO` and nothing is written) |
| `RUN` (5) | the text that is typed, with Enter after it | — (with a program running, status 1: below) |
| `STAT` (6) | path on the card | Size (u32) and CRC-32 (u32) |
| `READY` (7) | — | — (when BASIC is waiting for a command; with a program running, status 1) |
| `DBG_READ` (8) | from (u16), how many (u16) | Those bytes of the BASIC debugger's mailbox |
| `DBG_WRITE` (9) | from (u16), the bytes | — |
| `SERVE` (10) | above | above |

**Status 1 in `RUN` and `READY`:** a program was running and the machine
restarts completely, as with Ctrl+Alt+Del; what was asked is not done. It
answers again after a boot (one or two seconds): send `HELLO` until it says
that BASIC is waiting for a command, and ask again.

## The versions

`HELLO` gives the machine's protocol version. **This is 4:** the `EXT:` drive
(the `EXT` channel and `SERVE`). Version 3 brought the machine restart in
`RUN` and `READY`; version 2, `STAT`, `READY` and the debugger. An `EXT:`
server needs a machine with 4 or later.
