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
- **The network, if the CPU Pico is a Pico 2 W** (D106): the same frames,
  each one in a signed UDP packet ([below](#over-the-network-wi-fi)). If there
  is an `EXT:` server on each path at the same time, the machine uses the one
  on the cable.

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
| `HELLO` (1) | — | Protocol version (u16), whether BASIC is waiting for a command (u8: 1 yes, 0 no, 2 it is updating its firmware), specification version (u32) and firmware version (text) |
| `OPEN` (2) | size (u32), path on the card | — (what arrives is written aside and gets its name when closed) |
| `DATA` (3) | position (u32), up to 512 bytes | The next position (u32). Out of order: `ZXV_E_INVAL` and the one it expects; with no file open, `ZXV_E_BADF` |
| `CLOSE` (4) | CRC-32 of everything (u32) | — (if it does not match, `ZXV_E_IO` and nothing is written) |
| `RUN` (5) | the text that is typed, with Enter after it | — (with a program running, status 1: below) |
| `STAT` (6) | path on the card | Size (u32) and CRC-32 (u32) |
| `READY` (7) | — | — (when BASIC is waiting for a command; with a program running, status 1) |
| `DBG_READ` (8) | from (u16), how many (u16) | Those bytes of the BASIC debugger's mailbox |
| `DBG_WRITE` (9) | from (u16), the bytes | — |
| `SERVE` (10) | above | above |
| `UPDATE` (11) | — | — (the machine restarts and updates its firmware from `EXT:/zxv-update.bin`: below) |
| `LIST` (12) | from (u16), path | How many entries this reply has (in the status), whether there are more (u8) and each one: size (u32), attributes (u8: 1 folder), name length (u8) and the name |
| `GET` (13) | position (u32), path | How many bytes (in the status, up to 508), the file size (u32) and those bytes |
| `REMOVE` (14) | path | — (a file or an empty folder) |
| `RENAME` (15) | path, 0, new path | — (on the same drive; if something else already has the new name, `ZXV_E_EXIST`; the same name with other capitals changes it) |
| `MKDIR` (16) | path | — |

**The card and the ROM like an explorer** (`LIST`, `GET`, `REMOVE`,
`RENAME`, `MKDIR`, since version 6): paths carry their drive, `SD:` or
`ROM:` (with no drive, the card); not `EXT:`, which is the outside
device's own folder. `ROM:` is read only: the rest gives an error. `LIST`
returns the entries that fit in a frame, starting at number "from": if it
says there are more, ask again from the next one. `GET` returns a piece
from "position"; while the same file is asked for in a row, the machine
keeps it open, and any other request closes it. Running what is already on
the machine is `RUN` with the command: `CD "SD:/games": LOAD "game.zxv"`.

**Status 1 in `RUN` and `READY`:** a program was running and the machine
restarts completely, as with Ctrl+Alt+Del; what was asked is not done. It
answers again after a boot (one or two seconds): send `HELLO` until it says
that BASIC is waiting for a command, and ask again.

**`UPDATE`:** updating the firmware of both Picos from outside, with no card
(`zxv-link update`). First serve `EXT:` with the file at `/zxv-update.bin`;
BASIC must be waiting for a command. With status 0, the machine restarts into
its update screen and reads the file from `EXT:`. It restarts again after
writing the CPU firmware and once more at the end, so keep serving it (and
announcing) until the end. Meanwhile, `HELLO` says 2, the report (`LOG`
channel) tells the steps (a step that fails ends in ": error") and everything
else, except `SERVE`, answers `ZXV_E_ACCESS`. At the end the machine goes
back to BASIC: if `HELLO` gives the file's version, it went well. Errors:
`ZXV_E_OFFLINE` if it does not see the `EXT:` server; `ZXV_E_ACCESS` with a
program running, if the Pico has no partitions (flash it once over USB) or if
it is already updating.

## Over the network (Wi-Fi)

With a Pico 2 W in the CPU socket, the machine joins the network chosen in
**EDIT > Wi-Fi** and listens on **UDP port 5612**. Each frame above (in COBS,
with its zero) goes **whole in one packet**; the machine answers to the port
it came from. Everything except finding and pairing is **signed** with a key
that only the machine and that PC have, so nobody else on the network can
send it programs or pose as its `EXT:` server. It is not encrypted: what
travels can be read. The reference is `zlnet.h`, `zlnet.c` and `netpc.c` (the
PC) and `pico/common/netsvc.c` (the machine).

Each packet starts with `ZXV` and a letter, the type. Numbers in little
endian:

| Type | Who | What it carries |
|---|---|---|
| `F` | PC | Find: the PC's id (8 bytes) and the PC's network protocol version (u8). To the network broadcast or to an address |
| `f` | Machine | The machine's id (8), its boot (8, random at each power-up), whether it is paired with that PC (u8), the frame version (u16), its name (16, zero-padded: `ZX-V-1A2B`, from its MAC), the firmware (text: `ZX-V 0.11.0`), a 0 and the network protocol version (u8) |
| `P` | PC | Start pairing: the PC's id (8), its X25519 public key (32) and its name |
| `p` | Machine | State (u8), its public key (32) and its commitment (32) |
| `N` | PC | The PC's id (8) and its random number Npc (16). From the address and port of the `P` |
| `n` | Machine | State (u8) and its random number Nm (16) |
| `Q` | PC | The PC's id (8) and its proof (16, below): has the user answered? |
| `q` | Machine | State (u8) and, if it is 1, its proof (16) |
| `D` | PC | Data: the PC's id (8), the machine's boot (8), counter (u64), one frame and the tag (16) |
| `d` | Machine | Data, like `D` |
| `b` | Machine | The PC's id (8), its new boot (8), the counter of the `D` it answers (u64) and the tag (16): its answer to a `D` with another boot |

**The states:** 0 carry on (waiting for the user or the next step), 1
accepted, 2 rejected (or the box was closed), 3 the machine is not pairing,
4 it is pairing with another PC.

### Pairing

Like Bluetooth's numeric comparison. The user opens **EDIT > Wi-Fi > Pair a PC**
on the machine; otherwise it answers `P` with state 3.

1. The PC sends `P` with its public key Ppc.
2. The machine makes a new X25519 key Pm and a number Nm, and answers `p`
   with Pm and the commitment `SHA-256("ZX-V commit" | Nm | Pm | Ppc)`.
3. The PC sends `N` with its number Npc; the machine answers `n` with Nm. The
   PC checks that Nm gives the earlier commitment.
4. Both show the code: the first 4 bytes of
   `SHA-256("ZX-V code" | Ppc | Pm | Npc | Nm)`, as a big-endian number,
   modulo 1,000,000 (six digits).
5. The user checks that they match and accepts on the machine. The PC asks
   with `Q` until it gets 1 or 2. `Q` carries the proof of the key the PC
   has: the first 16 bytes of `HMAC-SHA-256(key, "ZX-V query" | PC's id |
   machine's id)`. The machine answers 1 only if it has stored that key (the
   user accepted), with its own proof, `"ZX-V paired"` instead of
   `"ZX-V query"`; the PC does not keep the key if it doesn't match. It
   answers 2 if it doesn't have it (rejected, or the box was closed), even if
   that PC was already paired with another key.
6. The pair's key is `SHA-256("ZX-V key" | X25519(own key, the other's) |
   Ppc | Pm | Npc | Nm)`. The machine keeps it with the PC's id and name (it
   remembers 8 PCs; one more forgets the oldest).

The labels are ASCII, without a zero. Since the machine commits to Nm before
it sees Npc, someone in the middle cannot search for keys that make both codes
match: they would have one chance in a million.

### The data

`D` (from the PC to the machine), `d` (from the machine to the PC) and `b`
are signed with the pair's key: the tag is the first 16 bytes of
`HMAC-SHA-256(key, "ZX-V net" | the network protocol version (u8) |
everything before it in the packet)`. The letter is signed, so a packet sent
back to whoever sent it is worthless, and with the version inside, a machine
and a PC of different versions accept nothing from each other. In both
directions it carries the PC's id and the machine's boot. **Counters never
repeat:** a
packet with one the machine has already seen, with a bad tag or from a PC it
doesn't know is dropped without an answer (a recorded packet cannot be
replayed). As a PC may run several programs at once (for example `zxv-link
serve` and `zxv-link log`), each with its own UDP port, their packets may
arrive out of order: the machine remembers the last 16 counters it accepted
and, below them, a floor per PC, and drops whatever is not above the floor.
The machine's starts at each boot; the PC's comes from its clock, so it keeps
going up when it is opened again: `zxv-link` uses milliseconds times 1024
with a random lane per program in bits 4 to 9, so two programs don't produce
the same counter in the same millisecond. And it **never goes down for a
machine**, even if the PC's clock goes back: `zxv-link` writes down the last
one it used with each (every second, or every 256 if the clock has gone
back) and carries on above it; while the clock is behind, one by one.

If the machine has restarted, the boot is no longer the one the PC has: to a
well-signed `D` with the old boot, the machine answers `b` with the new one
and that `D`'s counter. The PC only believes it if it answers one of its
latest `D`s and brings another boot; then it switches and repeats the last
one. A recorded `b` is useless: it no longer answers any of those.

**Each PC program gets its own** (each one is an address and a UDP port):

- **The reply** to a request goes to the program that made it, including the
  one to `RUN`, which comes when BASIC has typed the command, even if others
  speak in the meantime.
- **What programs ask of `EXT:`** goes to the program serving it: the one of
  the last `SERVE`.
- **The report** goes to everyone who has said something in the last 15
  seconds (4 at most). A program that only listens to the report sends a
  `HELLO` every 5 seconds to stay on the list.

**On the PC,** `zxv-link` keeps its id and the machines' keys in
`zxv-link-net.txt` (in `%APPDATA%\ZX-V` on Windows and `~/.config/ZX-V`
elsewhere), and the last counter for each machine next to it, in
`zxv-link-net-counters.txt`.

**The network protocol version** goes in `F` and `f`; this is version 2. A
machine that doesn't give it in `f` has version 1 (the first test builds).
With another version, `zxv-link` does not talk to the machine: it says they
are not compatible and which to update, the machine or `zxv-link`. The J5
frames don't change.

## The versions

`HELLO` gives the machine's protocol version. **This is 6:** `LIST`,
`GET`, `REMOVE`, `RENAME` and `MKDIR`. Version 5 brought `UPDATE`; version
4, the `EXT:` drive (the `EXT` channel and `SERVE`); version 3, the machine
restart in `RUN` and `READY`; version 2, `STAT`, `READY` and the debugger. An `EXT:` server needs a machine with 4 or later; updating
through J5, 5; and browsing the card, 6.
