/*
 * zxv.js - La página de ZX-V: la máquina (zxv-core.wasm, src/web_main.c)
 * en un canvas, con el sonido por Web Audio, el teclado, el ratón y los
 * mandos del navegador, y la tarjeta SD guardada en el navegador (IndexedDB).
 *
 * El ritmo: con sonido, se hacen los frames que faltan para tener unos 60 ms
 * en la cola (como zxv y zxv-sdl); sin sonido (hasta la primera tecla o el
 * primer clic, que el navegador no deja sonar antes), uno cada 20 ms.
 */
(() => {
'use strict';

const CARD = '/tarjeta';
const W = 720, H = 576, RATE = 48000, FRAME_SAMPLES = 960;
const AUDIO_AHEAD = 0.06;                   // segundos en la cola
const QUICK_SNAPSHOT = '/instantanea.zxvs'; // la de Alt+I y Alt+U (en memoria)

/* --- Los textos de la página (el español es el original) --------------------- */

const EN = {
    'Archivo': 'File',
    'Máquina': 'Machine',
    'Ver': 'View',
    'Ayuda': 'Help',
    'Abrir...': 'Open...',
    'La tarjeta SD...': 'The SD card...',
    'Grabar el programa...': 'Save the program...',
    'Guardar una instantánea': 'Save a snapshot',
    'Volver a la instantánea': 'Back to the snapshot',
    'Descargar la instantánea': 'Download the snapshot',
    'Guardar la imagen': 'Save the image',
    'Reiniciar': 'Reset',
    'Ctrl+Alt+Supr (vuelve al BASIC)': 'Ctrl+Alt+Del (back to BASIC)',
    'Parar': 'Stop',
    'Seguir': 'Continue',
    'Una instrucción': 'One instruction',
    'Panel de depuración': 'Debug panel',
    'Sonido': 'Sound',
    'Pantalla completa': 'Full screen',
    'Cambiar el borde': 'Change the border',
    'Borde': 'Border',
    'Completo (720 x 576)': 'Full (720 x 576)',
    'Mediano (640 x 480)': 'Medium (640 x 480)',
    'Pequeño (576 x 432)': 'Small (576 x 432)',
    'Sin borde (512 x 384)': 'No border (512 x 384)',
    'Escalado': 'Scaling',
    'Mantener la proporción': 'Keep the aspect ratio',
    'Escala entera (los píxeles, todos iguales)': 'Integer scale (all pixels the same size)',
    'Llenar la ventana (deformando la imagen)': 'Fill the window (stretching the picture)',
    'Alt+Intro': 'Alt+Enter',
    'Opciones...': 'Options...',
    'Abrir en una pestaña aparte': 'Open in a separate tab',
    'Opciones': 'Options',
    'El teclado y los atajos': 'The keyboard and shortcuts',
    'El proyecto (GitHub)': 'The project (GitHub)',
    'Términos de uso': 'Terms of use',
    'Avisos de terceros': 'Third-party notices',
    'Cargando...': 'Loading...',
    'La tarjeta SD': 'The SD card',
    'La tarjeta se guarda en este navegador. Los ficheros que se suben, o se arrastran a la página, van a la carpeta que se ve.':
        'The card is stored in this browser. Files you upload, or drag onto the page, go to the folder shown.',
    'Subir ficheros...': 'Upload files...',
    'Nueva carpeta...': 'New folder...',
    'Cerrar': 'Close',
    'Idioma': 'Language',
    'Modo estricto (avisa de lo que no haría la máquina)': 'Strict mode (warns about what the machine would not do)',
    'CPU sin límite de velocidad': 'CPU with no speed limit',
    'Ocultar el puntero sobre la imagen': 'Hide the pointer over the image',
    'El idioma de la máquina cambia al reiniciar.': 'The machine language changes on reset.',
    'abrir': 'open',
    'descargar': 'download',
    'borrar': 'delete',
    '(vacía)': '(empty)',
    '¿Borrar %1?': 'Delete %1?',
    'Nombre de la carpeta:': 'Folder name:',
    'Nombre del programa en la tarjeta (sin extensión):': 'Program name on the card (no extension):',
    'Abierto: %1': 'Opened: %1',
    'No sé abrir %1: los programas son .zxv, .tap o .tzx; los listados, .bas o .txt; las instantáneas, .zxvs':
        'Cannot open %1: programs are .zxv, .tap or .tzx; listings, .bas or .txt; snapshots, .zxvs',
    'Instantánea guardada': 'Snapshot saved',
    'Instantánea cargada: %1': 'Snapshot loaded: %1',
    'Este navegador no puede guardar la tarjeta: se perderá al cerrar la página.':
        'This browser cannot store the card: it will be lost when the page is closed.',
    'frame %1': 'frame %1',
    'parada': 'stopped',
    '%1 avisos del modo estricto': '%1 strict mode warnings',
    'sin sonido: pulsa una tecla o haz clic': 'no sound: press a key or click',
    'AYUDA': `<h2>ZX-V</h2>
<p>A 32-bit recreational computer inspired by the ZX Spectrum, here in the browser: the same
machine, ROM and BASIC as the desktop emulator.</p>
<h3>The keyboard</h3>
<p>The keys are the PC's, with its layout: what you type is what you get. BASIC keywords are
typed letter by letter. Click on the image so it gets the keyboard.</p>
<table>
<tr><td><kbd>Enter</kbd></td><td>store the line or run the command</td></tr>
<tr><td>Cursor keys</td><td>move the cursor around the listing</td></tr>
<tr><td><kbd>Home</kbd>, <kbd>End</kbd></td><td>start and end of the line (with Ctrl: of the program)</td></tr>
<tr><td><kbd>Esc</kbd></td><td>undo the changes to the line; with a program running, stop it (BREAK)</td></tr>
<tr><td><kbd>Tab</kbd></td><td>the editor menu (renumber, print...)</td></tr>
</table>
<h3>The emulator shortcuts (left Alt)</h3>
<table>
<tr><td><kbd>Alt+O</kbd></td><td>open a program, a listing or a snapshot</td></tr>
<tr><td><kbd>Alt+G</kbd></td><td>save the BASIC program to the card (SAVE)</td></tr>
<tr><td><kbd>Alt+I</kbd>, <kbd>Alt+U</kbd></td><td>save a snapshot; go back to it</td></tr>
<tr><td><kbd>Alt+R</kbd></td><td>reset</td></tr>
<tr><td><kbd>Alt+P</kbd>, <kbd>Alt+N</kbd></td><td>stop or continue; one instruction</td></tr>
<tr><td><kbd>Alt+D</kbd></td><td>debug panel (CPU, VRAM, sound, line table)</td></tr>
<tr><td><kbd>Alt+S</kbd></td><td>save the image (PNG)</td></tr>
<tr><td><kbd>Alt+B</kbd></td><td>change the border (full, medium, small, none)</td></tr>
<tr><td><kbd>Alt+Enter</kbd></td><td>full screen</td></tr>
</table>
<p>You can drag files onto the page: they are copied to the card and the first one is opened.
The card (Card button) keeps its files in this browser; download the ones you want to keep
elsewhere.</p>`,
};

const ES_HELP = `<h2>ZX-V</h2>
<p>Un ordenador recreativo de 32 bits inspirado en el ZX Spectrum, aquí en el navegador: la
misma máquina, la misma ROM y el mismo BASIC que el emulador de escritorio.</p>
<h3>El teclado</h3>
<p>Las teclas son las del PC, con su distribución: lo que se teclea es lo que sale. Las
palabras clave del BASIC se escriben letra a letra. Haz clic en la imagen para que reciba el
teclado.</p>
<table>
<tr><td><kbd>Intro</kbd></td><td>guardar la línea o ejecutar la orden</td></tr>
<tr><td>Cursores</td><td>mover el cursor por el listado</td></tr>
<tr><td><kbd>Inicio</kbd>, <kbd>Fin</kbd></td><td>principio y final de la línea (con Ctrl: del programa)</td></tr>
<tr><td><kbd>Esc</kbd></td><td>deshacer los cambios de la línea; con un programa en marcha, pararlo (BREAK)</td></tr>
<tr><td><kbd>Tab</kbd></td><td>el menú del editor (renumerar, imprimir...)</td></tr>
</table>
<h3>Los atajos del emulador (Alt izquierda)</h3>
<table>
<tr><td><kbd>Alt+O</kbd></td><td>abrir un programa, un listado o una instantánea</td></tr>
<tr><td><kbd>Alt+G</kbd></td><td>grabar el programa BASIC en la tarjeta (SAVE)</td></tr>
<tr><td><kbd>Alt+I</kbd>, <kbd>Alt+U</kbd></td><td>guardar una instantánea; volver a ella</td></tr>
<tr><td><kbd>Alt+R</kbd></td><td>reiniciar</td></tr>
<tr><td><kbd>Alt+P</kbd>, <kbd>Alt+N</kbd></td><td>parar o seguir; una instrucción</td></tr>
<tr><td><kbd>Alt+D</kbd></td><td>panel de depuración (CPU, VRAM, sonido, tabla por línea)</td></tr>
<tr><td><kbd>Alt+S</kbd></td><td>guardar la imagen (PNG)</td></tr>
<tr><td><kbd>Alt+B</kbd></td><td>cambiar el borde (completo, mediano, pequeño, ninguno)</td></tr>
<tr><td><kbd>Alt+Intro</kbd></td><td>pantalla completa</td></tr>
</table>
<p>Se pueden arrastrar ficheros a la página: se copian a la tarjeta y se abre el primero. La
tarjeta (botón Tarjeta) guarda sus ficheros en este navegador; descarga los que quieras
conservar en otro sitio.</p>`;

let uiLang = 'es';

function t(es, ...args) {
    let s = uiLang === 'es' ? es : (EN[es] || es);
    args.forEach((a, i) => { s = s.replace('%' + (i + 1), a); });
    return s;
}

function langCode(s) { return s.charCodeAt(0) | s.charCodeAt(1) << 8; }

function store(key, value) {
    try {
        if (value === undefined)
            return localStorage.getItem(key);
        localStorage.setItem(key, value);
    } catch (e) { /* sin almacenamiento: solo esta vez */ }
    return null;
}

function translatePage() {
    document.documentElement.lang = uiLang;
    for (const el of document.querySelectorAll('[data-t]'))
        el.textContent = t(el.dataset.t);
    document.getElementById('ayuda-texto').innerHTML = uiLang === 'es' ? ES_HELP : EN.AYUDA;
}

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

/* --- La máquina ------------------------------------------------------------------ */

const canvas = document.getElementById('pantalla');
const ctx2d = canvas.getContext('2d');
/* El cuadro entero (720 x 576) se pinta aparte; en el canvas, la parte que
 * se ve con el borde elegido, escalada (present). */
const frameCanvas = document.createElement('canvas');
frameCanvas.width = W;
frameCanvas.height = H;
const frameCtx = frameCanvas.getContext('2d');
const big = document.createElement('canvas');
const bigCtx = big.getContext('2d');
const BORDER_SIDES = [[104, 96], [64, 48], [32, 24], [0, 0]];   // options_border_rect
let border = Math.min(3, Math.max(0, parseInt(store('zxv-borde'), 10) || 0));
let fit = ['proporcion', 'entera', 'llenar'].includes(store('zxv-escalado')) ? store('zxv-escalado') : 'proporcion';
let shown = null;           // lo que se ve: x, y, w, h del cuadro; y dónde, en el canvas
let core = null;            // las funciones de web_main.c
let FS = null;
let image = null;
let audio = null, audioNext = 0, muted = store('zxv-silencio') === '1';
let clockNext = 0;
let paused = false;
let view = 0;
let quickSnapshot = false;
let lastLog = '';
let syncing = false, syncAgain = false;

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
    const ptr = core.screen();
    if (!image || image.data.buffer !== Module.HEAPU8.buffer || image.ptr !== ptr) {
        image = new ImageData(new Uint8ClampedArray(Module.HEAPU8.buffer, ptr, W * H * 4), W, H);
        image.ptr = ptr;
    }
    frameCtx.putImageData(image, 0, 0);
    present();
}

/* La parte del cuadro que se ve: con el panel de depuración, todo. */
function shownRect() {
    if (view !== 0)
        return [0, 0, W, H];
    const [sx, sy] = BORDER_SIDES[border];
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
    if (fit === 'entera') {
        const k = Math.max(1, Math.min(Math.floor(cw / sw), Math.floor(ch / sh)));
        dw = sw * k;
        dh = sh * k;
    } else if (fit === 'proporcion') {
        const s = Math.min(cw / sw, ch / sh);
        dw = Math.max(1, Math.round(sw * s));
        dh = Math.max(1, Math.round(sh * s));
    }
    const dx = Math.floor((cw - dw) / 2), dy = Math.floor((ch - dh) / 2);
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
    shown = { sx, sy, sw, sh, dx, dy, dw, dh };
}

function setBorder(b) {
    border = b;
    store('zxv-borde', String(b));
    document.getElementById('o-borde').value = String(b);
    present();
}

function playFrame() {
    if (!audio || audio.state !== 'running' || muted)
        return;
    const buf = audio.createBuffer(2, FRAME_SAMPLES, RATE);
    const pcm = new Int16Array(Module.HEAPU8.buffer, core.audio(), FRAME_SAMPLES * 2);
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
    const r = core.frame();
    if (r & 1)
        playFrame();
    if (r & 3)
        draw();
    if (!!(r & 2) !== paused) {
        paused = !!(r & 2);
        updateButtons();
    }
    return r;
}

function audioPaced() { return audio && audio.state === 'running' && !muted; }

function tick() {
    if (!paused && !window.zxv.manual) {
        if (audioPaced()) {
            for (let i = 0; i < 4 && audioNext - audio.currentTime < AUDIO_AHEAD; i++)
                frame();
            clockNext = performance.now();
        } else {
            const now = performance.now();
            if (now - clockNext > 200)
                clockNext = now;                // muy atrasados: no recuperar
            for (let i = 0; i < 4 && now >= clockNext; i++) {
                frame();
                clockNext += 20;
            }
        }
    }
    status();
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

let statusText = '';
function status() {
    const parts = [t('frame %1', Math.floor(core.frames()))];
    if (paused)
        parts.push(t('parada'));
    const w = core.warnings();
    if (w)
        parts.push(t('%1 avisos del modo estricto', w));
    if (!audioPaced() && !muted)
        parts.push(t('sin sonido: pulsa una tecla o haz clic'));
    const s = parts.join(' · ');
    if (s !== statusText) {
        statusText = s;
        document.getElementById('estado').textContent = s;
    }
}

function message(text) {
    lastLog = text;
    document.getElementById('aviso').textContent = text;
}

/* --- La tarjeta ------------------------------------------------------------------ */

/* Guarda la tarjeta en IndexedDB (sin solaparse). */
function syncCard() {
    if (syncing) {
        syncAgain = true;
        return;
    }
    syncing = true;
    FS.syncfs(false, () => {
        syncing = false;
        if (syncAgain) {
            syncAgain = false;
            syncCard();
        }
    });
}

function isDir(path) {
    try { return FS.isDir(FS.stat(path).mode); } catch (e) { return false; }
}

function exists(path) {
    try { FS.stat(path); return true; } catch (e) { return false; }
}

function copyTree(from, to) {
    for (const name of FS.readdir(from)) {
        if (name === '.' || name === '..')
            continue;
        const a = from + '/' + name, b = to + '/' + name;
        if (isDir(a)) {
            if (!exists(b))
                FS.mkdir(b);
            copyTree(a, b);
        } else if (!exists(b)) {
            FS.writeFile(b, FS.readFile(a));
        }
    }
}

function emptyCard() {
    return FS.readdir(CARD).filter(n => n !== '.' && n !== '..').length === 0;
}

/* Abre un fichero de la tarjeta, como zxv: un programa se carga con LOAD
 * (con CD si está en una subcarpeta), un listado se teclea y una
 * instantánea se carga en la máquina. */
function openFile(path) {
    const name = path.split('/').pop();
    const ext = (name.match(/\.[^.]*$/) || [''])[0].toLowerCase();
    const rel = path.slice(CARD.length + 1);
    if (ext === '.zxvs')
        return loadSnapshot(path);
    if (ext === '.bas' || ext === '.txt') {
        core.reset();
        withStr(new TextDecoder().decode(FS.readFile(path)), p => core.paste(p, 40));
    } else if (ext === '.zxv' || ext === '.tap' || ext === '.tzx') {
        core.reset();
        const slash = rel.lastIndexOf('/');
        const base = rel.slice(slash + 1).replace(/\.[^.]*$/, '');
        const keys = (slash > 0 ? `CD "/${rel.slice(0, slash)}"\\n` : '') + `LOAD "${base}"\\n`;
        withStr(keys, p => core.type(p, 25));
    } else {
        message(t('No sé abrir %1: los programas son .zxv, .tap o .tzx; los listados, .bas o .txt; las instantáneas, .zxvs', name));
        return false;
    }
    message(t('Abierto: %1', rel));
    canvas.focus();
    return true;
}

function loadSnapshot(path) {
    lastLog = '';
    const ok = withStr(path, p => core.loadSnapshot(p));
    if (ok)
        message(t('Instantánea cargada: %1', path.split('/').pop()));
    paused = !!core.paused();
    draw();
    updateButtons();
    return !!ok;
}

/* Copia ficheros del ordenador a una carpeta de la tarjeta. */
async function importFiles(files, dir) {
    const done = [];
    for (const f of files) {
        const path = dir + '/' + f.name;
        FS.writeFile(path, new Uint8Array(await f.arrayBuffer()));
        done.push(path);
    }
    syncCard();
    return done;
}

function download(name, data, type) {
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([data], { type: type || 'application/octet-stream' }));
    a.download = name;
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 1000);
}

