#!/usr/bin/env python3
"""Keeps i18n/geocampo_<lang>.ts, the app's own translation layer, in sync with the source.

The app translator is searched before QField's (see QfTranslatorManager::appTranslator()),
so i18n/geocampo_<lang>.ts only holds:

- strings that QField's translation (i18n/qfield_<lang>.ts) leaves untranslated, and
- strings whose QField translation we change, e.g. to name the app GeoCampo.

QField's own files are left untouched, so they can keep following upstream.

The script runs lupdate over src/ into a temporary copy of i18n/qfield_<lang>.ts and
rewrites i18n/geocampo_<lang>.ts with every string that is still untranslated there,
plus every string already translated in the GeoCampo file. Strings that left the source
are dropped. Translate the "unfinished" entries with Qt Linguist or any text editor.

Usage:
    scripts/geocampo_translations.py pt_BR [--lupdate /usr/lib/qt6/bin/lupdate]
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from xml.sax.saxutils import escape

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def read_messages(path):
    """Returns {(context, source, comment): message dict} for the active messages of a .ts file."""
    messages = {}
    for context in ET.parse(path).getroot().iter("context"):
        context_name = context.findtext("name")
        for message in context.iter("message"):
            translation = message.find("translation")
            kind = translation.get("type")
            if kind in ("obsolete", "vanished"):
                continue
            numerus = message.get("numerus") == "yes"
            if numerus:
                text = ["".join(form.itertext()) for form in translation.iter("numerusform")]
                translated = any(form.strip() for form in text)
            else:
                text = "".join(translation.itertext())
                translated = bool(text.strip())
            key = (context_name, message.findtext("source"), message.findtext("comment") or "")
            messages[key] = {
                "numerus": numerus,
                "text": text,
                "finished": kind != "unfinished" and translated,
                "translated": translated,
            }
    return messages


def write_ts(path, language, entries):
    """Writes entries [(key, numerus, forms_or_text, finished)] grouped by context, in lupdate's layout."""
    lines = [
        '<?xml version="1.0" encoding="utf-8"?>',
        "<!DOCTYPE TS>",
        f'<TS version="2.1" language="{language}">',
    ]
    current_context = None
    for (context, source, comment), numerus, text, finished in entries:
        if context != current_context:
            if current_context is not None:
                lines.append("</context>")
            lines += ["<context>", f"    <name>{escape(context)}</name>"]
            current_context = context
        lines.append('    <message numerus="yes">' if numerus else "    <message>")
        lines.append(f"        <source>{escape(source)}</source>")
        if comment:
            lines.append(f"        <comment>{escape(comment)}</comment>")
        attribute = "" if finished else ' type="unfinished"'
        if numerus:
            lines.append(f"        <translation{attribute}>")
            lines += [f"            <numerusform>{escape(form)}</numerusform>" for form in text]
            lines.append("        </translation>")
        elif text:
            lines.append(f"        <translation{attribute}>{escape(text)}</translation>")
        else:
            lines.append(f"        <translation{attribute}></translation>")
        lines.append("    </message>")
    if current_context is not None:
        lines.append("</context>")
    lines.append("</TS>")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("language", help="language code, as in i18n/qfield_<language>.ts (e.g. pt_BR)")
    parser.add_argument("--lupdate", default=shutil.which("lupdate") or "/usr/lib/qt6/bin/lupdate")
    args = parser.parse_args()

    qfield_ts = os.path.join(ROOT, "i18n", f"qfield_{args.language}.ts")
    app_ts = os.path.join(ROOT, "i18n", f"geocampo_{args.language}.ts")
    if not os.path.exists(qfield_ts):
        sys.exit(f"{qfield_ts} not found")

    with tempfile.TemporaryDirectory() as tmp:
        synced_ts = os.path.join(tmp, os.path.basename(qfield_ts))
        shutil.copy(qfield_ts, synced_ts)
        subprocess.run(
            [args.lupdate, "-silent", "-locations", "none", "-recursive", "src", "-ts", synced_ts],
            cwd=ROOT,
            check=True,
        )
        qfield = read_messages(synced_ts)

    ours = read_messages(app_ts) if os.path.exists(app_ts) else {}

    entries = []
    pending = 0
    for key, upstream in qfield.items():
        mine = ours.get(key)
        if mine and mine["translated"]:
            entries.append((key, upstream["numerus"], mine["text"], mine["finished"]))
            pending += not mine["finished"]
        elif not upstream["finished"]:
            empty = [""] * max(len(upstream["text"]), 2) if upstream["numerus"] else ""
            entries.append((key, upstream["numerus"], empty, False))
            pending += 1

    entries.sort(key=lambda entry: entry[0])
    write_ts(app_ts, args.language, entries)
    print(f"{os.path.relpath(app_ts, ROOT)}: {len(entries)} strings, {pending} still to translate")


if __name__ == "__main__":
    main()
