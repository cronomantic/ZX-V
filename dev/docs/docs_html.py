#!/usr/bin/env python3
"""La documentación (docs/) en HTML, para leerla sin conexión.

  docs_html.py docs/ SALIDA

Deja en SALIDA las páginas de docs/en y docs/es en HTML, con las mismas
carpetas (README.md pasa a index.html), las imágenes y un index.html que
elige el idioma. Los enlaces entre páginas siguen funcionando, también los
de las anclas; los que salen de docs/ (los ejemplos del SDK...) van a GitHub.
Termina con error si un enlace lleva a una página o un ancla que no existe.

Cada página lleva a un lado el índice, el del README.md del idioma (sus
secciones y sus enlaces), y debajo de una página las de más adentro a las que
lleva, cuando se está en ella (las lecciones del curso). Una página que no
sale en el índice también es un error. Es lo que se lee en los paquetes y en
https://cronomantic.github.io/ZX-V/docs/ (GitHub Pages). El estilo está en
docs.css, junto a este script, y va como style.css; el icono es el de zxv
(favicon.png junto al script en el repositorio público, que lo regenera en
GitHub Pages con cada cambio de docs/: dev/public/.github/workflows/docs.yml).

Necesita Python-Markdown (python3-markdown en Debian y Ubuntu).
"""
import html
import os
import posixpath
import re
import shutil
import sys

import markdown
import markdown.extensions.toc as toc

REPO = 'https://github.com/cronomantic/ZX-V'
WEB = 'https://cronomantic.github.io/ZX-V/'
HERE = os.path.dirname(os.path.abspath(__file__))
LANGS = ('en', 'es')
NAME = {'en': 'English', 'es': 'Español'}
WORDS = {
    'en': {'docs': 'Documentation', 'contents': 'Contents', 'web': 'Try it in your browser',
           'prev': 'Previous', 'next': 'Next',
           'licence': 'This documentation is licensed under <a href="%s/blob/main/docs/LICENSE">CC BY 4.0</a>; '
                      'its code examples are public domain (CC0).' % REPO},
    'es': {'docs': 'Documentación', 'contents': 'Índice', 'web': 'Pruébalo en el navegador',
           'prev': 'Anterior', 'next': 'Siguiente',
           'licence': 'Esta documentación tiene licencia <a href="%s/blob/main/docs/LICENSE">CC BY 4.0</a>; '
                      'sus ejemplos de código son de dominio público (CC0).' % REPO},
}


def slug(text, sep):
    """Las anclas de GitHub: minúsculas, sin puntuación, guiones."""
    text = re.sub(r'[^\w\- ]', '', text.strip().lower())
    return text.replace(' ', sep)


def unique(id, ids):
    """Las repetidas, como en GitHub: tit, tit-1, tit-2..."""
    base, n = id, 0
    while id in ids or not id:
        n += 1
        id = '%s-%d' % (base, n)
    ids.add(id)
    return id


toc.unique = unique

PAGE = '''<!DOCTYPE html>
<html lang="{lang}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<link rel="stylesheet" href="{root}style.css">
<link rel="icon" type="image/png" href="{root}favicon.png">
</head>
<body>
<header>
<button type="button" id="menu" aria-label="{w[contents]}" onclick="document.body.classList.toggle('abierto')">&#9776;</button>
<a class="marca" href="{home}">ZX-V</a>
<span class="seccion">{w[docs]}</span>
<span class="hueco"></span>
<a class="ir" href="{other}" hreflang="{olang}" lang="{olang}">{oname}</a>
<a class="ir web" href="{web}">{w[web]}</a>
</header>
<div class="marco">
<aside>
{side}
</aside>
<main>
{body}
{steps}
<footer>{w[licence]}</footer>
</main>
</div>
<script>
var aqui = document.querySelector('aside .aqui');
if (aqui) aqui.closest('aside').scrollTop = aqui.offsetTop - 120;
document.querySelector('main').addEventListener('click', function () {{
    document.body.classList.remove('abierto');
}});
</script>
</body>
</html>
'''

CHOOSE = '''<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ZX-V</title>
<link rel="stylesheet" href="style.css">
<link rel="icon" type="image/png" href="favicon.png">
</head>
<body>
<div class="portada">
<span class="marca">ZX-V</span>
<p>Documentación · Documentation</p>
<nav>
<a href="es/index.html" lang="es">Español</a>
<a href="en/index.html" lang="en">English</a>
</nav>
</div>
</body>
</html>
'''