/* El diálogo de la tarjeta. */
let cardDir = CARD;
function showCard() {
    const list = document.getElementById('t-lista');
    list.innerHTML = '';
    document.getElementById('t-ruta').textContent = cardDir.slice(CARD.length) || '/';
    document.getElementById('t-arriba').disabled = cardDir === CARD;
    const names = FS.readdir(cardDir).filter(n => n !== '.' && n !== '..')
        .sort((a, b) => (isDir(cardDir + '/' + b) - isDir(cardDir + '/' + a)) || a.localeCompare(b));
    if (!names.length)
        list.insertRow().insertCell().textContent = t('(vacía)');
    for (const name of names) {
        const path = cardDir + '/' + name;
        const row = list.insertRow();
        const cName = row.insertCell(), cSize = row.insertCell(), cAct = row.insertCell();
        cSize.className = 'tam';
        cAct.className = 'acc';
        const link = (text, f) => {
            const a = document.createElement('a');
            a.textContent = text;
            a.onclick = f;
            cAct.append(a, ' ');
        };
        if (isDir(path)) {
            const a = document.createElement('a');
            a.textContent = name + '/';
            a.onclick = () => { cardDir = path; showCard(); };
            cName.append(a);
        } else {
            cName.textContent = name;
            cSize.textContent = String(FS.stat(path).size);
            if (/\.(zxv|tap|tzx|bas|txt|zxvs)$/i.test(name))
                link(t('abrir'), () => { document.getElementById('d-tarjeta').close(); openFile(path); });
            link(t('descargar'), () => download(name, FS.readFile(path)));
        }
        link(t('borrar'), () => {
            if (!confirm(t('¿Borrar %1?', name)))
                return;
            try {
                if (isDir(path))
                    FS.rmdir(path);
                else
                    FS.unlink(path);
            } catch (e) {
                message(e.message || String(e));
            }
            syncCard();
            showCard();
        });
    }
}

