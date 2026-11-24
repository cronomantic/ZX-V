**English** · [Español](README.es.md)

# Nave 3D

A **3D** example in ZX Basic: a ship of flat faces that turns, with light and
pattern shading, among stars that come towards the camera. It uses
`zxv3d.bas`, the simple 3D of the [SDK libraries](../../../docs/en/sdk/libraries.md#simple-3d-zxv_3dh)
in the ZX Basic fork, and the model comes from an `.obj` file
(Blender's format) with the SDK converter.

| File | Contents |
|---|---|
| `nave3d.bas` | The program |
| `nave.obj` | The ship, as Wavefront `.obj`: 28 vertices and 22 faces |
| `nave.bas` | The ship in BASIC, made by `obj2c.py --bas` from `nave.obj` |

## How it is made

- **The model:** `obj2c.py --bas` writes two arrays (`nave_v`, the vertices;
  `nave_f`, the faces with their colours) and a `SUB nave_model(n)` that sets
  them as model `n`. The colours come from the materials of the `.obj`: one
  called `zxv_1_9` is the ZX-V blue (1, the dark one; 9, the light one);
  `zxv_2_10_two`, the fin, is also seen from both sides.
- **A model for the painter:** faces are painted from far to near by their
  mean distance, so the wings touch the body without going into it, the fin
  is a single face and the top of the body is split right under it.
  Otherwise, now and then a wing or the fin shows where it should not.
- **Each frame:** `BEGIN3D`, one `ADD3D` per object (the model, where it is
  and how it is turned) and `DRAW3D`, which paints it all from back to front
  on the hidden page. `ADD3D` returns the faces that are seen.
- **The stars:** they are points of the world; `PROJECT3D` says whether they
  are in front of the camera, and `PROJ3DX` and `PROJ3DY` the pixel they
  fall on.
- **Double buffer:** everything is drawn on the hidden page with `SCREEN` and
  shown with `PAGE`, as in Stardust.

To change the ship, edit `nave.obj` (or make another one in Blender and
export it as `.obj`) and make `nave.bas` again:

```
python sdk/tools/obj2c.py nave.obj nave.bas nave --size 4 --bas
```

`--size 4`: the longest side of the ship measures 4 units of the world.

## Building it

It needs the ZX Basic fork (`cronomantic/zxbasic`), v1.19.0-zxv.7 or later,
and the RISC-V C compiler, the same one the SDK uses:

```
zxbc --arch zxv nave3d.bas
```

The result is `nave3d.zxv`: copy it to the card and load it with
`LOAD "nave3d"`.

| Control | What it does |
|---|---|
| Cursor left and right, or the joystick | Turn faster one way or the other |
| Cursor up and down, or the joystick | Tilt the ship |
| Space or fire | Faces or only edges (wireframe) |
