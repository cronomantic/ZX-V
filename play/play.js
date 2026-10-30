/*
 * play.js - La página de un juego (D85): un solo programa de ZX-V, que empieza
 * solo y vuelve a empezar al acabar, sin que se vea el BASIC. Lo prepara
 * sdk/tools/mkweb.py, que escribe game.js (window.ZXV_GAME): el nombre, el
 * fichero que arranca, el idioma, el borde, los controles táctiles y los
 * ficheros del juego en un disco ROM (la unidad ROM:), en base64.
 *
 * Cómo arranca: la máquina empieza sin verse, deprisa y sin sonido; cuando
 * el BASIC espera una orden (la variable EDITOR, spec §8.1) se le pega
 * LOAD "ROM:juego" (y RUN si es un listado). Cuando EDITOR lleva unos frames
 * a 0, el juego ya está en marcha: se enseña. Si vuelve a 1 (el juego acabó o
 * se paró con Esc), se reinicia la máquina y otra vez desde el principio. Si
 * se para enseguida varias veces seguidas, se enseña la pantalla del BASIC
 * (con el informe) y un botón para volver a empezar.
 *
 * La tarjeta es la carpeta /tarjeta-ID, con su propia base de datos de
 * IndexedDB: los récords de cada juego se quedan en el navegador y no se
 * mezclan con los de otros juegos del mismo sitio (itch.io).
 */