/* --- Órdenes ------------------------------------------------------------------- */

function saveSnapshot() {
    lastLog = '';
    if (withStr(QUICK_SNAPSHOT, p => core.saveSnapshot(p))) {
        quickSnapshot = true;
        message(t('Instantánea guardada'));
    }
    updateButtons();
}

function saveProgram() {
    const name = prompt(t('Nombre del programa en la tarjeta (sin extensión):'), 'programa');
    if (!name)
        return;
    withStr('{CTRL+END}', p => core.type(p, 0));
    withStr(`SAVE "${name.replace(/"/g, '')}"\n`, p => core.paste(p, 8));
    canvas.focus();
}

function setPaused(on) {
    core.pause(on ? 1 : 0);
    paused = !!core.paused();
    draw();
    updateButtons();
}

function cycleView() {
    view = core.view((view + 1) % 5);
    draw();
    updateButtons();
}

function toggleFullScreen() {
    /* Dentro de un iframe (itch.io...) puede no estar permitido. */
    const p = document.fullscreenElement ? document.exitFullscreen()
                                         : document.getElementById('marco').requestFullscreen();
    if (p)
        p.catch(e => message(e.message || String(e)));
}

/* La imagen, con el borde que se ve y a su tamaño. */
function screenshot() {
    const [sx, sy, sw, sh] = shownRect();
    const c = document.createElement('canvas');
    c.width = sw;
    c.height = sh;
    c.getContext('2d').drawImage(frameCanvas, sx, sy, sw, sh, 0, 0, sw, sh);
    c.toBlob(b => download('zxv.png', b, 'image/png'), 'image/png');
}

