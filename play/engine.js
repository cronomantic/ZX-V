/*
 * engine.js - La máquina en el navegador, para las dos páginas: la del
 * emulador (zxv.js) y la de un juego solo (play/play.js, D85). La máquina
 * (zxv-core.js, src/web_main.c) en un canvas, con el sonido por Web Audio, el
 * teclado, el ratón y los mandos del navegador, y la tarjeta SD guardada en el
 * navegador (IndexedDB). Lo de cada página (menús, avisos...) va por los
 * ganchos de ZXV.init.
 *
 * El ritmo: con sonido, se hacen los frames que faltan para tener unos 60 ms
 * en la cola (como zxv y zxv-sdl); sin sonido (hasta la primera tecla o el
 * primer clic, que el navegador no deja sonar antes), uno cada 1/60 s.
 */
window.ZXV = (() => {
'use strict';

const W = 640, H = 480, RATE = 48000;     // el cuadro (D81)
const FRAME_HZ = 60;                        // ZXV_FRAME_HZ (D81)
const FRAME_SAMPLES = RATE / FRAME_HZ;      // 800
const AUDIO_AHEAD = 0.06;                   // segundos en la cola

/* Los ganchos de la página (init). */
let hooks = {};

/* --- El teclado: KeyboardEvent.code a código HID (el de SDL_Scancode) -------- */

const HID = (() => {
    const map = {};
    for (let i = 0; i < 26; i++)
        map['Key' + String.fromCharCode(65 + i)] = 4 + i;
    for (let i = 1; i <= 9; i++)
        map['Digit' + i] = 29 + i;
    map.Digit0 = 39;
    Object.assign(map, {
        Enter: 40, Escape: 41, Backspace: 42, Tab: 43, Space: 44, Minus: 45, Equal: 46,
        BracketLeft: 47, BracketRight: 48, Backslash: 49, Semicolon: 51, Quote: 52, Backquote: 53,
        Comma: 54, Period: 55, Slash: 56, CapsLock: 57, PrintScreen: 70, ScrollLock: 71, Pause: 72,
        Insert: 73, Home: 74, PageUp: 75, Delete: 76, End: 77, PageDown: 78, ArrowRight: 79,
        ArrowLeft: 80, ArrowDown: 81, ArrowUp: 82, NumLock: 83, NumpadDivide: 84, NumpadMultiply: 85,
        NumpadSubtract: 86, NumpadAdd: 87, NumpadEnter: 88, Numpad0: 98, NumpadDecimal: 99,
        IntlBackslash: 100, ContextMenu: 101, IntlRo: 135, IntlYen: 137,
        ControlLeft: 224, ShiftLeft: 225, AltLeft: 226, MetaLeft: 227,
        ControlRight: 228, ShiftRight: 229, AltRight: 230, MetaRight: 231,
    });
    for (let i = 1; i <= 12; i++)
        map['F' + i] = 57 + i;
    for (let i = 1; i <= 9; i++)
        map['Numpad' + i] = 88 + i;
    return map;
})();

/* Los mandos (bits ZXV_JOY_*, spec §6.2). */
const JOY = { RIGHT: 1, LEFT: 2, DOWN: 4, UP: 8, FIRE1: 16, FIRE2: 32, FIRE3: 64, START: 128, SELECT: 256, CONNECTED: 0x8000 };

/* --- El cuadro en el canvas ---------------------------------------------------- */

let canvas = null, keysIn = null;
let gl = null, ctx2d = null;
/* El cuadro entero (640 x 480) en un canvas aparte: con 2D se pinta en él en
 * cada frame; con WebGL, solo cuando hace falta (guardar la imagen, pixel). */
const frameCanvas = document.createElement('canvas');
frameCanvas.width = W;
frameCanvas.height = H;
const frameCtx = frameCanvas.getContext('2d');
let frameStale = false;
const big = document.createElement('canvas');
const bigCtx = big.getContext('2d');
let glTex = null, glProg = null, glSrc = null, glPrescale = null;

/* Como options_border_rect: el cuadro entero (640 x 480), el pequeño y
 * ninguno. */
const BORDER_SIDES = [[64, 48], [32, 24], [0, 0]];

const E = {
    W, H, FRAME_HZ, HID, JOY, BORDER_SIDES,
    core: null,                 // las funciones de web_main.c
    FS: null,
    border: 0,
    fit: 'proporcion',          // 'proporcion', 'entera' o 'llenar'
    shown: null,                // lo que se ve: x, y, w, h del cuadro; y dónde, en el canvas
    view: 0,                    // el panel de depuración (0: no)
    paused: false,
    muted: false,
    silent: false,              // sin sonido aunque se pueda (la página de un juego, al cargarlo)
    rush: 0,                    // > 0: tantos frames en cada vuelta, sin esperar al reloj
    manual: false,              // las pruebas hacen los frames a mano
    hold: false,                // la página de un juego, quieta hasta que se pulsa jugar
};

let image = null;
let audio = null, audioNext = 0;
let clockNext = 0;

/* Se pinta con WebGL: el cuadro va a una textura y el escalado lo hace la
 * tarjeta gráfica. Con el canvas 2D (si no hay WebGL), escalar costaba en
 * Firefox unos 5 ms por frame, la cuarta parte de los 20 que hay.
 *
 * El escalado de present() en un solo paso: como ampliar sin suavizar al
 * múltiplo entero de encima (prescale) y reducir suavizando, que deja los
 * píxeles nítidos y todos iguales ("sharp bilinear"). */
function glInit() {
    const sh = (type, src) => {
        const s = gl.createShader(type);
        gl.shaderSource(s, src);
        gl.compileShader(s);
        return s;
    };
    glProg = gl.createProgram();
    gl.attachShader(glProg, sh(gl.VERTEX_SHADER,
        'attribute vec2 p; uniform vec4 src; varying vec2 t;' +
        'void main() { t = src.xy + (p * 0.5 + 0.5) * src.zw; gl_Position = vec4(p.x, -p.y, 0.0, 1.0); }'));
    gl.attachShader(glProg, sh(gl.FRAGMENT_SHADER,
        'precision highp float; uniform sampler2D tex; uniform vec2 size, prescale; varying vec2 t;' +
        'void main() {' +
        '  vec2 c = fract(t) - 0.5, r = 0.5 - 0.5 / prescale;' +
        '  vec2 u = floor(t) + (c - clamp(c, -r, r)) * prescale + 0.5;' +
        '  gl_FragColor = texture2D(tex, u / size); }'));
    gl.linkProgram(glProg);
    gl.useProgram(glProg);
    gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 1, -1, -1, 1, 1, 1]), gl.STATIC_DRAW);
    const p = gl.getAttribLocation(glProg, 'p');
    gl.enableVertexAttribArray(p);
    gl.vertexAttribPointer(p, 2, gl.FLOAT, false, 0, 0);
    glTex = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, glTex);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, W, H, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
    gl.uniform2f(gl.getUniformLocation(glProg, 'size'), W, H);
    glSrc = gl.getUniformLocation(glProg, 'src');
    glPrescale = gl.getUniformLocation(glProg, 'prescale');
    gl.clearColor(0, 0, 0, 0);
}

