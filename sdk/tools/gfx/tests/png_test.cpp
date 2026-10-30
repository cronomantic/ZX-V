/*
 * png_test.cpp - La importación de imágenes de zxv-gfx (png.cpp) con las
 * mismas órdenes que sdk/tools/png2gfx.py, para comparar los dos .gfx
 * (png_test.py):
 *
 *   png_test SALIDA.gfx [--flip] [--key RRGGBB] ORDEN...
 *     --sprite N IMAGEN [X,Y,AN,AL]   --sprites N IMAGEN AN,AL
 *     --tiles S N IMAGEN [X,Y,AN,AL]  --level M S IMAGEN
 *     --texture N IMAGEN [C]
 */
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QStringList>
#include <cstdio>

#include "../png.h"

using namespace gfx;

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QStringList a = app.arguments().mid(1);
    if (a.isEmpty())
        return 2;
    QString out = a.takeFirst();
    bool flip = a.contains("--flip");
    a.removeAll("--flip");
    QColor key;
    int k = int(a.indexOf("--key"));
    if (k >= 0) {
        key = QColor("#" + a.value(k + 1));
        a.remove(k, 2);
    }
    Document doc;
    auto load = [&](const QString &path) { return fromQImage(QImage(path), nullptr, key); };
    auto rect = [](const QString &s) {
        QStringList v = s.split(',');
        return QRect(v[0].toInt(), v[1].toInt(), v[2].toInt(), v[3].toInt());
    };
    auto isArg = [&](int i) { return i < a.size() && !a[i].startsWith("--"); };
    QString err;
    for (int i = 0; i < a.size() && err.isEmpty();) {
        QString cmd = a[i++];
        if (cmd == "--sprite") {
            int n = a[i++].toInt();
            Image img = load(a[i++]);
            err = Import::sprite(doc, n, img, isArg(i) ? rect(a[i++]) : QRect());
        } else if (cmd == "--sprites") {
            int n = a[i++].toInt();
            Image img = load(a[i++]);
            QStringList wh = a[i++].split(',');
            err = Import::sheet(doc, n, img, wh[0].toInt(), wh[1].toInt());
        } else if (cmd == "--tiles") {
            int s = a[i++].toInt(), n = a[i++].toInt();
            Image img = load(a[i++]);
            err = Import::tiles(doc, s, n, img, isArg(i) ? rect(a[i++]) : QRect());
        } else if (cmd == "--level") {
            int m = a[i++].toInt(), s = a[i++].toInt();
            err = Import::level(doc, m, s, load(a[i++]), flip);
        } else if (cmd == "--texture") {
            int n = a[i++].toInt();
            Image img = load(a[i++]);
            err = Import::texture(doc, n, img, isArg(i) ? a[i++].toInt() : -1);
        } else {
            err = "orden desconocida: " + cmd;
        }
    }
    if (!err.isEmpty()) {
        std::fprintf(stderr, "png_test: %s\n", qPrintable(err));
        return 1;
    }
    std::vector<uint8_t> data = doc.save();
    QFile f(out);
    if (!f.open(QIODevice::WriteOnly))
        return 1;
    f.write(reinterpret_cast<const char *>(data.data()), qint64(data.size()));
    return 0;
}