def html_name(path):
    """docs/en/basic/README.md -> en/basic/index.html"""
    head, name = posixpath.split(path)
    name = 'index.html' if name == 'README.md' else name[:-3] + '.html'
    return posixpath.join(head, name)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip().splitlines()[2].strip())
    docs, out = sys.argv[1], sys.argv[2]
    shutil.rmtree(out, ignore_errors=True)
    shutil.copytree(os.path.join(docs, 'images'), os.path.join(out, 'images'))
    shutil.copy(os.path.join(HERE, 'docs.css'), os.path.join(out, 'style.css'))
    icon = os.path.join(HERE, 'favicon.png')
    if not os.path.exists(icon):
        icon = os.path.join(HERE, '..', 'src', 'qt', 'zxv.png')
    shutil.copy(icon, os.path.join(out, 'favicon.png'))

    pages = {}       # en/basic/index.html -> idioma, título y cuerpo
    anchors = {}     # en/basic/index.html -> ids de la página
    links = []       # (página, destino, ancla)
    for lang in LANGS:
        for root, _, files in os.walk(os.path.join(docs, lang)):
            for f in sorted(files):
                if f.endswith('.md'):
                    src = posixpath.relpath(os.path.join(root, f), docs).replace(os.sep, '/')
                    convert(docs, src, pages, anchors, links)

    bad = 0
    for page, target, anchor in links:
        if target not in pages:
            print('%s: no existe %s' % (page, target))
            bad += 1
        elif anchor and anchor not in anchors[target]:
            print('%s: no existe %s#%s' % (page, target, anchor))
            bad += 1
    if bad:
        sys.exit('%d enlaces rotos' % bad)

    tocs = {lang: contents(lang, pages) for lang in LANGS}
    lost = [page for page in pages
            if page not in tocs[pages[page]['lang']][1]]
    for page in lost:
        print('%s: no sale en el índice (el README.md del idioma)' % page)
    if lost:
        sys.exit('%d páginas fuera del índice' % len(lost))

    for page, p in pages.items():
        lang, here = p['lang'], posixpath.dirname(page)
        olang = [l for l in LANGS if l != lang][0]
        other = olang + page[len(lang):]
        if other not in pages:
            other = olang + '/index.html'
        title = p['title'] if 'ZX-V' in p['title'] else p['title'] + ' · ZX-V'
        text = PAGE.format(lang=lang, title=html.escape(title), w=WORDS[lang],
                           root=posixpath.relpath('.', here) + '/',
                           home=posixpath.relpath(lang + '/index.html', here),
                           other=posixpath.relpath(other, here), olang=olang, oname=NAME[olang],
                           web=WEB, side=sidebar(page, tocs[lang][0]), body=p['body'],
                           steps=steps(page, tocs[lang][0], WORDS[lang]))
        path = os.path.join(out, page)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w', encoding='utf-8', newline='\n') as f:
            f.write(text)
    with open(os.path.join(out, 'index.html'), 'w', encoding='utf-8', newline='\n') as f:
        f.write(CHOOSE)
    print('%d páginas en %s' % (len(pages), out))


def page_links(page, body):
    """Los enlaces de la página a otras de docs/, en orden: (destino desde
    docs/, con su ancla, y lo de dentro del enlace). Y los títulos de sus
    secciones (h2), como (None, título)."""
    here = posixpath.dirname(page)
    for m in re.finditer(r'<h2[^>]*>(.*?)</h2>|<a href="([^"]+)">(.*?)</a>', body, re.S):
        if m.group(1) is not None:
            title = re.sub(r'<a class="ancla".*?</a>', '', m.group(1))
            yield None, re.sub(r'<[^>]+>', '', title).strip()
            continue
        url = html.unescape(m.group(2))
        path, _, anchor = url.partition('#')
        if re.match(r'[a-z]+:', url) or not path.endswith('.html'):
            continue
        target = posixpath.normpath(posixpath.join(here, path))
        yield target + ('#' + anchor if anchor else ''), m.group(3).strip()


def contents(lang, pages):
    """El índice del idioma, del README.md: ([(sección, [(destino, texto,
    hijos)])], las páginas que salen). Los hijos de la portada de una carpeta
    (un README.md) son las páginas de más adentro a las que lleva que no
    salen ya en el índice, como (destino, texto): las lecciones del curso."""
    home = lang + '/index.html'
    sections, seen = [], {home}
    for target, text in page_links(home, pages[home]['body']):
        if target is None:
            sections.append((text, []))
        elif sections and target.startswith(lang + '/') and target not in seen:
            seen.add(target)
            sections[-1][1].append((target, text, []))
    for _, items in sections:
        for target, _, kids in items:
            if posixpath.basename(target) != 'index.html':
                continue
            inside = posixpath.dirname(target) + '/'
            for kid, text in page_links(target, pages[target]['body']):
                if kid and '#' not in kid and kid.startswith(inside) \
                        and posixpath.dirname(kid) + '/' != inside and kid not in seen:
                    seen.add(kid)
                    kids.append((kid, text))
    seen.update(t.partition('#')[0] for t in list(seen))
    return [s for s in sections if s[1]], seen