/* Con WebGL, el canvas del cuadro se pone al día solo cuando se lee. */
function frameCanvasNow() {
    if (frameStale && image) {
        frameCtx.putImageData(image, 0, 0);
        frameStale = false;
    }
    return frameCanvas;
}

function cstr(s) {
    const n = Module.lengthBytesUTF8(s) + 1;
    const p = Module._malloc(n);
    Module.stringToUTF8(s, p, n);
    return p;
}

function withStr(s, f) {
    const p = cstr(s);
    try { return f(p); } finally { Module._free(p); }
}

function draw() {
    const ptr = E.core.screen();
    if (!image || image.data.buffer !== Module.HEAPU8.buffer || image.ptr !== ptr) {
        image = new ImageData(new Uint8ClampedArray(Module.HEAPU8.buffer, ptr, W * H * 4), W, H);
        image.ptr = ptr;
    }
    if (gl) {
        gl.bindTexture(gl.TEXTURE_2D, glTex);
        gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, W, H, gl.RGBA, gl.UNSIGNED_BYTE,
                         new Uint8Array(Module.HEAPU8.buffer, ptr, W * H * 4));
        frameStale = true;
    } else {
        frameCtx.putImageData(image, 0, 0);
    }
    present();
}

/* La parte del cuadro que se ve: con el panel de depuración, todo. */
function shownRect() {
    if (E.view !== 0)
        return [0, 0, W, H];
    const [sx, sy] = BORDER_SIDES[E.border];
    const w = 512 + 2 * sx, h = 384 + 2 * sy;
    return [(W - w) / 2, (H - h) / 2, w, h];
}

