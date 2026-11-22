// The hero demo: the getting-started program, typed in the editor and run,
// drawn with the ROM's own 8x8 font (rom/font/zxv8x8.txt) on a 640x480 frame.
(() => {
  const FONT = '00000000000000000010101010001000002828000000000000287c28287c2800103c5038147810000062640810264600003048304a443a00001010000000000000081010101008000020101010102000000028107c102800000010107c1010000000000000181820000000007c000000000000000018180000020408102040000038444c5464380000103010101038000038440810207c00007c08180444380000081828487c0800007c4078044438000038407844443800007c0408101010000038443844443800003844443c0438000000181800181800000018180018182000000810201008000000007c007c0000000020100810200000384408100010000038445c5c403c00003844447c444400007844784444780000384440404438000078444444447800007c407840407c00007c407840404000003844404c4438000044447c444444000038101010103800000404040444380000444870484444000040404040407c0000446c5444444400004464544c44440000384444444438000078444478404000003844445448340000784444784844000038403804443800007c1010101010000044444444443800004444444428100000444444546c440000442810102844000044281010101000007c081020407c00003820202020380000402010080402000038080808083800001028440000000000000000000000ff0018242078207c00000038043c443c00004040784444780000003840404038000004043c44443c000000384478403800001820782020200000003c44443c043800404078444444000010003010103800000800080808483000404850605048000020202020201800000068545454540000007844444444000000384444443800000078444478404000003c44443c0406000058604040400000003c403804780000207820202018000000444444443c0000004444442810000000445454542800000044281028440000004444443c043800007c0810207c00001820602020180000101010101010000030080c0808300000003448000000003c4299a1a199423c';
  const cv = document.getElementById('demo');
  cv.width = 640; cv.height = 480;
  const g = cv.getContext('2d');
  const img = g.createImageData(640, 480);
  const px = new Uint32Array(img.data.buffer);
  const RGB = c => 0xff000000 | (parseInt(c.slice(5, 7), 16) << 16) | (parseInt(c.slice(3, 5), 16) << 8) | parseInt(c.slice(1, 3), 16);
  const COL = ['#000000', '#0000d7', '#d70000', '#d700d7', '#00d700', '#00d7d7', '#d7d700', '#d7d7d7']
    .map(RGB);
  const BLACK = COL[0], BLUE = COL[1], WHITE = COL[7], GREY = COL[7];
  const BX = 64, BY = 48;                      // border of the 640x480 frame
  
  const fill = (x, y, w, h, c) => {
    for (let j = y; j < y + h; j++) px.fill(c, j * 640 + x, j * 640 + x + w);
  };
  const glyph = (ch, x, y, s, ink, paper) => {
    const i = (ch.charCodeAt(0) - 32) * 16;
    for (let r = 0; r < 8; r++) {
      const b = parseInt(FONT.substr(i + r * 2, 2), 16);
      for (let k = 0; k < 8; k++) {
        const c = (b & (0x80 >> k)) ? ink : paper;
        if (c !== undefined) fill(x + k * s, y + r * s, s, s, c);
      }
    }
  };
  const text = (s, x, y, sc, ink, paper) => { [...s].forEach((ch, i) => glyph(ch, x + i * 8 * sc, y, sc, ink, paper)); };
  const plot = (x, y, c) => { if (x >= 0 && x < 256 && y >= 0 && y < 192) fill(BX + x * 2, BY + y * 2, 2, 2, c); };
  const circle = (cx, cy, r, c) => {          // midpoint circle, in 256x192 pixels
    let x = r, y = 0, e = 1 - r;
    while (x >= y) {
      for (const [a, b] of [[x, y], [y, x], [-y, x], [-x, y], [-x, -y], [-y, -x], [y, -x], [x, -y]]) plot(cx + a, cy + b, c);
      y++;
      if (e < 0) e += 2 * y + 1; else { x--; e += 2 * (y - x) + 1; }
    }
  };

  // The program of the getting-started guide in the page's language, as the
  // machine shows it with --lang (the free memory changes with its length)
  const ES = document.documentElement.lang === 'es';
  const HELLO = ES ? 'Hola, soy ZX-V' : 'Hello, I am ZX-V!';
  const FREE = ES ? '93717' : '93714';
  const PROG = [
    '10 BORDER 0: PAPER 0: INK 7: CLS',
    '20 PRINT "' + HELLO + '"',
    '30 FOR i=1 TO 7: CIRCLE INK i;128,88,i*10: NEXT i',
  ];
  const CPS = 22;                              // typing speed, characters per second
  const total = PROG.reduce((n, l) => n + l.length, 0);
  const T_TYPE = total / CPS, T_RUNKEY = T_TYPE + 0.9, T_RUN = T_RUNKEY + 0.7;
  const T_END = T_RUN + 7 * 0.35 + 3.5;
  let t0 = performance.now();

  // The editor in mode 0 (EDIT > Mode 0): 32 columns with the letters twice
  // as big, as in the real one: a line wraps to the rows below, and one of
  // exactly 32 characters leaves the cursor on the next row. The bar at the
  // bottom only shows the free memory.
  const CYAN_B = RGB('#00ffff');
  const RAINBOW_B = ['#ff0000', '#ffff00', '#00ff00', '#00ffff'].map(RGB);
  const at = (col, row) => [BX + col * 16, BY + row * 16];
  function editor(t) {
    fill(0, 0, 640, 480, GREY);
    let n = t >= T_TYPE ? total : Math.floor(t * CPS), row = 0, cur = [0, 0];
    for (const line of PROG) {
      const shown = line.slice(0, Math.max(0, n)); n -= line.length;
      for (let i = 0; i < shown.length; i++) glyph(shown[i], ...at(i % 32, row + (i / 32 | 0)), 2, BLACK);
      cur = [shown.length % 32, row + (shown.length / 32 | 0)];
      if (n < 0) break;
      row += (line.length / 32 | 0) + 1;
      cur = [0, row];
    }
    if (t >= T_TYPE) {                          // the program is in: now RUN
      const k = Math.min(3, Math.floor((t - T_TYPE) * 6));
      text('RUN'.slice(0, k), ...at(0, row), 2, BLACK);
      cur = [k, row];
    }
    fill(...at(...cur), 16, 16, (t * 2.5 | 0) % 2 ? BLUE : CYAN_B);
    const [bx, by] = at(0, 23);                 // the bar
    fill(bx, by, 512, 16, BLACK);
    text('ZX-V BASIC', ...at(1, 23), 2, WHITE);
    text(FREE, ...at(21, 23), 2, WHITE);
    RAINBOW_B.forEach((c, i) => {
      for (let r = 0; r < 8; r++) fill(BX + 2 * (223 - r + i * 8), by + 2 * r, 16, 2, c);
    });
  }
  function run(t) {
    fill(0, 0, 640, 480, BLACK);
    text(HELLO, BX, BY, 2, WHITE);
    const k = Math.min(7, Math.floor((t - T_RUN) / 0.35) + 1);
    for (let i = 1; i <= k; i++) circle(128, 88, i * 10, COL[i]);
    if (k === 7 && t > T_RUN + 7 * 0.35) text('0 OK, 30:3', BX, BY + 184 * 2, 2, WHITE);
  }
  // ?demo_t=5.6 stops the animation at that second (to check a frame). With
  // "reduce motion" in the system, it stays still on the finished program.
  const FIXED = parseFloat(new URLSearchParams(location.search).get('demo_t'));
  const still = matchMedia('(prefers-reduced-motion: reduce)');
  function step(now) {
    let t = (now - t0) / 1000;
    if (t > T_END) { t0 = now; t = 0; }
    const fixed = FIXED >= 0 ? FIXED : still.matches ? T_RUN + 8 * 0.35 : -1;
    if (fixed >= 0) t = fixed;
    if (t < T_RUN) editor(t); else run(t);
    g.putImageData(img, 0, 0);
    if (fixed < 0) requestAnimationFrame(step);
  }
  still.addEventListener('change', () => requestAnimationFrame(step));
  requestAnimationFrame(step);
})();