function updateButtons() {
    document.getElementById('b-volver').disabled = !quickSnapshot;
    document.getElementById('b-descargar').disabled = !quickSnapshot;
    document.querySelector('#b-parar span').textContent = t(paused ? 'Seguir' : 'Parar');
    document.getElementById('b-paso').disabled = !paused;
    document.getElementById('b-depurar').classList.toggle('activo', view !== 0);
    document.getElementById('b-sonido').classList.toggle('activo', !muted);
}

/* Los atajos con Alt izquierda (Alt derecha puede ser AltGr). */
let altLeft = false;
function shortcut(code) {
    switch (code) {
    case 'KeyO': document.getElementById('fichero').click(); return true;
    case 'KeyG': saveProgram(); return true;
    case 'KeyI': saveSnapshot(); return true;
    case 'KeyU': if (quickSnapshot) loadSnapshot(QUICK_SNAPSHOT); return true;
    case 'KeyR': core.reset(); message(''); return true;
    case 'KeyP': setPaused(!paused); return true;
    case 'KeyN': if (paused) core.step(); return true;
    case 'KeyD': cycleView(); return true;
    case 'KeyS': screenshot(); return true;
    case 'KeyB': setBorder((border + 1) % BORDER_SIDES.length); return true;
    case 'Enter': toggleFullScreen(); return true;
    }
    return false;
}