/* El cuadro en el canvas, como en zxv (Ver > Escalado): manteniendo la
 * proporción, en múltiplos enteros o llenándolo. Si no es un múltiplo entero,
 * primero se amplía sin suavizar al múltiplo de encima y luego se reduce
 * suavizando: los píxeles quedan nítidos y todos iguales. */
function present() {
    const dpr = window.devicePixelRatio || 1;
    const cw = Math.max(1, Math.round(canvas.clientWidth * dpr));
    const ch = Math.max(1, Math.round(canvas.clientHeight * dpr));
    if (canvas.width !== cw || canvas.height !== ch) {
        canvas.width = cw;
        canvas.height = ch;
    }
    const [sx, sy, sw, sh] = shownRect();
    let dw = cw, dh = ch;
    if (E.fit === 'entera') {
        const k = Math.max(1, Math.min(Math.floor(cw / sw), Math.floor(ch / sh)));
        dw = sw * k;
        dh = sh * k;
    } else if (E.fit === 'proporcion') {
        const s = Math.min(cw / sw, ch / sh);
        dw = Math.max(1, Math.round(sw * s));
        dh = Math.max(1, Math.round(sh * s));
    }
    const dx = Math.floor((cw - dw) / 2), dy = Math.floor((ch - dh) / 2);
    E.shown = { sx, sy, sw, sh, dx, dy, dw, dh };
    if (gl) {
        gl.viewport(0, 0, cw, ch);
        gl.clear(gl.COLOR_BUFFER_BIT);
        gl.viewport(dx, ch - dy - dh, dw, dh);
        gl.uniform4f(glSrc, sx, sy, sw, sh);
        gl.uniform2f(glPrescale, Math.ceil(dw / sw), Math.ceil(dh / sh));
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
        return;
    }
    ctx2d.clearRect(0, 0, cw, ch);
    if (dw % sw === 0 && dh % sh === 0) {
        ctx2d.imageSmoothingEnabled = false;
        ctx2d.drawImage(frameCanvas, sx, sy, sw, sh, dx, dy, dw, dh);
    } else {
        const bw = sw * Math.ceil(dw / sw), bh = sh * Math.ceil(dh / sh);
        if (big.width !== bw || big.height !== bh) {
            big.width = bw;
            big.height = bh;
        }
        bigCtx.imageSmoothingEnabled = false;
        bigCtx.drawImage(frameCanvas, sx, sy, sw, sh, 0, 0, bw, bh);
        ctx2d.imageSmoothingEnabled = true;
        ctx2d.imageSmoothingQuality = 'low';     // bilineal
        ctx2d.drawImage(big, dx, dy, dw, dh);
    }
}

/* Un píxel del cuadro de 640 x 480 (con cualquier borde), en RRGGBB. */
function pixel(x, y) {
    frameCanvasNow();
    const d = frameCtx.getImageData(x, y, 1, 1).data;
    return ((d[0] << 16) | (d[1] << 8) | d[2]).toString(16).padStart(6, '0').toUpperCase();
}

/* Un píxel del canvas (en píxeles del dispositivo). */
function canvasPixel(x, y) {
    let d;
    if (gl) {
        d = new Uint8Array(4);
        gl.readPixels(x, canvas.height - 1 - y, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, d);
    } else {
        d = ctx2d.getImageData(x, y, 1, 1).data;
    }
    return ((d[0] << 16) | (d[1] << 8) | d[2]).toString(16).padStart(6, '0').toUpperCase();
}

/* --- El sonido y el ritmo ------------------------------------------------------ */

