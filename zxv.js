/*
 * zxv.js - La página del emulador: los menús, los diálogos, los atajos, la
 * tarjeta con los ejemplos y los avisos. La máquina (en el canvas, con el
 * sonido, el teclado, el ratón y los mandos) es la de engine.js, la misma que
 * la de la página de un juego (play/, D85).
 */
(() => {
'use strict';

const E = window.ZXV;
const CARD = '/tarjeta';
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
    'Completo (640 x 480)': 'Full (640 x 480)',
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
    'Ejemplos actualizados en la tarjeta: %1': 'Examples updated on the card: %1',
    'Teclado de ZX-V': 'ZX-V keyboard',
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
<tr><td><kbd>Alt+B</kbd></td><td>change the border (full, small, none)</td></tr>
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
<tr><td><kbd>Alt+B</kbd></td><td>cambiar el borde (completo, pequeño, ninguno)</td></tr>
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
    keysIn.setAttribute('aria-label', t('Teclado de ZX-V'));
    document.getElementById('ayuda-texto').innerHTML = uiLang === 'es' ? ES_HELP : EN.AYUDA;
}

/* --- La máquina ------------------------------------------------------------------ */

const canvas = document.getElementById('pantalla');
const keysIn = document.getElementById('teclado');      /* recibe el teclado (engine.js) */
/* Antes de D81, con el cuadro de 720 x 576, 'zxv-borde' contaba también el de
 * 720 x 576 (0) y el de 640 x 480 (1), que ahora es el completo. */
let border = parseInt(store('zxv-borde-salida') ?? '', 10);
if (isNaN(border))
    border = Math.max(0, (parseInt(store('zxv-borde'), 10) || 0) - 1);
E.border = Math.min(E.BORDER_SIDES.length - 1, Math.max(0, border));
E.fit = ['proporcion', 'entera', 'llenar'].includes(store('zxv-escalado')) ? store('zxv-escalado') : 'proporcion';
E.muted = store('zxv-silencio') === '1';
let quickSnapshot = false;
let lastLog = '';

function setBorder(b) {
    E.border = b;
    store('zxv-borde-salida', String(b));
    document.getElementById('o-borde').value = String(b);
    E.present();
}

let statusText = '';
function status() {
    const parts = [t('frame %1', Math.floor(E.core.frames()))];
    if (E.paused)
        parts.push(t('parada'));
    const w = E.core.warnings();
    if (w)
        parts.push(t('%1 avisos del modo estricto', w));
    if (!E.audioPaced() && !E.muted)
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

function emptyCard() {
    return E.FS.readdir(CARD).filter(n => n !== '.' && n !== '..').length === 0;
}

/* --- Los ejemplos de la tarjeta -------------------------------------------------
 *
 * La página trae los ejemplos (en /examples) y la tarjeta del navegador se
 * guarda de una visita a otra. Al llegar una versión nueva, los ejemplos de la
 * tarjeta se cambian por los nuevos, pero solo los que siguen como se
 * copiaron: lo que el usuario ha cambiado o borrado se queda como está. Lo que
 * se copió, con su huella, se apunta en localStorage (EXAMPLES_KEY). */

const EXAMPLES_KEY = 'zxv-ejemplos';

/* Las huellas de los ejemplos de la 0.4.0 a la 0.5.0, que no apuntaban lo que
 * copiaban: lo que hay en la tarjeta igual a uno de estos se puede cambiar. */
const OLD_EXAMPLES = {
    'boardtest.zxv': ['b9d01a37453d333b', 'd37a31e331f34777'],
    'digital.zxv': ['775bdff383bb9b17'],
    'estela.zxv': ['f7c8840452adbbb0', '5d71bc629beaaeae'],
    'files.zxv': ['371dcc64d44a5328'],
    'graphics.zxv': ['28c53cc779f22aa3'],
    'hello.zxv': ['0fdd5ca306af8ad7'],
    'mine.zxv': ['4af7e01c4d339b18', '00b5a7897c9ab6f5'],
    'muncher.bas': ['6db8767ab93a68ee', '472b0c0729d4e39b'],
    'platformer.gfx': ['eab3ad52eac13efe'],
    'platformer.tap': ['0f35e335967bcd31', '2843e55e1f9323fa'],
    'route128.zxv': ['e22b0a11b600b785', 'bc725b33e272228f'],
    'rps.bas': ['83eaa6d011d1bc5c'],
    'scroll.zxv': ['f7bb7d99da60da5d', '0f8c6eac540550b8'],
    'sentry.bas': ['dd2c8b32559f4366', '9c71d058eb93e444'],
    'street.zxv': ['a5d04098838e5cf4', 'c47da53251ecd06e'],
};

/* La huella de un fichero: dos FNV-1a de 32 bits con distinta base. */
function fingerprint(bytes) {
    let a = 0x811c9dc5, b = 0x2b6d6a09;
    for (let i = 0; i < bytes.length; i++) {
        a = Math.imul(a ^ bytes[i], 0x01000193) >>> 0;
        b = Math.imul(b ^ bytes[i], 0x01000193) >>> 0;
    }
    return a.toString(16).padStart(8, '0') + b.toString(16).padStart(8, '0');
}

/* Los ficheros de una carpeta y sus subcarpetas, relativos a ella. */
function listTree(dir, rel = '') {
    const out = [];
    for (const name of E.FS.readdir(dir)) {
        if (name === '.' || name === '..')
            continue;
        if (E.isDir(dir + '/' + name))
            out.push(...listTree(dir + '/' + name, rel + name + '/'));
        else
            out.push(rel + name);
    }
    return out;
}

/* Copia los ejemplos que faltan y pone al día los que siguen como se copiaron.
 * Da cuántos ha cambiado (los nuevos de la primera vez no cuentan). */
function updateExamples() {
    let installed = null;
    try {
        installed = JSON.parse(store(EXAMPLES_KEY));
    } catch (e) { /* estropeado: como si no estuviera */ }
    const first = emptyCard();
    if (first)
        installed = {};
    const record = {};
    let updated = 0;
    for (const rel of listTree('/examples')) {
        const data = E.FS.readFile('/examples/' + rel), now = fingerprint(data);
        const path = CARD + '/' + rel;
        /* Lo que se sabe que se copió antes: lo apuntado o, en una tarjeta de
         * antes de apuntarlo, lo que traían las versiones anteriores. */
        const before = installed ? (rel in installed ? [installed[rel]] : []) : (OLD_EXAMPLES[rel] || []);
        if (!E.exists(path)) {
            if (before.length) {                /* lo borró el usuario */
                record[rel] = before[0];
                continue;
            }
            for (let d = CARD, parts = rel.split('/').slice(0, -1), i = 0; i < parts.length; i++) {
                d += '/' + parts[i];
                if (!E.exists(d))
                    E.FS.mkdir(d);
            }
            E.FS.writeFile(path, data);
            record[rel] = now;
            if (!first)
                updated++;
            continue;
        }
        const card = fingerprint(E.FS.readFile(path));
        if (card !== now && before.includes(card)) {
            E.FS.writeFile(path, data);           /* el de antes, sin tocar: el nuevo */
            updated++;
            record[rel] = now;
        } else {
            record[rel] = card === now ? now : (before[0] || card);
        }
    }
    store(EXAMPLES_KEY, JSON.stringify(record));
    return updated;
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
        E.core.reset();
        E.withStr(new TextDecoder().decode(E.FS.readFile(path)), p => E.core.paste(p, 40));
    } else if (ext === '.zxv' || ext === '.tap' || ext === '.tzx') {
        E.core.reset();
        const slash = rel.lastIndexOf('/');
        const base = rel.slice(slash + 1).replace(/\.[^.]*$/, '');
        const keys = (slash > 0 ? `CD "/${rel.slice(0, slash)}"\\n` : '') + `LOAD "${base}"\\n`;
        E.withStr(keys, p => E.core.type(p, 25));
    } else {
        message(t('No sé abrir %1: los programas son .zxv, .tap o .tzx; los listados, .bas o .txt; las instantáneas, .zxvs', name));
        return false;
    }
    message(t('Abierto: %1', rel));
    E.focusKeys();
    return true;
}

function loadSnapshot(path) {
    lastLog = '';
    const ok = E.withStr(path, p => E.core.loadSnapshot(p));
    if (ok)
        message(t('Instantánea cargada: %1', path.split('/').pop()));
    E.paused = !!E.core.paused();
    E.draw();
    updateButtons();
    return !!ok;
}

/* Copia ficheros del ordenador a una carpeta de la tarjeta. */
async function importFiles(files, dir) {
    const done = [];
    for (const f of files) {
        const path = dir + '/' + f.name;
        E.FS.writeFile(path, new Uint8Array(await f.arrayBuffer()));
        done.push(path);
    }
    E.syncCard();
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
    const names = E.FS.readdir(cardDir).filter(n => n !== '.' && n !== '..')
        .sort((a, b) => (E.isDir(cardDir + '/' + b) - E.isDir(cardDir + '/' + a)) || a.localeCompare(b));
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
        if (E.isDir(path)) {
            const a = document.createElement('a');
            a.textContent = name + '/';
            a.onclick = () => { cardDir = path; showCard(); };
            cName.append(a);
        } else {
            cName.textContent = name;
            cSize.textContent = String(E.FS.stat(path).size);
            if (/\.(zxv|tap|tzx|bas|txt|zxvs)$/i.test(name))
                link(t('abrir'), () => { document.getElementById('d-tarjeta').close(); openFile(path); });
            link(t('descargar'), () => download(name, E.FS.readFile(path)));
        }
        link(t('borrar'), () => {
            if (!confirm(t('¿Borrar %1?', name)))
                return;
            try {
                if (E.isDir(path))
                    E.FS.rmdir(path);
                else
                    E.FS.unlink(path);
            } catch (e) {
                message(e.message || String(e));
            }
            E.syncCard();
            showCard();
        });
    }
}

