#include "landfallkeys.h"

#include <QFile>
#include <QFileInfo>
#include <QDateTime>

namespace LandfallKeys {

static QVector<Rule> s_Rules;
static QString s_Path;
static QDateTime s_Modified;

static bool parseCombo(const QByteArray& text, Rule& rule)
{
    rule.mods = 0;
    rule.key = SDL_SCANCODE_UNKNOWN;
    const QList<QByteArray> parts = text.split('+');
    for (int i = 0; i < parts.size(); i++) {
        QByteArray p = parts[i].trimmed();
        // "Ctrl++" (la tecla +): el ultimo pedazo vacio es la tecla.
        if (p.isEmpty() && i == parts.size() - 1) {
            p = "+";
        }
        QByteArray low = p.toLower();
        if (i < parts.size() - 1 && (low == "ctrl" || low == "control")) {
            rule.mods |= MOD_CTRL;
        }
        else if (i < parts.size() - 1 && low == "alt") {
            rule.mods |= MOD_ALT;
        }
        else if (i < parts.size() - 1 && low == "shift") {
            rule.mods |= MOD_SHIFT;
        }
        else if (i < parts.size() - 1 && (low == "super" || low == "win" || low == "cmd" || low == "gui")) {
            rule.mods |= MOD_SUPER;
        }
        else if (i == parts.size() - 1) {
            rule.key = SDL_GetScancodeFromName(p.constData());
        }
    }
    rule.text = text.trimmed();
    return rule.key != SDL_SCANCODE_UNKNOWN;
}

void load()
{
    if (s_Path.isEmpty()) {
        s_Path = qEnvironmentVariable("LANDFALL_TECLAS");
        if (s_Path.isEmpty()) {
            return;
        }
    }
    QFileInfo info(s_Path);
    if (!info.exists()) {
        s_Rules.clear();
        return;
    }
    if (info.lastModified() == s_Modified) {
        return;
    }
    QFile f(s_Path);
    if (!f.open(QIODevice::ReadOnly)) {
        return;
    }
    s_Modified = info.lastModified();
    s_Rules.clear();
    while (!f.atEnd()) {
        QByteArray line = f.readLine().trimmed();
        int space = line.indexOf(' ');
        if (line.isEmpty() || line.startsWith('#') || space < 0) {
            continue;
        }
        QByteArray side = line.left(space);
        Rule rule;
        if ((side == "aca" || side == "alla") && parseCombo(line.mid(space + 1), rule)) {
            rule.local = side == "aca";
            s_Rules.append(rule);
        }
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Landfall: %d key rules", (int)s_Rules.size());
}

int modsOf(Uint16 sdlMod)
{
    int mods = 0;
    if (sdlMod & KMOD_CTRL) {
        mods |= MOD_CTRL;
    }
    if (sdlMod & KMOD_ALT) {
        mods |= MOD_ALT;
    }
    if (sdlMod & KMOD_SHIFT) {
        mods |= MOD_SHIFT;
    }
    if (sdlMod & KMOD_GUI) {
        mods |= MOD_SUPER;
    }
    return mods;
}

const Rule* match(SDL_Scancode key, Uint16 sdlMod)
{
    int mods = modsOf(sdlMod);
    for (const Rule& rule : s_Rules) {
        if (rule.key == key && rule.mods == mods) {
            return &rule;
        }
    }
    return nullptr;
}

}