function playFrame() {
    if (!audio || audio.state !== 'running' || E.muted || E.silent)
        return;
    const buf = audio.createBuffer(2, FRAME_SAMPLES, RATE);
    const pcm = new Int16Array(Module.HEAPU8.buffer, E.core.audio(), FRAME_SAMPLES * 2);
    const l = buf.getChannelData(0), r = buf.getChannelData(1);
    for (let i = 0; i < FRAME_SAMPLES; i++) {
        l[i] = pcm[i * 2] / 32768;
        r[i] = pcm[i * 2 + 1] / 32768;
    }
    const src = audio.createBufferSource();
    src.buffer = buf;
    src.connect(audio.destination);
    if (audioNext < audio.currentTime)
        audioNext = audio.currentTime + 0.02;
    src.start(audioNext);
    audioNext += FRAME_SAMPLES / RATE;
}

/* Un frame: los mandos, la máquina, la imagen y el sonido. */
function frame() {
    pollPads();
    if (hooks.beforeFrame)
        hooks.beforeFrame();
    const r = E.core.frame();
    if (r & 1)
        playFrame();
    if (r & 3)
        draw();
    if (!!(r & 2) !== E.paused) {
        E.paused = !!(r & 2);
        if (hooks.onPaused)
            hooks.onPaused(E.paused);
    }
    if (hooks.afterFrame)
        hooks.afterFrame(r);
    return r;
}

function audioPaced() { return audio && audio.state === 'running' && !E.muted && !E.silent; }

function tick() {
    if (!E.paused && !E.manual && !E.hold) {
        if (E.rush > 0) {
            for (let i = 0; i < E.rush && E.rush > 0; i++)
                frame();
            clockNext = performance.now();
        } else if (audioPaced()) {
            for (let i = 0; i < 4 && audioNext - audio.currentTime < AUDIO_AHEAD; i++)
                frame();
            clockNext = performance.now();
        } else {
            const now = performance.now();
            if (now - clockNext > 200)
                clockNext = now;                // muy atrasados: no recuperar
            for (let i = 0; i < 4 && now >= clockNext; i++) {
                frame();
                clockNext += 1000 / FRAME_HZ;
            }
        }
    }
    if (hooks.onTick)
        hooks.onTick();
    requestAnimationFrame(tick);
}

function startAudio() {
    if (!audio) {
        try {
            audio = new AudioContext({ sampleRate: RATE, latencyHint: 'interactive' });
        } catch (e) {
            return;
        }
    }
    if (audio.state === 'suspended')
        audio.resume();
}

/* --- La tarjeta ------------------------------------------------------------------ */

let syncing = false, syncAgain = false;

/* Guarda la tarjeta en IndexedDB (sin solaparse). */
function syncCard() {
    if (syncing) {
        syncAgain = true;
        return;
    }
    syncing = true;
    E.FS.syncfs(false, () => {
        syncing = false;
        if (syncAgain) {
            syncAgain = false;
            syncCard();
        }
    });
}

function isDir(path) {
    try { return E.FS.isDir(E.FS.stat(path).mode); } catch (e) { return false; }
}

function exists(path) {
    try { E.FS.stat(path); return true; } catch (e) { return false; }
}

/* --- Entrada ---------------------------------------------------------------------- */

/* El teclado llega a un campo de texto invisible (keysIn), con el foco
 * cuando se juega. Las teclas, por su posición (KeyboardEvent.code, el código
 * HID), van a core.key al pulsarlas y soltarlas; el carácter que dan con la
 * distribución del PC, a core.text, como el texto de SDL. El carácter no se
 * saca de keydown sino de lo que escribe el campo (input y compositionend):
 * así llega ya compuesto, también con las teclas muertas (" y espacio da "),
 * la tecla Compose de Linux, los métodos de entrada y los teclados táctiles.
 * Las teclas que no dan texto (Intro, cursores, F1...) y las combinaciones
 * con Ctrl, Alt o Cmd no llegan al campo: preventDefault. */
function focusKeys() {
    keysIn.focus({ preventScroll: true });
}

/* Si la tecla puede escribir algo en el campo (sola, con Mayús o con AltGr). */
function writesText(e) {
    const altGr = e.getModifierState && e.getModifierState('AltGraph');
    if (!altGr && (e.ctrlKey || e.metaKey || e.altKey))
        return false;
    return [...e.key].length === 1 || e.key === 'Dead' || e.key === 'Process' || e.isComposing ||
        e.keyCode === 229;
}