/* --- Órdenes ------------------------------------------------------------------- */

function saveSnapshot() {
    lastLog = '';
    if (E.withStr(QUICK_SNAPSHOT, p => E.core.saveSnapshot(p))) {
        quickSnapshot = true;
        message(t('Instantánea guardada'));
    }
    updateButtons();
}

function saveProgram() {
    const name = prompt(t('Nombre del programa en la tarjeta (sin extensión):'), 'programa');
    if (!name)
        return;
    E.withStr('{CTRL+END}', p => E.core.type(p, 0));
    E.withStr(`SAVE "${name.replace(/"/g, '')}"\n`, p => E.core.paste(p, 8));
    E.focusKeys();
}

function setPaused(on) {
    E.core.pause(on ? 1 : 0);
    E.paused = !!E.core.paused();
    E.draw();
    updateButtons();
}

function cycleView() {
    E.view = E.core.view((E.view + 1) % 5);
    E.draw();
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
    const [sx, sy, sw, sh] = E.shownRect();
    const c = document.createElement('canvas');
    c.width = sw;
    c.height = sh;
    c.getContext('2d').drawImage(E.frameCanvasNow(), sx, sy, sw, sh, 0, 0, sw, sh);
    c.toBlob(b => download('zxv.png', b, 'image/png'), 'image/png');
}