/* --- Entrada ---------------------------------------------------------------------- */

function onKey(e, down) {
    if (e.code === 'AltLeft')
        altLeft = down;
    if (down)
        startAudio();
    if (down && altLeft && !e.ctrlKey && e.code !== 'AltLeft' && shortcut(e.code)) {
        e.preventDefault();
        return;
    }
    const hid = HID[e.code];
    if (hid === undefined)
        return;
    e.preventDefault();
    core.key(hid, down ? 1 : 0);
    /* Y el carácter que sale con la distribución del PC (como el texto de
     * SDL): no con Ctrl ni Cmd, salvo AltGr. */
    const altGr = e.getModifierState && e.getModifierState('AltGraph');
    if (down && [...e.key].length === 1 && (altGr || (!e.ctrlKey && !e.metaKey && !e.altKey)))
        core.text(e.key.codePointAt(0));
}

/* El ratón, en píxeles del cuadro de 720 x 576. */
function mouseXY(e) {
    const r = canvas.getBoundingClientRect();
    const px = (e.clientX - r.left) * canvas.width / r.width, py = (e.clientY - r.top) * canvas.height / r.height;
    const x = shown.sx + Math.floor((px - shown.dx) * shown.sw / shown.dw);
    const y = shown.sy + Math.floor((py - shown.dy) * shown.sh / shown.dh);
    return [Math.min(W - 1, Math.max(0, x)), Math.min(H - 1, Math.max(0, y))];
}