(() => {
'use strict';

const E = window.ZXV;
const G = window.ZXV_GAME;
const REVEAL = 20;              // frames con EDITOR a 0 para enseñar el juego
const LOAD_LIMIT = 15 * 60;     // frames para que arranque (si no, algo va mal)
const SHORT = 3 * 60;           // una partida más corta que esto cuenta como fallo
const RUSH = 8;                 // frames por vuelta mientras no se ve

const pageLang = (navigator.language || 'en').slice(0, 2).toLowerCase() === 'es' ? 'es' : 'en';
const lang = G && (G.lang === 'es' || G.lang === 'en') ? G.lang : pageLang;
const ES = lang === 'es';
const T = {
    play: ES ? 'Haz clic para jugar' : 'Click to play',
    tap: ES ? 'Toca para jugar' : 'Tap to play',
    loading: ES ? 'Cargando…' : 'Loading…',
    stopped: ES ? 'El juego se ha parado.' : 'The game stopped.',
    again: ES ? 'Volver a empezar' : 'Start again',
    sound: ES ? 'Sonido' : 'Sound',
    full: ES ? 'Pantalla completa' : 'Full screen',
    nogame: ES ? 'Falta game.js: prepara la página con mkweb.py.' : 'game.js is missing: build the page with mkweb.py.',
    nocard: ES ? 'Este navegador no guarda nada: los récords se perderán al cerrar la página.'
               : 'This browser cannot store anything: high scores will be lost when the page is closed.',
};

const $ = id => document.getElementById(id);
const canvas = $('pantalla'), keysIn = $('teclado');
const cover = $('portada'), loading = $('cargando'), stopped = $('parado');

document.documentElement.lang = lang;
loading.textContent = T.loading;
$('otra-vez').textContent = T.again;
$('parado-texto').textContent = T.stopped;
$('b-sonido').title = T.sound;
$('b-completa').title = T.full;

if (!G) {
    $('titulo').textContent = 'ZX-V';
    $('jugar').remove();
    cover.append(Object.assign(document.createElement('p'), { textContent: T.nogame }));
    return;
}

document.title = G.title || 'ZX-V';
$('titulo').textContent = G.title || 'ZX-V';

/* --- El estado -------------------------------------------------------------------- */

const CARD = '/tarjeta-' + G.id;
const KEY = 'zxv-play-' + G.id + '-';       /* localStorage: las preferencias de este juego */
function store(key, value) {
    try {
        if (value === undefined)
            return localStorage.getItem(KEY + key);
        localStorage.setItem(KEY + key, value);
    } catch (e) { /* sin almacenamiento */ }
    return null;
}

/* boot: arrancando la ROM; loading: cargando el juego; playing: en marcha;
 * stopped: parado (se ve el BASIC). */
let state = 'boot';
let stateFrames = 0;            // frames en el estado
let zeroFrames = 0;             // frames seguidos con EDITOR a 0
let ran = false;                // en loading: EDITOR ya ha pasado a 0 (la orden va)
const GONE = 60;                // en loading: frames tras los que EDITOR a 1 es que acabó
let fails = 0;                  // partidas cortas seguidas
let clicked = false;            // se ha pulsado jugar
let restarts = 0;               // las veces que ha vuelto a empezar

E.border = Number.isInteger(G.border) ? Math.min(2, Math.max(0, G.border)) : 0;
E.fit = ['proporcion', 'entera', 'llenar'].includes(G.fit) ? G.fit : 'proporcion';
E.muted = store('silencio') === '1';

function hidden(on) {
    canvas.classList.toggle('oculto', on);
    E.silent = on;
    E.rush = on ? RUSH : 0;
    loading.hidden = !on || !clicked;
}

function setState(s) {
    state = s;
    stateFrames = 0;
    zeroFrames = 0;
    ran = false;
}

/* El fichero que arranca, en ROM: (LOAD con la extensión: sin dudas). */
function loadCommand() {
    const start = G.start.replace(/"/g, '');
    return `LOAD "ROM:${start}"` + (/\.(bas|txt)$/i.test(start) ? ': RUN' : '') + '\n';
}

function restart() {
    restarts++;
    E.core.reset();
    E.core.releaseAll();
    setState('boot');
    stopped.hidden = true;
    hidden(true);
}

/* Después de cada frame: ver en qué está el BASIC. */
function afterFrame() {
    const editor = E.core.editor();
    stateFrames++;
    zeroFrames = editor ? 0 : zeroFrames + 1;
    switch (state) {
    case 'boot':
        if (editor) {
            E.withStr(loadCommand(), p => E.core.paste(p, 0));
            setState('loading');
        } else if (stateFrames > LOAD_LIMIT) {
            fail();
        }
        break;
    case 'loading':
        if (!editor)
            ran = true;
        if (editor && (ran || stateFrames > GONE)) {
            /* Ha vuelto al BASIC antes de enseñarse: acabó enseguida (quizá
             * en el mismo frame, sin que se viera EDITOR a 0). */
            ended(0);
        } else if (zeroFrames >= REVEAL) {
            setState('playing');
            hidden(false);
            if (!clicked)
                E.hold = true;          /* quieto hasta que se pulse jugar */
        } else if (stateFrames > LOAD_LIMIT) {
            fail();
        }
        break;
    case 'playing':
        if (editor)
            ended(stateFrames);     /* el juego acabó o se paró (Esc) */
        break;
    }
}

/* Ha vuelto al BASIC tras frames frames en marcha: otra vez, salvo si se
 * para enseguida una y otra vez. */
function ended(frames) {
    fails = frames < SHORT ? fails + 1 : 0;
    if (fails >= 2)
        fail();
    else
        restart();
}

/* No arranca, o se para enseguida: se ve lo que dice el BASIC. */
function fail() {
    setState('stopped');
    hidden(false);
    stopped.hidden = false;
}

/* --- Jugar --------------------------------------------------------------------------- */

function play() {
    if (clicked)
        return;
    clicked = true;
    cover.hidden = true;
    E.startAudio();
    if (!touchShown())
        E.focusKeys();
    E.hold = false;
    loading.hidden = state === 'playing' || state === 'stopped';
}

$('jugar').addEventListener('click', play);
cover.addEventListener('click', play);
$('otra-vez').addEventListener('click', () => { fails = 0; restart(); E.focusKeys(); });
$('b-sonido').addEventListener('click', () => {
    E.muted = !E.muted;
    store('silencio', E.muted ? '1' : '0');
    $('b-sonido').classList.toggle('mudo', E.muted);
    E.startAudio();
});
$('b-sonido').classList.toggle('mudo', E.muted);
const full = $('b-completa');
full.hidden = !document.fullscreenEnabled;
full.addEventListener('click', () => {
    const p = document.fullscreenElement ? document.exitFullscreen() : $('juego').requestFullscreen();
    if (p)
        p.catch(() => {});
});
/* Una tecla también empieza (el teclado aún no tiene el foco: en la página). */
document.addEventListener('keydown', e => {
    if (!clicked && !e.altKey && !e.ctrlKey && !e.metaKey) {
        play();
        e.preventDefault();
    }
});
for (const b of document.querySelectorAll('#botones button'))
    b.addEventListener('mouseup', () => { if (!touchShown()) E.focusKeys(); });

/* --- Los controles táctiles ------------------------------------------------------- */

/* Cada control manda un bit del mando 1 ({joy: 'FIRE1'}) o una tecla
 * ({key: 'Space'}, con el nombre de KeyboardEvent.code). */
const DPADS = {
    joy: { up: { joy: 'UP' }, down: { joy: 'DOWN' }, left: { joy: 'LEFT' }, right: { joy: 'RIGHT' } },
    arrows: { up: { key: 'ArrowUp' }, down: { key: 'ArrowDown' }, left: { key: 'ArrowLeft' }, right: { key: 'ArrowRight' } },
    qaop: { up: { key: 'KeyQ' }, down: { key: 'KeyA' }, left: { key: 'KeyO' }, right: { key: 'KeyP' } },
};
const DEFAULT_TOUCH = {
    show: 'auto',
    dpad: 'joy',
    buttons: [{ label: 'A', joy: 'FIRE1' }, { label: 'B', joy: 'FIRE2' }],
    menu: [{ label: 'ESC', key: 'Escape' }],
};
const touch = Object.assign({}, DEFAULT_TOUCH, G.touch || {});

const held = new Map();         /* lo que tiene pulsado cada control */
function press(target, down) {
    if (!target)
        return;
    if (target.joy && E.JOY[target.joy]) {
        const bit = E.JOY[target.joy];
        E.extraJoy[0] = (down ? E.extraJoy[0] | bit : E.extraJoy[0] & ~bit) | E.JOY.CONNECTED;
    } else if (target.key && E.HID[target.key] !== undefined) {
        E.core.key(E.HID[target.key], down ? 1 : 0);
    }
}

/* Los movimientos y el soltar del dedo, al control aunque salga de él. */
function capture(el, e) {
    try { el.setPointerCapture(e.pointerId); } catch (err) { /* sin captura: igual */ }
}

function makeButton(spec, cls) {
    const b = document.createElement('div');
    b.className = 'boton' + (cls ? ' ' + cls : '');
    b.textContent = spec.label || '';
    b.dataset.control = spec.label || '';
    const up = e => {
        if (!held.has(e.pointerId))
            return;
        held.delete(e.pointerId);
        b.classList.remove('pulsado');
        press(spec, false);
    };
    b.addEventListener('pointerdown', e => {
        e.preventDefault();
        capture(b, e);
        held.set(e.pointerId, spec);
        b.classList.add('pulsado');
        E.startAudio();
        press(spec, true);
    });
    b.addEventListener('pointerup', up);
    b.addEventListener('pointercancel', up);
    return b;
}

/* La cruceta: la dirección sale de dónde está el dedo respecto al centro (en
 * ocho direcciones, con una zona muerta en medio). */
function makeDpad(map) {
    const d = document.createElement('div');
    d.className = 'cruceta';
    const mark = document.createElement('div');
    mark.className = 'marca';
    d.append(mark);
    let pointer = null, dirs = new Set();
    const set = next => {
        for (const k of dirs)
            if (!next.has(k))
                press(map[k], false);
        for (const k of next)
            if (!dirs.has(k))
                press(map[k], true);
        dirs = next;
        d.classList.toggle('pulsada', dirs.size > 0);
    };
    const move = e => {
        const r = d.getBoundingClientRect();
        const x = (e.clientX - r.left) / r.width * 2 - 1, y = (e.clientY - r.top) / r.height * 2 - 1;
        const next = new Set();
        if (Math.hypot(x, y) > 0.25) {
            const a = Math.atan2(y, x);         /* 0 a la derecha, en sentido horario */
            const sector = Math.round(a / (Math.PI / 4));
            if ([-1, 0, 1].includes(sector)) next.add('right');
            if ([3, 4, -4, -3].includes(sector)) next.add('left');
            if ([1, 2, 3].includes(sector)) next.add('down');
            if ([-1, -2, -3].includes(sector)) next.add('up');
        }
        const k = Math.min(1, Math.hypot(x, y));
        const a = Math.atan2(y, x);
        mark.style.transform = `translate(${Math.cos(a) * k * 70}%, ${Math.sin(a) * k * 70}%)`;
        set(next);
    };
    const up = e => {
        if (e.pointerId !== pointer)
            return;
        pointer = null;
        mark.style.transform = '';
        set(new Set());
    };
    d.addEventListener('pointerdown', e => {
        e.preventDefault();
        capture(d, e);
        pointer = e.pointerId;
        E.startAudio();
        move(e);
    });
    d.addEventListener('pointermove', e => { if (e.pointerId === pointer) move(e); });
    d.addEventListener('pointerup', up);
    d.addEventListener('pointercancel', up);
    return d;
}

function buildTouch() {
    const box = $('tactil');
    const map = typeof touch.dpad === 'string' ? DPADS[touch.dpad] : touch.dpad;
    if (map)
        box.append(makeDpad(map));
    if (touch.buttons && touch.buttons.length) {
        const group = document.createElement('div');
        group.className = 'grupo';
        for (const spec of touch.buttons.slice(0, 4))
            group.append(makeButton(spec));
        box.append(group);
    }
    if (touch.menu && touch.menu.length) {
        const menu = document.createElement('div');
        menu.className = 'menu';
        for (const spec of touch.menu.slice(0, 4))
            menu.append(makeButton(spec, 'pequeno'));
        box.append(menu);
    }
}

function touchShown() { return document.body.classList.contains('tactil'); }

function showTouch() {
    if (touchShown() || touch.show === 'never')
        return;
    document.body.classList.add('tactil');
    $('tactil').hidden = false;
    E.extraJoy[0] |= E.JOY.CONNECTED;         /* el mando 1, enchufado: el de la pantalla */
    /* Sin el teclado de la pantalla al tocar la imagen. */
    keysIn.setAttribute('inputmode', 'none');
    $('jugar').textContent = clicked ? T.play : T.tap;
    if (E.core)
        E.present();
}

buildTouch();
$('jugar').textContent = T.play;
if (touch.show === 'always' || (touch.show !== 'never' &&
    (matchMedia('(pointer: coarse)').matches || navigator.maxTouchPoints > 0)))
    showTouch();
window.addEventListener('pointerdown', e => { if (e.pointerType === 'touch') showTouch(); }, true);

/* --- El arranque -------------------------------------------------------------------- */

function base64(s) {
    const bin = atob(s);
    const out = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++)
        out[i] = bin.charCodeAt(i);
    return out;
}

let resolveReady;
E.init({
    canvas,
    keysIn,
    card: CARD,
    onLog: text => console.log(text),
    afterFrame,
    /* El juego, como disco ROM: web_start lo pone en ROM:. */
    prepare: () => E.FS.writeFile('/romdisk.bin', base64(G.romdisk)),
    loaded: err => {
        if (err)
            console.warn(T.nocard);
        return [lang, lang];
    },
    started: () => {
        hidden(true);
        resolveReady();
        /* Los récords: a IndexedDB de vez en cuando y al irse. */
        document.addEventListener('visibilitychange', () => { if (document.hidden) E.syncCard(); });
        window.addEventListener('pagehide', E.syncCard);
        setInterval(E.syncCard, 3000);
    },
});

/* Para las pruebas (tests/web/play_test.cjs): los frames a mano. */
window.zxvPlay = {
    ready: new Promise(res => { resolveReady = res; }),
    get manual() { return E.manual; },
    set manual(v) { E.manual = v; },
    runFrames(n) { for (let i = 0; i < n && !E.hold; i++) E.frame(); },
    get state() { return state; },
    get visible() { return !canvas.classList.contains('oculto'); },
    get hold() { return E.hold; },
    get joy() { return E.extraJoy[0]; },
    get restarts() { return restarts; },
    get stoppedShown() { return !stopped.hidden; },
    get touch() { return touchShown(); },
    play,
    editor: () => E.core.editor(),
    pixel: E.pixel,
    canvasPixel: E.canvasPixel,
    get shown() { return E.shown; },
    exists: E.exists,
    readFile: path => new TextDecoder().decode(E.FS.readFile(path)),
    sync: () => new Promise(res => E.FS.syncfs(false, res)),
    card: CARD,
};
})();