def sidebar(page, toc):
    """El índice, con la página marcada, y las páginas de dentro de la suya."""
    here = posixpath.dirname(page)

    def item(target, text):
        path, _, anchor = target.partition('#')
        url = posixpath.relpath(path, here) + ('#' + anchor if anchor else '')
        mark = ' class="aqui"' if target == page else ''
        return '<a href="%s"%s>%s</a>' % (html.escape(url), mark, text)

    out = []
    for section, items in toc:
        out.append('<p>%s</p>\n<ul>' % section)
        for target, text, kids in items:
            out.append('<li>' + item(target, text))
            if page == target or page in (k for k, _ in kids):
                out.append('<ul>')
                out.extend('<li>%s</li>' % item(k, t) for k, t in kids)
                out.append('</ul>')
            out.append('</li>')
        out.append('</ul>')
    return '\n'.join(out)


def steps(page, toc, w):
    """Anterior y siguiente, en el orden del índice (sin las anclas)."""
    order = [toc[0][1][0][0].split('/')[0] + '/index.html']
    for _, items in toc:
        for target, text, kids in items:
            if '#' not in target:
                order.append(target)
            order.extend(k for k, _ in kids)
    order = list(dict.fromkeys(order))
    if page not in order:
        return ''
    i, here, out = order.index(page), posixpath.dirname(page), []
    titles = dict((t, x) for _, items in toc for t, x, kids in items)
    for _, items in toc:
        for _, _, kids in items:
            titles.update(kids)

    def link(target, cls, word, arrow):
        text = titles.get(target, w['docs'])
        return '<a class="%s" href="%s"><small>%s</small>%s</a>' % (
            cls, html.escape(posixpath.relpath(target, here)), word, arrow % text)

    if i > 0:
        out.append(link(order[i - 1], 'anterior', w['prev'], '← %s'))
    if i + 1 < len(order):
        out.append(link(order[i + 1], 'siguiente', w['next'], '%s →'))
    return '<nav class="pasos">%s</nav>' % ''.join(out)


def convert(docs, src, pages, anchors, links):
    lang = src.split('/')[0]
    with open(os.path.join(docs, src), encoding='utf-8') as f:
        text = f.read()
    # Lo que solo vale con el código (cómo compilar el emulador...) no va en
    # los paquetes, que son públicos, y lo de <!-- public: ... --> sí
    # (dev/public/export.py hace lo mismo).
    text = re.sub(r'<!-- private -->.*?<!-- /private -->\n?', '', text, flags=re.S)
    text = re.sub(r'<!-- public: ?(.*?) ?-->', r'\1', text, flags=re.S)
    # La línea del idioma (**English** · [Español](...)) la pone la cabecera.
    text = re.sub(r'\A\s*(?:\*\*\w+\*\*|\[\w+\]\([^)]*\)) · (?:\*\*\w+\*\*|\[\w+\]\([^)]*\))\s*\n',
                  '', text)
    # El Markdown de dentro de <details>, como en GitHub.
    text = text.replace('<details>', '<details markdown="1">')
    md = markdown.Markdown(extensions=['tables', 'fenced_code', 'md_in_html', 'toc'],
                           extension_configs={'toc': {'slugify': slug}})
    body = md.convert(text)
    page = html_name(src)
    here = posixpath.dirname(src)

    def link(m):
        attr, url = m.group(1), html.unescape(m.group(2))
        if re.match(r'[a-z]+:', url):
            return m.group(0)
        path, _, anchor = url.partition('#')
        if not path:
            links.append((page, page, anchor))
            return m.group(0)
        full = posixpath.normpath(posixpath.join(here, path))
        if not full.startswith(('../', 'images/')) and not path.endswith('/') \
                and not full.endswith('.md'):
            # Los demás ficheros de docs/ (LICENSE) no van en el paquete.
            full = '../docs/' + full
        if full.startswith('../'):
            # Fuera de docs/: a GitHub (las carpetas con tree, los ficheros con blob).
            repo = posixpath.normpath(posixpath.join('docs', full))
            kind = 'tree' if os.path.isdir(os.path.join(docs, '..', repo)) else 'blob'
            url = '%s/%s/main/%s' % (REPO, kind, repo)
        else:
            if path.endswith('/'):
                full += '/README.md'
            if full.endswith('.md'):
                full = html_name(full)
                links.append((page, full, anchor))
            url = posixpath.relpath(full, here)
            if anchor:
                url += '#' + anchor
        return '%s="%s"' % (attr, html.escape(url))

    body = re.sub(r'(href|src)="([^"]*)"', link, body)
    # Un # al lado de cada título, para enlazar su sección.
    body = re.sub(r'<(h[2-4]) id="([^"]+)">(.*?)</\1>',
                  r'<\1 id="\2">\3<a class="ancla" href="#\2" aria-hidden="true">#</a></\1>', body)
    title = re.search(r'<h1[^>]*>(.*?)</h1>', body)
    title = re.sub(r'<[^>]+>', '', title.group(1)) if title else 'ZX-V'
    pages[page] = {'lang': lang, 'title': title, 'body': body}
    anchors[page] = set(re.findall(r'\bid="([^"]+)"', body))


if __name__ == '__main__':
    main()