function onKey(e, down) {
    if (down)
        startAudio();
    if (hooks.shortcut && hooks.shortcut(e, down)) {
        e.preventDefault();
        return;
    }
    if (!down || !writesText(e))
        e.preventDefault();
    const hid = HID[e.code];
    if (hid !== undefined)
        E.core.key(hid, down ? 1 : 0);
}

function sendText(s) {
    for (const ch of s)
        E.core.text(ch.codePointAt(0));
}

/* El ratón, en píxeles del cuadro de 640 x 480. */
function mouseXY(e) {
    const r = canvas.getBoundingClientRect();
    const px = (e.clientX - r.left) * canvas.width / r.width, py = (e.clientY - r.top) * canvas.height / r.height;
    const s = E.shown;
    const x = s.sx + Math.floor((px - s.dx) * s.sw / s.dw);
    const y = s.sy + Math.floor((py - s.dy) * s.sh / s.dh);
    return [Math.min(W - 1, Math.max(0, x)), Math.min(H - 1, Math.max(0, y))];
}

function mouseButtons(e) {
    /* Bits 0-2: izquierdo, derecho y central (spec §6.1). */
    E.core.mouseButtons((e.buttons & 1) | (e.buttons & 2) | (e.buttons & 4));
}

/* Los mandos (la disposición estándar de la Gamepad API), más lo que ponga la
 * página en extraJoy (los controles táctiles de la de un juego). */
const lastPads = [0, 0];
const extraJoy = [0, 0];
function pollPads() {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    const bits = [0, 0];
    let n = 0;
    for (const p of pads) {
        if (!p || n >= 2)
            continue;
        const b = i => p.buttons[i] && p.buttons[i].pressed;
        const ax = p.axes[0] || 0, ay = p.axes[1] || 0, dead = 0.37;
        let v = JOY.CONNECTED;
        if (b(15) || ax > dead) v |= JOY.RIGHT;
        if (b(14) || ax < -dead) v |= JOY.LEFT;
        if (b(13) || ay > dead) v |= JOY.DOWN;
        if (b(12) || ay < -dead) v |= JOY.UP;
        if (b(0)) v |= JOY.FIRE1;
        if (b(1)) v |= JOY.FIRE2;
        if (b(2)) v |= JOY.FIRE3;
        if (b(9)) v |= JOY.START;
        if (b(8)) v |= JOY.SELECT;
        bits[n++] = v;
    }
    for (n = 0; n < 2; n++) {
        const v = bits[n] | extraJoy[n];
        if (v !== lastPads[n]) {
            E.core.joy(n, v);
            lastPads[n] = v;
        }
    }
}

/* El teclado (campo keysIn) y el ratón (sobre el canvas). */
function bindInput() {
    keysIn.addEventListener('keydown', e => onKey(e, true));
    keysIn.addEventListener('keyup', e => onKey(e, false));
    keysIn.addEventListener('blur', () => {
        if (hooks.onBlur)
            hooks.onBlur();
        E.core.releaseAll();
    });
    /* El texto: lo tecleado (insertText) y, al acabar, lo compuesto (con una
     * tecla muerta o un método de entrada; Firefox manda después un input de
     * insertCompositionText, que no cuenta). El campo se vacía cada vez. */
    keysIn.addEventListener('input', e => {
        if (e.inputType === 'insertText' && e.data && !e.isComposing)
            sendText(e.data);
        if (!e.isComposing)
            keysIn.value = '';
    });
    keysIn.addEventListener('compositionend', e => {
        if (e.data)
            sendText(e.data);
        keysIn.value = '';
    });
    canvas.addEventListener('focus', focusKeys);        /* llegar con Tab */
    canvas.addEventListener('mousedown', e => { startAudio(); focusKeys(); mouseButtons(e); e.preventDefault(); });
    canvas.addEventListener('mouseup', mouseButtons);
    canvas.addEventListener('contextmenu', e => e.preventDefault());
    canvas.addEventListener('mouseenter', () => E.core.mouseEnter());
    canvas.addEventListener('mousemove', e => { const [x, y] = mouseXY(e); E.core.mouseMove(x, y); });
    canvas.addEventListener('wheel', e => { E.core.mouseWheel(e.deltaY < 0 ? 1 : -1); e.preventDefault(); },
                            { passive: false });
    /* El tamaño del sitio cambia: con la ventana, la pantalla completa... */
    new ResizeObserver(() => present()).observe(canvas);
}