let buttons = 0;
function mouseButtons(e) {
    /* Bits 0-2: izquierdo, derecho y central (spec §6.1). */
    buttons = (e.buttons & 1) | (e.buttons & 2) | (e.buttons & 4);
    core.mouseButtons(buttons);
}

/* Los mandos (la disposición estándar de la Gamepad API). */
const JOY = { RIGHT: 1, LEFT: 2, DOWN: 4, UP: 8, FIRE1: 16, FIRE2: 32, FIRE3: 64, START: 128, SELECT: 256, CONNECTED: 0x8000 };
const lastPads = [0, 0];
function pollPads() {
    const pads = navigator.getGamepads ? navigator.getGamepads() : [];
    let n = 0;
    for (const p of pads) {
        if (!p || n >= 2)
            continue;
        const b = i => p.buttons[i] && p.buttons[i].pressed;
        const ax = p.axes[0] || 0, ay = p.axes[1] || 0, dead = 0.37;
        let bits = JOY.CONNECTED;
        if (b(15) || ax > dead) bits |= JOY.RIGHT;
        if (b(14) || ax < -dead) bits |= JOY.LEFT;
        if (b(13) || ay > dead) bits |= JOY.DOWN;
        if (b(12) || ay < -dead) bits |= JOY.UP;
        if (b(0)) bits |= JOY.FIRE1;
        if (b(1)) bits |= JOY.FIRE2;
        if (b(2)) bits |= JOY.FIRE3;
        if (b(9)) bits |= JOY.START;
        if (b(8)) bits |= JOY.SELECT;
        if (bits !== lastPads[n]) {
            core.joy(n, bits);
            lastPads[n] = bits;
        }
        n++;
    }
    for (; n < 2; n++)
        if (lastPads[n]) {
            core.joy(n, 0);
            lastPads[n] = 0;
        }
}

function closeMenus() {
    for (const m of document.querySelectorAll('.menu.abierto'))
        m.classList.remove('abierto');
}