function updateButtons() {
    document.getElementById('b-volver').disabled = !quickSnapshot;
    document.getElementById('b-descargar').disabled = !quickSnapshot;
    document.querySelector('#b-parar span').textContent = t(E.paused ? 'Seguir' : 'Parar');
    document.getElementById('b-paso').disabled = !E.paused;
    document.getElementById('b-depurar').classList.toggle('activo', E.view !== 0);
    document.getElementById('b-sonido').classList.toggle('activo', !E.muted);
}

/* Los atajos con Alt izquierda (Alt derecha puede ser AltGr). */
let altLeft = false;
function shortcut(code) {
    switch (code) {
    case 'KeyO': document.getElementById('fichero').click(); return true;
    case 'KeyG': saveProgram(); return true;
    case 'KeyI': saveSnapshot(); return true;
    case 'KeyU': if (quickSnapshot) loadSnapshot(QUICK_SNAPSHOT); return true;
    case 'KeyR': E.core.reset(); message(''); return true;
    case 'KeyP': setPaused(!E.paused); return true;
    case 'KeyN': if (E.paused) E.core.step(); return true;
    case 'KeyD': cycleView(); return true;
    case 'KeyS': screenshot(); return true;
    case 'KeyB': setBorder((E.border + 1) % E.BORDER_SIDES.length); return true;
    case 'Enter': toggleFullScreen(); return true;
    }
    return false;
}

/* Para engine.js: Alt izquierda más una tecla (Alt derecha puede ser AltGr). */
function onShortcutKey(e, down) {
    if (e.code === 'AltLeft')
        altLeft = down;
    return down && altLeft && !e.ctrlKey && e.code !== 'AltLeft' && shortcut(e.code);
}

function closeMenus() {
    for (const m of document.querySelectorAll('.menu.abierto'))
        m.classList.remove('abierto');
}