/* --- El arranque ------------------------------------------------------------------ */

function langCode(s) { return s.charCodeAt(0) | s.charCodeAt(1) << 8; }

/* Prepara la página: el canvas, el campo del teclado y los ganchos:
 *   card          la carpeta de la tarjeta (se guarda en IndexedDB)
 *   prepare()     antes de leer la tarjeta (poner ficheros: /romdisk.bin...)
 *   loaded(err)   con la tarjeta leída, antes de arrancar; devuelve los
 *                 idiomas [máquina, mensajes] ('es', 'en'...)
 *   started()     la máquina en marcha
 *   onLog(texto), onLang(código), onPaused(sí), shortcut(e, pulsada),
 *   onBlur(), beforeFrame(), afterFrame(r), onTick() */
function init(opts) {
    hooks = opts;
    canvas = opts.canvas;
    keysIn = opts.keysIn;
    gl = canvas.getContext('webgl', { alpha: true, antialias: false, preserveDrawingBuffer: true });
    ctx2d = gl ? null : canvas.getContext('2d');
    if (gl)
        glInit();
    const Module = window.Module = {
        locateFile: path => path,
        print: text => console.log(text),
        printErr: text => console.warn(text),
        onLog: text => { console.log(text); if (hooks.onLog) hooks.onLog(text); },
        onLang: code => { if (hooks.onLang) hooks.onLang(code); },
        onRuntimeInitialized: () => {
            const FS = E.FS = Module.FS;
            const c = (name, ret, args) => Module.cwrap('web_' + name, ret, args);
            E.core = {
                start: c('start', 'number', ['number', 'number', 'number']),
                reset: c('reset', 'number', []),
                ctrlAltDel: c('ctrl_alt_del', null, []),
                frame: c('frame', 'number', []),
                screen: c('screen', 'number', []),
                audio: c('audio', 'number', []),
                frames: c('frames', 'number', []),
                warnings: c('warnings', 'number', []),
                editor: c('editor', 'number', []),
                key: c('key', null, ['number', 'number']),
                text: c('text', null, ['number']),
                releaseAll: c('release_all', null, []),
                mouseEnter: c('mouse_enter', null, []),
                mouseMove: c('mouse_move', null, ['number', 'number']),
                mouseButtons: c('mouse_buttons', null, ['number']),
                mouseWheel: c('mouse_wheel', null, ['number']),
                joy: c('joy', null, ['number', 'number']),
                paste: c('paste', null, ['number', 'number']),
                type: c('type', null, ['number', 'number']),
                saveSnapshot: c('save_snapshot', 'number', ['number']),
                loadSnapshot: c('load_snapshot', 'number', ['number']),
                setStrict: c('set_strict', null, ['number']),
                setTurbo: c('set_turbo', null, ['number']),
                setLang: c('set_lang', null, ['number']),
                setMsgLang: c('set_msg_lang', null, ['number']),
                view: c('view', 'number', ['number']),
                paused: c('paused', 'number', []),
                pause: c('pause', null, ['number']),
                step: c('step', null, []),
            };
            FS.mkdir(opts.card);
            FS.mount(FS.filesystems.IDBFS, {}, opts.card);
            if (opts.prepare)
                opts.prepare();
            FS.syncfs(true, err => {
                const [lang, msgLang] = opts.loaded(err);
                withStr(opts.card, p => E.core.start(langCode(lang), langCode(msgLang), p));
                bindInput();
                draw();
                if (opts.started)
                    opts.started();
                requestAnimationFrame(tick);
            });
        },
    };
    const script = document.createElement('script');
    script.src = 'zxv-core.js';
    document.body.append(script);
}

Object.assign(E, {
    init, langCode, withStr, draw, present, shownRect, frame, frameCanvasNow, pixel, canvasPixel,
    startAudio, audioPaced, syncCard, isDir, exists, focusKeys, extraJoy,
});
return E;
})();