function bindUi() {
    canvas.addEventListener('keydown', e => onKey(e, true));
    canvas.addEventListener('keyup', e => onKey(e, false));
    canvas.addEventListener('blur', () => { altLeft = false; core.releaseAll(); });
    canvas.addEventListener('mousedown', e => { startAudio(); canvas.focus(); mouseButtons(e); e.preventDefault(); });
    canvas.addEventListener('mouseup', mouseButtons);
    canvas.addEventListener('contextmenu', e => e.preventDefault());
    canvas.addEventListener('mouseenter', () => core.mouseEnter());
    canvas.addEventListener('mousemove', e => { const [x, y] = mouseXY(e); core.mouseMove(x, y); });
    canvas.addEventListener('wheel', e => { core.mouseWheel(e.deltaY < 0 ? 1 : -1); e.preventDefault(); },
                            { passive: false });

    const on = (id, f) => document.getElementById(id).addEventListener('click', () => {
        closeMenus();
        startAudio();
        f();
        if (!document.querySelector('dialog[open]'))
            canvas.focus();
    });
    /* Los menús: se abren con un clic y se cierran al elegir o al hacer clic
     * fuera. */
    for (const menu of document.querySelectorAll('.menu'))
        menu.querySelector('.titulo').addEventListener('click', e => {
            const open = !menu.classList.contains('abierto');
            closeMenus();
            menu.classList.toggle('abierto', open);
            e.stopPropagation();
        });
    document.addEventListener('click', e => { if (!e.target.closest('.menu')) closeMenus(); });
    const input = document.getElementById('fichero');
    on('b-abrir', () => input.click());
    input.addEventListener('change', async () => {
        const paths = await importFiles([...input.files], CARD);
        input.value = '';
        if (paths.length)
            openFile(paths[0]);
    });
    on('b-tarjeta', () => { showCard(); document.getElementById('d-tarjeta').showModal(); });
    on('b-grabar', saveProgram);
    on('b-guardar', saveSnapshot);
    on('b-volver', () => loadSnapshot(QUICK_SNAPSHOT));
    on('b-descargar', () => download('zxv.zxvs', FS.readFile(QUICK_SNAPSHOT)));
    on('b-reiniciar', () => { core.reset(); message(''); });
    on('b-cad', () => core.ctrlAltDel());
    on('b-parar', () => setPaused(!paused));
    on('b-paso', () => core.step());
    on('b-depurar', cycleView);
    on('b-imagen', screenshot);
    on('b-sonido', () => { muted = !muted; store('zxv-silencio', muted ? '1' : '0'); updateButtons(); });
    on('b-completa', toggleFullScreen);
    on('b-borde', () => setBorder((border + 1) % BORDER_SIDES.length));
    /* Dentro de un iframe de otro sitio (itch.io...), algunos navegadores no
     * dejan guardar nada: en una pestaña aparte, sí. */
    const tab = document.getElementById('b-pestana');
    tab.hidden = window.top === window;
    on('b-pestana', () => window.open(location.href, '_blank', 'noopener'));
    on('b-opciones', () => document.getElementById('d-opciones').showModal());
    on('b-ayuda', () => document.getElementById('d-ayuda').showModal());

    const upload = document.getElementById('subir');
    on('t-subir', () => upload.click());
    upload.addEventListener('change', async () => {
        await importFiles([...upload.files], cardDir);
        upload.value = '';
        showCard();
    });
    on('t-arriba', () => { cardDir = cardDir.slice(0, cardDir.lastIndexOf('/')) || CARD; showCard(); });
    on('t-carpeta', () => {
        const name = prompt(t('Nombre de la carpeta:'));
        if (name && !/[\/\\]/.test(name)) {
            try { FS.mkdir(cardDir + '/' + name); } catch (e) { message(e.message || String(e)); }
            syncCard();
            showCard();
        }
    });
    for (const d of ['d-tarjeta', 'd-opciones', 'd-ayuda']) {
        const dlg = document.getElementById(d);
        dlg.addEventListener('close', () => canvas.focus());
    }
    on('t-cerrar', () => document.getElementById('d-tarjeta').close());
    on('o-cerrar', () => document.getElementById('d-opciones').close());
    on('a-cerrar', () => document.getElementById('d-ayuda').close());

    const lang = document.getElementById('o-idioma');
    lang.value = uiLang;
    lang.addEventListener('change', () => {
        uiLang = lang.value;
        store('zxv-idioma', uiLang);
        store('zxv-maquina', uiLang);
        core.setMsgLang(langCode(uiLang));
        core.setLang(langCode(uiLang));
        translatePage();
        updateButtons();
    });
    const borderSel = document.getElementById('o-borde');
    borderSel.value = String(border);
    borderSel.addEventListener('change', () => setBorder(parseInt(borderSel.value, 10)));
    const fitSel = document.getElementById('o-escalado');
    fitSel.value = fit;
    fitSel.addEventListener('change', () => {
        fit = fitSel.value;
        store('zxv-escalado', fit);
        present();
    });
    /* El tamaño del sitio cambia: con la ventana, la pantalla completa... */
    new ResizeObserver(() => present()).observe(canvas);
    const check = (id, key, f) => {
        const box = document.getElementById(id);
        box.checked = store(key) === '1';
        f(box.checked);
        box.addEventListener('change', () => { store(key, box.checked ? '1' : '0'); f(box.checked); });
    };
    check('o-estricto', 'zxv-estricto', v => core.setStrict(v ? 1 : 0));
    check('o-turbo', 'zxv-turbo', v => core.setTurbo(v ? 1 : 0));
    check('o-puntero', 'zxv-puntero', v => canvas.classList.toggle('sin-puntero', v));

    /* Arrastrar ficheros a la página: a la tarjeta, y se abre el primero. */
    const marco = document.getElementById('marco');
    document.addEventListener('dragover', e => { e.preventDefault(); marco.classList.add('arrastrando'); });
    document.addEventListener('dragleave', () => marco.classList.remove('arrastrando'));
    document.addEventListener('drop', async e => {
        e.preventDefault();
        marco.classList.remove('arrastrando');
        const paths = await importFiles([...e.dataTransfer.files], CARD);
        if (paths.length)
            openFile(paths[0]);
    });

    /* Antes de irse, la tarjeta a IndexedDB; y de vez en cuando, por lo que
     * escriban los programas. */
    document.addEventListener('visibilitychange', () => { if (document.hidden) syncCard(); });
    window.addEventListener('pagehide', syncCard);
    setInterval(syncCard, 3000);
}