function bindUi() {

    const on = (id, f) => document.getElementById(id).addEventListener('click', () => {
        closeMenus();
        E.startAudio();
        f();
        if (!document.querySelector('dialog[open]'))
            E.focusKeys();
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
    on('b-descargar', () => download('zxv.zxvs', E.FS.readFile(QUICK_SNAPSHOT)));
    on('b-reiniciar', () => { E.core.reset(); message(''); });
    on('b-cad', () => E.core.ctrlAltDel());
    on('b-parar', () => setPaused(!E.paused));
    on('b-paso', () => E.core.step());
    on('b-depurar', cycleView);
    on('b-imagen', screenshot);
    on('b-sonido', () => { E.muted = !E.muted; store('zxv-silencio', E.muted ? '1' : '0'); updateButtons(); });
    on('b-completa', toggleFullScreen);
    on('b-borde', () => setBorder((E.border + 1) % E.BORDER_SIDES.length));
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
            try { E.FS.mkdir(cardDir + '/' + name); } catch (e) { message(e.message || String(e)); }
            E.syncCard();
            showCard();
        }
    });
    for (const d of ['d-tarjeta', 'd-opciones', 'd-ayuda']) {
        const dlg = document.getElementById(d);
        dlg.addEventListener('close', E.focusKeys);
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
        E.core.setMsgLang(E.langCode(uiLang));
        E.core.setLang(E.langCode(uiLang));
        translatePage();
        updateButtons();
    });
    const borderSel = document.getElementById('o-borde');
    borderSel.value = String(E.border);
    borderSel.addEventListener('change', () => setBorder(parseInt(borderSel.value, 10)));
    const fitSel = document.getElementById('o-escalado');
    fitSel.value = E.fit;
    fitSel.addEventListener('change', () => {
        E.fit = fitSel.value;
        store('zxv-escalado', E.fit);
        E.present();
    });
    const check = (id, key, f) => {
        const box = document.getElementById(id);
        box.checked = store(key) === '1';
        f(box.checked);
        box.addEventListener('change', () => { store(key, box.checked ? '1' : '0'); f(box.checked); });
    };
    check('o-estricto', 'zxv-estricto', v => E.core.setStrict(v ? 1 : 0));
    check('o-turbo', 'zxv-turbo', v => E.core.setTurbo(v ? 1 : 0));
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
    document.addEventListener('visibilitychange', () => { if (document.hidden) E.syncCard(); });
    window.addEventListener('pagehide', E.syncCard);
    setInterval(E.syncCard, 3000);
}

/* --- Arranque ---------------------------------------------------------------------- */

const pageLang = (navigator.language || 'en').slice(0, 2).toLowerCase() === 'es' ? 'es' : 'en';
uiLang = store('zxv-idioma') || pageLang;
translatePage();

let updated = 0;                    /* los ejemplos puestos al día al arrancar */
E.init({
    canvas,
    keysIn,
    card: CARD,
    onLog: message,
    onLang: code => store('zxv-maquina', code),
    onPaused: updateButtons,
    shortcut: onShortcutKey,
    onBlur: () => { altLeft = false; },
    onTick: status,
    /* La tarjeta: la del navegador; la primera vez, con los ejemplos. */
    loaded: err => {
        if (err)
            message(t('Este navegador no puede guardar la tarjeta: se perderá al cerrar la página.'));
        updated = E.exists('/examples') ? updateExamples() : 0;
        E.syncCard();
        return [store('zxv-maquina') || uiLang, uiLang];
    },
    started: () => {
        document.getElementById('cargando').remove();
        bindUi();
        updateButtons();
        E.focusKeys();
        if (updated)
            message(t('Ejemplos actualizados en la tarjeta: %1', updated));
        window.zxv.resolve();
    },
});

/* Para las pruebas (tests/web): la máquina sin esperar al reloj (con
 * manual, la página no hace frames por su cuenta). */
window.zxv = {
    get manual() { return E.manual; },
    set manual(v) { E.manual = v; },
    ready: null,
    resolve: null,
    runFrames(n) { for (let i = 0; i < n; i++) E.frame(); },
    open: openFile,
    saveSnapshot: path => E.withStr(path, p => E.core.saveSnapshot(p)),
    loadSnapshot,
    type: (seq, delay) => E.withStr(seq, p => E.core.type(p, delay || 0)),
    paste: (text, delay) => E.withStr(text, p => E.core.paste(p, delay || 0)),
    reset: () => E.core.reset(),
    view: v => { E.view = E.core.view(v); E.draw(); },
    /* Un píxel del cuadro de 640 x 480 (con cualquier borde) */
    pixel: E.pixel,
    /* Un píxel del canvas (en píxeles del dispositivo), y dónde está el cuadro en él */
    canvasPixel: E.canvasPixel,
    get shown() { return E.shown; },
    setBorder,
    setFit(f) { E.fit = f; E.present(); },
    readFile: path => new TextDecoder().decode(E.FS.readFile(path)),
    exists: E.exists,
    sync: () => new Promise(res => E.FS.syncfs(false, res)),
    get message() { return lastLog; },
    get paused() { return E.paused; },
};
window.zxv.ready = new Promise(res => { window.zxv.resolve = res; });
})();