/* --- Arranque ---------------------------------------------------------------------- */

const pageLang = (navigator.language || 'en').slice(0, 2).toLowerCase() === 'es' ? 'es' : 'en';
uiLang = store('zxv-idioma') || pageLang;
translatePage();

const Module = window.Module = {
    locateFile: path => path,
    print: text => console.log(text),
    printErr: text => console.warn(text),
    onLog: text => { console.log(text); message(text); },
    onLang: code => store('zxv-maquina', code),
    onRuntimeInitialized: () => {
        FS = Module.FS;
        const c = (name, ret, args) => Module.cwrap('web_' + name, ret, args);
        core = {
            start: c('start', 'number', ['number', 'number']),
            reset: c('reset', 'number', []),
            ctrlAltDel: c('ctrl_alt_del', null, []),
            frame: c('frame', 'number', []),
            screen: c('screen', 'number', []),
            audio: c('audio', 'number', []),
            frames: c('frames', 'number', []),
            warnings: c('warnings', 'number', []),
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
        /* La tarjeta: la del navegador; la primera vez, con los ejemplos. */
        FS.mkdir(CARD);
        FS.mount(FS.filesystems.IDBFS, {}, CARD);
        FS.syncfs(true, err => {
            if (err)
                message(t('Este navegador no puede guardar la tarjeta: se perderá al cerrar la página.'));
            if (emptyCard() && exists('/examples')) {
                copyTree('/examples', CARD);
                syncCard();
            }
            const machineLang = store('zxv-maquina') || uiLang;
            core.start(langCode(machineLang), langCode(uiLang));
            document.getElementById('cargando').remove();
            bindUi();
            updateButtons();
            draw();
            canvas.focus();
            window.zxv.resolve();
            requestAnimationFrame(tick);
        });
    },
};

/* Para las pruebas (tests/web): la máquina sin esperar al reloj (con
 * manual, la página no hace frames por su cuenta). */
window.zxv = {
    manual: false,
    ready: null,
    resolve: null,
    runFrames(n) { for (let i = 0; i < n; i++) frame(); },
    open: openFile,
    saveSnapshot: path => withStr(path, p => core.saveSnapshot(p)),
    loadSnapshot,
    type: (seq, delay) => withStr(seq, p => core.type(p, delay || 0)),
    paste: (text, delay) => withStr(text, p => core.paste(p, delay || 0)),
    reset: () => core.reset(),
    view: v => { view = core.view(v); draw(); },
    /* Un píxel del cuadro de 720 x 576 (con cualquier borde) */
    pixel(x, y) {
        const d = frameCtx.getImageData(x, y, 1, 1).data;
        return ((d[0] << 16) | (d[1] << 8) | d[2]).toString(16).padStart(6, '0').toUpperCase();
    },
    /* Un píxel del canvas (en píxeles del dispositivo), y dónde está el cuadro en él */
    canvasPixel(x, y) {
        const d = ctx2d.getImageData(x, y, 1, 1).data;
        return ((d[0] << 16) | (d[1] << 8) | d[2]).toString(16).padStart(6, '0').toUpperCase();
    },
    get shown() { return shown; },
    setBorder,
    setFit(f) { fit = f; present(); },
    readFile: path => new TextDecoder().decode(FS.readFile(path)),
    exists,
    sync: () => new Promise(res => FS.syncfs(false, res)),
    get message() { return lastLog; },
    get paused() { return paused; },
};
window.zxv.ready = new Promise(res => { window.zxv.resolve = res; });

const script = document.createElement('script');
script.src = 'zxv-core.js';
document.body.append(script);
})();
