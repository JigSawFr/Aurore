#!/usr/bin/env python3
"""Builds the website's API reference (site/README.md) from what Doxygen read
of Aurore's public headers (docs/Doxyfile, its XML in build_docs/xml): a page
per namespace and per class or struct, a home page that sorts them by theme,
the index of every class, the macros, and search.json for the search box. The
pages are the site's (tools/build_site.py's page(), its header, its footer,
site.css), with site/assets/api.css and api.js for what is the API's own.

They are written in <out-dir> as /api/'s content: `SITE_API=<out-dir>
tools/build_site.py` publishes them with the site. The URLs follow the names:
/api/aurore/View/, /api/cadran/kit/Chip/, /api/ui/ (a namespace). Each page
of Doxygen's old HTML (classaurore_1_1View.html) sends its visitor on to the
new one.

The API is in English, as the headers: the pages wear the site's English
header, whose language menu leads to each language's home page.

Usage: tools/build_api.py <xml-dir> <out-dir> [repo-root]
"""
import html
import json
import os
import re
import sys
import xml.etree.ElementTree as ET

sys.dont_write_bytecode = True   # no __pycache__ beside build_site.py
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build_site as site  # noqa: E402  (its page(), its texts, its highlighting)

# The namespaces that are not Aurore's API: the standard library's and
# tinyxml2's (forward declarations), and the implementation's own.
SKIPPED = {"std", "tinyxml2", "aurore::detail", "aurore::internal"}
# The macros an app writes (the others are the headers' own business).
MACROS = re.compile(r"^(AURORE_|CONTENT_FROM_XML_)")

# The home page: the API by theme, a sentence and the names for each. Each
# name must be a class, a namespace or a member Doxygen read: the build says
# which is not any more.
THEMES = [
    ("app", "Application and activities",
     "The application's life, from <code>Application::init()</code> to its main loop, and the activities it "
     "pushes and pops, each a screen described in XML or in code.",
     ["aurore::Application", "aurore::Activity", "aurore::Event", "aurore::Subscription", "aurore::Observable",
      "aurore::NotificationManager", "aurore::Threading", "aurore::Logger", "aurore::HotReload",
      "aurore::Inspector", "aurore::Profiler"]),
    ("layout", "Views and layout",
     "Every view is an <code>aurore::View</code>, laid out by flexbox (Yoga) in an <code>aurore::Box</code>; "
     "XML files create them, <code>AURORE_BIND</code> finds them again in code.",
     ["aurore::View", "aurore::Box", "aurore::Padding", "aurore::BoundView", "AURORE_BIND",
      "CONTENT_FROM_XML_RES", "aurore::Point", "aurore::Size", "aurore::Rect", "aurore::Axis",
      "aurore::JustifyContent", "aurore::AlignItems", "aurore::Visibility", "aurore::ViewNotFoundException"]),
    ("views", "Built-in views",
     "The views an app is made of: text, images, buttons, frames that scroll or recycle their cells, "
     "tabs, a sidebar, dialogs and the cells of a settings list.",
     ["aurore::Label", "aurore::Image", "aurore::Button", "aurore::IconButton", "aurore::Rectangle",
      "aurore::ProgressSpinner", "aurore::Slider", "aurore::ScrollingFrame", "aurore::HScrollingFrame",
      "aurore::RecyclerFrame", "aurore::RecyclingGrid", "aurore::AppletFrame", "aurore::TabFrame",
      "aurore::Sidebar", "aurore::Header", "aurore::BottomBar", "aurore::Hints", "aurore::Dialog",
      "aurore::Dropdown", "aurore::EditTextDialog", "aurore::ContextMenu", "aurore::DetailCell",
      "aurore::BooleanCell", "aurore::RadioCell", "aurore::SelectorCell", "aurore::SliderCell",
      "aurore::InputCell", "aurore::BatteryWidget", "aurore::WirelessWidget"]),
    ("cadran", "Cadran",
     "Aurore's look and its kit: <code>ui::</code> opens dialogs, sheets, pickers and notices in one call, "
     "<code>cadran::</code> styles the frames and builds cards and lists, and its views draw the rest.",
     ["ui", "cadran", "cadran::kit", "cadran::List", "cadran::Card", "cadran::Switch", "cadran::palette",
      "ui::Prefs", "ui::hooks", "motion", "effects", "text_size", "markdown", "CadranDigits", "LoadingBar",
      "QrView", "ScrollView", "FooterHints", "VersionBadge"]),
    ("themes", "Themes and styles",
     "The colours, metrics and pictures of a theme file, read at start-up and switched while the app "
     "runs, and the styles that size each view.",
     ["aurore::Theme", "aurore::ThemeValues", "aurore::ThemeInfo", "aurore::ThemeDecorations",
      "aurore::ThemeImages", "aurore::Style", "aurore::StyleValues", "aurore::ButtonStyle",
      "aurore::ThemeVariant"]),
    ("input", "Input",
     "A controller, a keyboard, a mouse or a touch screen: the actions views answer to, the focus that "
     "moves between them, and the gestures a touch makes.",
     ["aurore::InputManager", "aurore::Action", "aurore::ControllerButton", "aurore::ControllerState",
      "aurore::KeyState", "aurore::FocusDirection", "aurore::ImeManager", "aurore::GestureRecognizer",
      "aurore::TapGestureRecognizer", "aurore::PanGestureRecognizer", "aurore::ScrollGestureRecognizer",
      "aurore::LongPressGestureRecognizer"]),
    ("time", "Animations and timers",
     "Values that move from one to another over time, timers that call back once or again, all driven "
     "by the frames the application draws.",
     ["aurore::Animatable", "aurore::Ticking", "aurore::FiniteTicking", "aurore::Timer",
      "aurore::RepeatingTimer", "aurore::RepeatingTask", "aurore::EasingFunction"]),
    ("i18n", "I18n",
     "Strings read from the app's catalogs in the console's language, with their arguments; "
     "<code>\"…\"_i18n</code> for those without.",
     ["aurore::getStr", "aurore::loadTranslations", "aurore::localeFromSystem", "aurore::literals"]),
    ("platform", "Platform",
     "What each console or system provides behind one interface: the window and its drawing, the "
     "fonts, the sounds, the textures.",
     ["aurore::Platform", "VideoContext", "aurore::FrameContext", "aurore::FontLoader", "aurore::AudioPlayer",
      "aurore::TextureCache"]),
    ("testing", "Testing",
     "The app's screens run without a window: a headless platform, a harness that drives them and "
     "compares what they draw.",
     ["aurore::test", "aurore::test::Harness", "aurore::test::Options", "aurore::HeadlessPlatform",
      "aurore::HeadlessInputManager", "aurore::HeadlessVideoContext", "AURORE_TEST", "AURORE_CHECK"]),
]

KIND_NAMES = {"class": "Class", "struct": "Struct", "union": "Union", "namespace": "Namespace"}
# Doxygen's characters as elements (<ndash/>…).
CHARS = {"ndash": "–", "mdash": "—", "lsquo": "‘", "rsquo": "’", "ldquo": "“", "rdquo": "”", "nonbreakablespace": " ",
         "zwj": "‍", "zwnj": "‌", "nzwj": "", "sp": " ", "copy": "©", "trade": "™", "reg": "®",
         "laquo": "«", "raquo": "»", "hellip": "…", "times": "×", "middot": "·", "deg": "°", "rarr": "→",
         "larr": "←", "le": "≤", "ge": "≥", "ne": "≠", "minus": "−", "plusmn": "±", "bull": "•"}
# The sections of a class page, in their order: (id, heading).
SECTIONS = [("types", "Types"), ("constructors", "Constructors"), ("functions", "Functions"),
            ("static-functions", "Static functions"), ("attributes", "Attributes"),
            ("static-attributes", "Static attributes"), ("protected-functions", "Protected functions"),
            ("protected-attributes", "Protected attributes"), ("friends", "Friends")]
# The ids a page has already: a member's anchor takes another.
RESERVED = {"main", "description", "nested", "sites", "languages", "doc-results"}
# A namespace page's.
NS_SECTIONS = [("namespaces", "Namespaces"), ("classes", "Classes"), ("functions", "Functions"),
               ("enums", "Enums"), ("types", "Types"), ("variables", "Variables")]
RESERVED |= {k for k, _ in SECTIONS + NS_SECTIONS}


def text_of(e):
    return " ".join("".join(e.itertext()).split()) if e is not None else ""


def slug(name):
    return re.sub(r"[^A-Za-z0-9_]+", "-", name).strip("-") or "operator"


def path_of(name):
    """/api/'s folder of a qualified name: aurore::View → aurore/View/."""
    return "/".join(slug(p) for p in name.split("::")) + "/"


class Api:
    """Every class, namespace and member Doxygen read, and where each is."""

    def __init__(self, folder):
        self.folder = folder
        self.compounds = {}      # refid → compounddef, the ones published
        self.url = {}            # refid (compound or member) → URL under /api/
        self.members = {}        # qualified name → (refid, kind) of a namespace's member or a macro
        self.names = {}          # qualified name → refid of a compound
        self.problems = []
        self.files = {}          # refid of a header → its path in the repository
        index = ET.parse(os.path.join(folder, "index.xml")).getroot()
        loaded = {}
        for c in index.findall("compound"):
            kind = c.get("kind")
            if kind == "file":
                loaded[c.get("refid")] = None
            if kind not in ("class", "struct", "union", "namespace", "file"):
                continue
            root = ET.parse(os.path.join(folder, c.get("refid") + ".xml")).getroot()
            loaded[c.get("refid")] = root.find("compounddef")
        for refid, cd in loaded.items():
            if cd is None:
                continue
            kind, name = cd.get("kind"), cd.findtext("compoundname")
            if kind == "file":
                location = cd.find("location")
                self.files[refid] = "library/include/" + location.get("file") if location is not None else None
                continue
            if not self.published(name, cd):
                continue
            self.compounds[refid] = cd
            self.names[name] = refid
            self.url[refid] = path_of(name)
        # The namespaces with nothing of their own (no member, no class shown).
        for refid, cd in list(self.compounds.items()):
            if cd.get("kind") == "namespace" and not self.namespace_content(cd):
                del self.compounds[refid], self.url[refid], self.names[cd.findtext("compoundname")]
        seen = {}
        for refid, url in self.url.items():
            if url.lower() in seen:
                raise SystemExit(f"api: {url} for both {refid} and {seen[url.lower()]}")
            seen[url.lower()] = refid
        # The members, an anchor each on their compound's page.
        for refid, cd in self.compounds.items():
            anchors = set(RESERVED)
            for m in self.shown_members(cd):
                self.add_member(refid, m, anchors)
        # The macros, on their own page.
        self.macros = []
        taken = set()
        for refid, cd in loaded.items():
            if cd is None or cd.get("kind") != "file":
                continue
            for m in cd.iter("memberdef"):
                name = m.findtext("name")
                if m.get("kind") == "define" and MACROS.match(name) and name not in taken:
                    taken.add(name)
                    self.macros.append(m)
                    self.url[m.get("id")] = f"macros/#{slug(name)}"
                    self.members[name] = (m.get("id"), "macro")
        self.macros.sort(key=lambda m: m.findtext("name"))

    def published(self, name, cd):
        if cd.get("prot") in ("private", "protected"):
            return False
        parts = name.split("::")
        return not any("::".join(parts[:i]) in SKIPPED for i in range(1, len(parts) + 1))

    def namespace_content(self, cd):
        inner = [i for i in cd.findall("innerclass") + cd.findall("innernamespace") if i.get("refid") in self.url]
        return inner or list(self.shown_members(cd))

    def shown_members(self, cd):
        for section in cd.findall("sectiondef"):
            for m in section.findall("memberdef"):
                if m.get("prot") == "private":
                    continue
                if m.get("kind") == "define":
                    continue
                yield m

    def add_member(self, refid, m, anchors):
        name = m.findtext("name")
        anchor, n = slug(name), 2
        while anchor in anchors:
            anchor = f"{slug(name)}-{n}"
            n += 1
        anchors.add(anchor)
        m.set("anchor", anchor)
        self.url[m.get("id")] = f"{self.url[refid]}#{anchor}"
        cd = self.compounds[refid]
        if cd.get("kind") == "namespace":
            qualified = f"{cd.findtext('compoundname')}::{name}"
            self.members.setdefault(qualified, (m.get("id"), m.get("kind")))
        for value in m.findall("enumvalue"):
            value_anchor = f"{anchor}-{slug(value.findtext('name'))}"
            value.set("anchor", value_anchor)
            self.url[value.get("id")] = f"{self.url[refid]}#{value_anchor}"

    def link(self, refid, to_api):
        url = self.url.get(refid)
        if url is not None:
            return to_api + url
        if refid in self.files and self.files[refid]:
            return f"{site.REPO_URL}/blob/main/{self.files[refid]}"
        return None

    def lookup(self, name):
        """A name of the home page: its URL (under /api/) and what it is."""
        if name in self.names:
            cd = self.compounds[self.names[name]]
            return self.url[self.names[name]], cd.get("kind")
        if name in self.members:
            refid, kind = self.members[name]
            return self.url[refid], kind
        self.problems.append(f"api: {name}, on the home page, is not in the headers any more")
        return None, None


# ---- Doxygen's markup as HTML --------------------------------------------------

class Render:
    """A page's descriptions and signatures, its links relative to it."""

    def __init__(self, api, to_api, heading=3):
        self.api, self.to_api, self.heading = api, to_api, heading

    def ref(self, e, inner):
        href = self.api.link(e.get("refid"), self.to_api)
        return f'<a href="{html.escape(href)}">{inner}</a>' if href else inner

    def inline(self, e, code=False):
        """`e`'s content as inline HTML (its text, then its children's)."""
        out = [self.text(e.text, code)]
        for c in e:
            out.append(self.inline_element(c, code))
            out.append(self.text(c.tail, code))
        return "".join(out)

    def text(self, s, code):
        if not s:
            return ""
        return site.highlight(s, "cpp") if code else html.escape(s, quote=False)

    def inline_element(self, c, code):
        tag = c.tag
        if tag == "ref":
            return self.ref(c, self.inline(c, code))
        if tag == "computeroutput":
            return f"<code>{self.inline(c)}</code>"
        if tag == "bold":
            return f"<strong>{self.inline(c, code)}</strong>"
        if tag in ("emphasis", "underline"):
            return f"<em>{self.inline(c, code)}</em>"
        if tag == "strike" or tag == "del":
            return f"<del>{self.inline(c, code)}</del>"
        if tag == "ulink":
            return f'<a href="{html.escape(c.get("url", ""))}">{self.inline(c)}</a>'
        if tag == "linebreak":
            return "<br>"
        if tag in CHARS:
            return CHARS[tag]
        if tag == "anchor":
            return ""
        if tag in ("subscript", "superscript"):
            t = "sub" if tag == "subscript" else "sup"
            return f"<{t}>{self.inline(c, code)}</{t}>"
        if tag == "highlight":
            return self.inline(c, code)
        self.api.problems.append(f"api: <{tag}> in a description, not rendered as such")
        return self.inline(c, code)

    def blocks(self, e):
        """A description (briefdescription, detaileddescription…) as blocks."""
        if e is None:
            return ""
        out = []
        for c in e:
            out.append(self.block(c))
        return "".join(out)

    def para(self, p):
        """A <para>: its inline content in <p>s, around the blocks Doxygen
        puts in it (lists, code, notes, parameters)."""
        out, current = [], [self.text(p.text, False)]

        def flush():
            content = "".join(current).strip()
            if content:
                out.append(f"<p>{content}</p>")
            current.clear()

        for c in p:
            if c.tag in BLOCK_TAGS:
                flush()
                out.append(self.block(c))
            else:
                current.append(self.inline_element(c, False))
            current.append(self.text(c.tail, False))
        flush()
        return "".join(out)

    def block(self, c):
        tag = c.tag
        if tag == "para":
            return self.para(c)
        if tag in ("itemizedlist", "orderedlist"):
            t = "ul" if tag == "itemizedlist" else "ol"
            items = []
            for item in c.findall("listitem"):
                paras = item.findall("para")
                if len(paras) == 1 and len(item) == 1:
                    body = self.para(paras[0])
                    body = body[3:-4] if body.startswith("<p>") and body.endswith("</p>") and body.count("<p>") == 1 else body
                else:
                    body = self.blocks(item)
                items.append(f"<li>{body}</li>")
            return f"<{t}>{''.join(items)}</{t}>"
        if tag == "programlisting":
            lines = []
            for line in c.findall("codeline"):
                lines.append("".join(self.code_text(x) for x in line))
            lang = (c.get("filename") or ".cpp").lstrip(".") or "cpp"
            return code_block("\n".join(lines), lang)
        if tag in ("verbatim", "preformatted"):
            return f'<pre tabindex="0"><code>{html.escape(c.text or "")}</code></pre>'
        if tag == "simplesect":
            return self.simplesect(c)
        if tag == "parameterlist":
            return self.parameters(c)
        if tag in ("sect1", "sect2", "sect3", "sect4"):
            level = min(6, self.heading + int(tag[-1]) - 1)
            title = self.inline(c.find("title")) if c.find("title") is not None else ""
            rest = "".join(self.block(x) for x in c if x.tag != "title")
            return f"<h{level}>{title}</h{level}>{rest}"
        if tag == "heading":
            level = min(6, self.heading + int(c.get("level", "1")))
            return f"<h{level}>{self.inline(c)}</h{level}>"
        if tag == "hruler":
            return "<hr>"
        if tag == "blockquote":
            return f"<blockquote>{self.blocks(c)}</blockquote>"
        if tag == "table":
            rows = []
            for row in c.findall("row"):
                cells = "".join(
                    f'<{"th" if entry.get("thead") == "yes" else "td"}>{self.blocks(entry)}</{"th" if entry.get("thead") == "yes" else "td"}>'
                    for entry in row.findall("entry"))
                rows.append(f"<tr>{cells}</tr>")
            return f'<div class="table" tabindex="0"><table>{"".join(rows)}</table></div>'
        if tag == "xrefsect":
            title = c.findtext("xreftitle") or ""
            kind = "warning" if title.lower() == "deprecated" else "note"
            return (f'<div class="callout {kind}"><p><strong>{html.escape(title)}</strong></p>'
                    f'{self.blocks(c.find("xrefdescription"))}</div>')
        if tag in ("title", "internal"):
            return self.blocks(c)
        if tag in CHARS or tag in INLINE_TAGS:
            return self.inline_element(c, False)
        self.api.problems.append(f"api: <{tag}> in a description, not rendered as such")
        return self.blocks(c)

    def code_text(self, e):
        text = " " if e.tag == "sp" else (e.text or "") + "".join(self.code_text(x) for x in e)
        return text + (e.tail or "")

    def simplesect(self, c):
        kind = c.get("kind")
        body = self.blocks(c)
        if kind in ("note", "remark", "attention", "warning", "important"):
            cls = {"warning": "warning", "attention": "important", "important": "important"}.get(kind, "note")
            return f'<div class="callout {cls}">{body}</div>'
        labels = {"return": "Returns", "see": "See also", "since": "Since", "pre": "Before", "post": "After",
                  "author": "Author", "authors": "Authors", "version": "Version", "date": "Date",
                  "invariant": "Invariant", "copyright": "Copyright", "rcs": "", "par": ""}
        label = labels.get(kind, kind.capitalize())
        if kind == "par" and c.find("title") is not None:
            label = self.inline(c.find("title"))
        return f'<dl class="api-facts"><dt>{label}</dt><dd>{body}</dd></dl>'

    def parameters(self, c):
        kind = c.get("kind")
        label = {"param": "Parameters", "templateparam": "Template parameters", "retval": "Return values",
                 "exception": "Throws"}.get(kind, "Parameters")
        rows = []
        for item in c.findall("parameteritem"):
            names = ", ".join(f"<code>{self.inline(n)}</code>" for n in item.iter("parametername"))
            direction = {p.get("direction") for p in item.iter("parametername") if p.get("direction")}
            if direction:
                names += f' <span class="api-dir">{"/".join(sorted(direction))}</span>'
            rows.append(f"<dt>{names}</dt><dd>{self.blocks(item.find('parameterdescription'))}</dd>")
        return f'<div class="api-params"><p class="api-label">{label}</p><dl>{"".join(rows)}</dl></div>'

    # ---- Signatures ----

    def code(self, e):
        """A type, an initializer: code, its names linked."""
        return self.inline(e, code=True).strip() if e is not None else ""

    def signature(self, m, owner):
        """A member as its declaration reads, the names it uses linked."""
        kind = m.get("kind")
        name = html.escape(m.findtext("name"))
        lines = []
        tpl = m.find("templateparamlist")
        if tpl is not None:
            lines.append(self.template_line(tpl))
        words = []
        if m.get("static") == "yes" and kind in ("function", "variable"):
            words.append("static")
        if m.get("virt") in ("virtual", "pure-virtual"):
            words.append("virtual")
        if m.get("explicit") == "yes":
            words.append("explicit")
        if m.get("constexpr") == "yes":
            words.append("constexpr")
        prefix = "".join(f'<span class="tok-keyword">{w}</span> ' for w in words)
        if kind == "function":
            type_ = self.code(m.find("type"))
            params = [self.param(p) for p in m.findall("param")]
            args = m.findtext("argsstring") or ""
            tail = html.escape(after_parens(args)).strip()
            tail = re.sub(r"\b(const|override|final|noexcept|delete|default|volatile)\b",
                          r'<span class="tok-keyword">\1</span>', tail)
            head = f'{prefix}{type_ + " " if type_ else ""}<span class="sig-name">{name}</span>('
            plain_len = len(re.sub(r"<[^>]+>", "", head)) + sum(len(html.unescape(re.sub(r"<[^>]+>", "", p))) + 2 for p in params)
            if params and plain_len > 72:
                body = head + ",".join(f"\n    {p}" for p in params) + ")"
            else:
                body = head + ", ".join(params) + ")"
            lines.append(body + (" " + tail if tail else ""))
        elif kind == "variable":
            type_ = self.code(m.find("type"))
            init = self.code(m.find("initializer"))
            args = html.escape(m.findtext("argsstring") or "")
            lines.append(f'{prefix}{type_} <span class="sig-name">{name}</span>{args}{" " + init if init else ""}')
        elif kind == "enum":
            strong = '<span class="tok-keyword">class</span> ' if m.get("strong") == "yes" else ""
            type_ = self.code(m.find("type"))
            lines.append(f'<span class="tok-keyword">enum</span> {strong}<span class="sig-name">{name}</span>'
                         f'{" : " + type_ if type_ else ""}')
        elif kind == "typedef":
            definition = m.findtext("definition") or ""
            type_ = self.code(m.find("type"))
            if definition.startswith("using"):
                lines.append(f'<span class="tok-keyword">using</span> <span class="sig-name">{name}</span> = {type_}')
            else:
                args = html.escape(m.findtext("argsstring") or "")
                lines.append(f'<span class="tok-keyword">typedef</span> {type_} <span class="sig-name">{name}</span>{args}')
        elif kind == "define":
            params = [html.escape(p.findtext("defname") or "") for p in m.findall("param")]
            args = f"({', '.join(params)})" if m.find("param") is not None else ""
            lines.append(f'<span class="tok-pre">#define</span> <span class="sig-name">{name}</span>{args}')
        elif kind == "friend":
            type_ = re.sub(r'^<span class="tok-keyword">friend</span>\s*', "", self.code(m.find("type")))
            args = html.escape(m.findtext("argsstring") or "")
            lines.append(f'<span class="tok-keyword">friend</span> {type_} <span class="sig-name">{name}</span>{args}')
        else:
            lines.append(html.escape(m.findtext("definition") or name))
        return "\n".join(lines)

    def param(self, p):
        type_ = self.code(p.find("type"))
        declname = html.escape(p.findtext("declname") or p.findtext("defname") or "")
        array = html.escape(p.findtext("array") or "")
        default = self.code(p.find("defval"))
        out = type_
        if declname:
            out += f'{"" if out.endswith(("*", "&amp;")) else " "}<span class="sig-param">{declname}</span>'
        out += array
        if default:
            out += f" = {default}"
        return out

    def template_line(self, tpl):
        params = []
        for p in tpl.findall("param"):
            text = self.code(p.find("type"))
            if p.findtext("declname"):
                text += " " + html.escape(p.findtext("declname"))
            if p.find("defval") is not None:
                text += " = " + self.code(p.find("defval"))
            params.append(text)
        return f'<span class="tok-keyword">template</span> &lt;{", ".join(params)}&gt;'


BLOCK_TAGS = {"itemizedlist", "orderedlist", "programlisting", "verbatim", "preformatted", "simplesect",
              "parameterlist", "table", "xrefsect", "heading", "hruler", "blockquote", "sect1", "sect2", "sect3"}
INLINE_TAGS = {"ref", "computeroutput", "bold", "emphasis", "ulink", "linebreak", "anchor"}


def after_parens(args):
    """What follows a function's parameters in its argsstring: const, override, = 0…"""
    depth, start = 0, args.find("(")
    if start < 0:
        return args
    for i in range(start, len(args)):
        if args[i] == "(":
            depth += 1
        elif args[i] == ")":
            depth -= 1
            if depth == 0:
                return args[i + 1:]
    return ""


def code_block(code, lang="cpp"):
    """Code from a comment, as the documentation's (site.js adds its copy button)."""
    lang = {"hpp": "cpp", "h": "cpp", "cc": "cpp", "cxx": "cpp", "unparsed": "", "txt": ""}.get(lang, lang)
    return f'<pre tabindex="0"><code class="language-{lang or "text"}">{site.highlight(code, lang)}</code></pre>'


# ---- Pages ---------------------------------------------------------------------

def kind_of(cd):
    return KIND_NAMES.get(cd.get("kind"), cd.get("kind").capitalize())


def short_name(name):
    return name.split("::")[-1]


def has_text(e):
    return e is not None and text_of(e) != ""


def first_sentence(cd_or_m):
    """The brief, else the first sentence of the details: a line in a list."""
    brief = text_of(cd_or_m.find("briefdescription"))
    if brief:
        return brief
    detail = cd_or_m.find("detaileddescription")
    para = detail.find("para") if detail is not None else None
    if para is None:
        return ""
    words = " ".join("".join(t for t in para.itertext()).split())
    m = re.match(r"(.+?[.!?])(\s|$)", words)
    sentence = m.group(1) if m else words
    return sentence if len(sentence) <= 180 else sentence[:177].rsplit(" ", 1)[0] + "…"


def brief_html(r, e):
    """A brief as inline HTML (its paragraph's content)."""
    out = r.blocks(e.find("briefdescription")).strip()
    return out[3:-4] if out.startswith("<p>") and out.endswith("</p>") and out.count("<p>") == 1 else out


def member_section(m, cd):
    """Where a class's member goes (SECTIONS' id)."""
    kind, prot, static = m.get("kind"), m.get("prot"), m.get("static") == "yes"
    name = m.findtext("name")
    if kind == "friend":
        return "friends"
    if kind in ("enum", "typedef"):
        return "types"
    if kind == "function":
        if name in (short_name(cd.findtext("compoundname")), "~" + short_name(cd.findtext("compoundname"))):
            return "constructors"
        if prot == "protected":
            return "protected-functions"
        return "static-functions" if static else "functions"
    if kind == "variable":
        if prot == "protected":
            return "protected-attributes"
        return "static-attributes" if static else "attributes"
    return "functions"


def member_html(r, m, owner_name):
    """A member: its signature in the code's look, then what its comment
    says, an enum's values, and where the header declares it."""
    anchor = m.get("anchor")
    sig = r.signature(m, owner_name)
    source = m.find("location")
    details = [x for x in (r.blocks(m.find("briefdescription")), r.blocks(m.find("detaileddescription"))) if x]
    over = m.find("reimplements")
    if over is not None:
        href = r.api.link(over.get("refid"), r.to_api)
        target = r.api.url.get(over.get("refid"), "")
        owner = target.split("#")[0].rstrip("/").replace("/", "::")
        if href:
            details.append(f'<p class="api-over">Overrides <a href="{href}"><code>{html.escape(owner)}::'
                           f'{html.escape(over.text or "")}</code></a></p>')
    values = enum_values(r, m) if m.get("kind") == "enum" else ""
    link = ""
    if source is not None and source.get("file"):
        link = (f'<a class="api-source" href="{site.REPO_URL}/blob/main/library/include/{source.get("file")}'
                f'#L{source.get("line")}">{html.escape(os.path.basename(source.get("file")))}:{source.get("line")}</a>')
    if values:
        doc = (f'<div class="api-doc">{"".join(details)}</div>' if details else "") + values + \
              (f'<p class="api-foot">{link}</p>' if link else "")
    else:
        doc = f'<div class="api-doc">{"".join(details)}{link}</div>' if details else ""
    return (f'<section class="api-member{"" if doc else " bare"}" id="{anchor}">'
            f'<div class="api-sig"><a class="api-anchor" href="#{anchor}" aria-label="Link to {html.escape(m.findtext("name"))}"></a>'
            f'<code class="sig" tabindex="0">{sig}</code></div>'
            f'{doc}</section>')


def enum_values(r, m):
    """An enum's values: a table, its columns only those something fills."""
    values = []
    for v in m.findall("enumvalue"):
        init = r.code(v.find("initializer"))
        init = re.sub(r"^=\s*", "", init)
        doc = r.blocks(v.find("briefdescription")) + r.blocks(v.find("detaileddescription"))
        values.append((v, init, doc))
    if not values:
        return ""
    inits, docs = any(i for _, i, _ in values), any(d for _, _, d in values)
    rows = []
    for v, init, doc in values:
        cells = f'<td class="v-name"><code>{html.escape(v.findtext("name"))}</code></td>'
        if inits:
            cells += f'<td class="v-init">{f"<code>{init}</code>" if init else ""}</td>'
        if docs:
            cells += f'<td class="v-doc">{doc}</td>'
        rows.append(f'<tr id="{v.get("anchor")}">{cells}</tr>')
    head = "<th>Value</th>" + ("<th>Initializer</th>" if inits else "") + ("<th>Description</th>" if docs else "")
    return (f'<div class="table api-values" tabindex="0"><table><thead><tr>{head}</tr></thead>'
            f'<tbody>{"".join(rows)}</tbody></table></div>')


def members_html(r, members, owner_name):
    """Members one after the other; those with nothing but their signature
    gathered in one block of code, as the header lists them."""
    out, bare = [], []

    def flush():
        if bare:
            out.append('<div class="api-bare">' + "".join(bare) + "</div>")
            bare.clear()

    for m in members:
        documented = has_text(m.find("briefdescription")) or has_text(m.find("detaileddescription")) \
            or m.get("kind") == "enum" or m.find("reimplements") is not None
        if documented:
            flush()
            out.append(member_html(r, m, owner_name))
        else:
            anchor = m.get("anchor")
            bare.append(f'<div class="api-line" id="{anchor}"><a class="api-anchor" href="#{anchor}" '
                        f'aria-label="Link to {html.escape(m.findtext("name"))}"></a>'
                        f'<code class="sig" tabindex="0">{r.signature(m, owner_name)}</code></div>')
    flush()
    return "".join(out)


def section(id_, title, body, count=None):
    small = f" <small>{count}</small>" if count is not None else ""
    return (f'<section class="api-section" aria-labelledby="{id_}"><h2 id="{id_}">{title}{small}'
            f'<a class="anchor" href="#{id_}" aria-label="Link to this section"></a></h2>{body}</section>')


def class_list(api, r, refids):
    rows = []
    for refid in sorted(refids, key=lambda i: api.compounds[i].findtext("compoundname").lower()):
        cd = api.compounds[refid]
        rows.append(f'<li><a href="{r.to_api}{api.url[refid]}"><code>{html.escape(cd.findtext("compoundname"))}</code></a>'
                    f'<span>{html.escape(first_sentence(cd)) or kind_of(cd)}</span></li>')
    return f'<ul class="api-list">{"".join(rows)}</ul>'


class Builder:
    def __init__(self, api, out):
        self.api, self.out = api, out
        self.lang = site.LANGUAGES[0][0]
        self.t = site.Texts(self.lang)
        self.search = []
        self.count = 0
        # Each class's theme (the sidebar opens it).
        self.theme_of = {}
        for key, _, _, names in THEMES:
            for name in names:
                url, _ = api.lookup(name)
                if url:
                    self.theme_of.setdefault(url, key)

    # The page around the content: the site's, its links to the other
    # languages leading to their home page (the API is in English only).
    def page(self, path, title, description, article, toc, current_theme=None, crumbs=()):
        depth = path.count("/")
        to_api = "../" * depth
        to_root = to_api + "../"
        t = self.t
        nav = self.sidebar(to_api, path, current_theme)
        toc_items = "".join(f'<li><a href="#{i}">{html.escape(n)}</a></li>' for i, n in toc)
        folded = (f'<details class="toc-mobile"><summary>{t("on_this_page")}</summary><ul>{toc_items}</ul></details>\n'
                  if len(toc) > 1 else "")
        # The guides' layout (its class, which page() looks for to add the
        # slim bar once the header is scrolled away), in the API's.
        body = f"""  <div class="api"><div class="wrap doc">
    <nav class="doc-nav" aria-label="{t.plain('api_title')}">
      {site.reading_toggle(t)}
      <div class="doc-search" hidden><input type="search" placeholder="Search the API" aria-label="Search the API" aria-controls="doc-results" autocomplete="off" spellcheck="false" enterkeyhint="search" data-index="{to_api}search.json" data-base="{to_api}" data-none="{t.plain('search_none')}"><kbd class="search-key" aria-hidden="true">/</kbd><ol id="doc-results" class="doc-results" hidden></ol></div>
      <details class="doc-menu" open><summary>{t("api_title")}</summary>
{nav}
      </details>
    </nav>
    <article class="prose api-page">
      {article.replace("<!--toc-->", folded)}
      <p class="edit">Read from Aurore's headers by Doxygen at each build · <a href="{site.REPO_URL}/tree/main/library/include">library/include</a></p>
    </article>
    {f'<nav class="toc" aria-label="{t.plain("on_this_page")}"><p>{t("on_this_page")}</p><ul>{toc_items}</ul></nav>' if len(toc) > 1 else ""}
  </div></div>"""
        home = f"{site.SITE_URL}{site.lang_dir(self.lang)}"
        trail = [("Aurore", home), (t.plain("api_title"), f"{site.SITE_URL}api/")] + \
                [(n, f"{site.SITE_URL}api/{u}") for n, u in crumbs]
        structured = [{"@context": "https://schema.org", "@type": "TechArticle", "headline": title,
                       "description": description, "url": f"{site.SITE_URL}api/{path}", "inLanguage": "en",
                       "about": {"@type": "SoftwareSourceCode", "name": "Aurore"}},
                      {"@context": "https://schema.org", "@type": "BreadcrumbList", "itemListElement": [
                          {"@type": "ListItem", "position": i + 1, "name": n, "item": u} for i, (n, u) in enumerate(trail)]}]
        text = site.page(t, self.lang, to_root, f"api/{path}", f"{title} · Aurore API", body,
                         description=description, structured=structured)
        text = self.dress(text, to_root, path)
        site.write(self.out, f"{path}index.html", text)
        self.count += 1

    def dress(self, text, to_root, path):
        """The site's page made the API's: its stylesheet and script, the
        header's API link marked, the other languages' links to their home."""
        def once(pattern, replacement, s, what):
            new, n = re.subn(pattern, replacement, s, count=1)
            if not n:
                raise SystemExit(f"api: {what} not found in build_site.py's page(): update tools/build_api.py")
            return new
        css = f'<link rel="stylesheet" href="{to_root}assets/site.css">'
        text = once(re.escape(css), css + f'\n<link rel="stylesheet" href="{to_root}assets/api.css">', text, "site.css")
        js = f'<script src="{to_root}assets/site.js" defer></script>'
        text = once(re.escape(js), js + f'\n<script src="{to_root}assets/api.js" defer></script>', text, "site.js")
        text = once(rf'<li><a href="{re.escape(to_root)}api/">', f'<li><a href="{to_root}api/" aria-current="page">',
                    text, "the header's API link")
        # The same page in another language: none, its home instead.
        text = re.sub(r'<link rel="alternate" hreflang="[^"]*" href="[^"]*">\n?', "", text)
        text = re.sub(rf'href="([^"]*?)api/{re.escape(path)}"(?=[^>]*hreflang=)', r'href="\1"', text)
        return text

    def sidebar(self, to_api, path, current_theme):
        def item(url, label, code=True):
            current = ' aria-current="page"' if url == path else ""
            inner = f"<code>{html.escape(label)}</code>" if code else html.escape(label)
            return f'<li><a href="{to_api}{url}"{current}>{inner}</a></li>'
        top = "".join([item("", "Overview", False), item("classes/", "Every class", False),
                       item("macros/", "Macros", False)])
        groups = [f'<div class="doc-group"><h3>{self.t("api_title")}</h3><ul>{top}</ul></div>']
        themed = []
        for key, title, _, names in THEMES:
            links = []
            for name in names:
                url, kind = self.api.lookup(name)
                if not url:
                    continue
                # A class of aurore:: by its name, the rest in full.
                label = name.removeprefix("aurore::") if kind != "namespace" else name
                links.append(item(url, label + ("()" if kind == "function" else "")))
            opened = " open" if key == current_theme else ""
            themed.append(f'<details class="api-theme"{opened}><summary>{html.escape(title)}</summary><ul>{"".join(links)}</ul></details>')
        groups.append(f'<div class="doc-group"><h3>By theme</h3>{"".join(themed)}</div>')
        return '<div class="doc-list">' + "".join(groups) + "</div>"

    # ---- The pages ----

    def home(self):
        r = Render(self.api, "")
        cards = []
        for key, title, sentence, names in THEMES:
            links = []
            for name in names:
                url, kind = self.api.lookup(name)
                if url:
                    # aurore:: is the card's own namespace; the others say theirs
                    label = (name.removeprefix("aurore::") if kind != "namespace" else name) + ("()" if kind == "function" else "")
                    links.append(f'<li><a href="{url}"><code>{html.escape(label)}</code></a></li>')
            cards.append(f'<section class="api-theme-card" id="{key}" aria-labelledby="{key}-title">'
                         f'<h2 id="{key}-title">{html.escape(title)}</h2><p>{sentence}</p>'
                         f'<ul class="api-chips">{"".join(links)}</ul></section>')
        namespaces = [i for i, cd in self.api.compounds.items() if cd.get("kind") == "namespace"]
        base = f"../{site.lang_dir(self.lang)}docs/"
        article = f"""<p class="api-eyebrow">Aurore · C++</p>
      <h1>{self.t("api_title")}</h1>
      <p class="lede">Every class, function and enum of Aurore's public headers (namespace <code>aurore</code> unless named), read from them at each build. To see how they fit together, start with the guides: <a href="{base}getting-started/">Aurore in 15 minutes</a>, <a href="{base}views/">Views</a>, <a href="{base}cadran/">Cadran</a>, <a href="{base}themes/">Themes</a>.</p>
      <!--toc-->
      <div class="api-themes">{"".join(cards)}</div>
      {section("namespaces", "Namespaces", class_list(self.api, r, namespaces))}
      <p class="api-more"><a href="classes/">Every class, by namespace →</a> · <a href="macros/">The macros →</a></p>"""
        toc = [(k, t) for k, t, _, _ in THEMES] + [("namespaces", "Namespaces")]
        self.page("", self.t.plain("api_title"), "The API reference of Aurore, the C++ interface library: "
                  "every class, function and enum of its public headers, by theme.", article, toc)

    def compound(self, refid):
        api, cd = self.api, self.api.compounds[refid]
        name = cd.findtext("compoundname")
        path = api.url[refid]
        r = Render(api, "../" * path.count("/"))
        kind = cd.get("kind")
        parent = "::".join(name.split("::")[:-1])
        eyebrow = kind_of(cd)
        crumbs = []
        parts = name.split("::")
        for i in range(1, len(parts)):
            q = "::".join(parts[:i])
            if q in api.names:
                crumbs.append((q, api.url[api.names[q]]))
        if parent and parent in api.names:
            eyebrow += f' · <a href="{r.to_api}{api.url[api.names[parent]]}"><code>{html.escape(parent)}</code></a>'
        crumbs.append((name, path))
        title_html = (f'<span class="api-scope">{html.escape(parent)}::</span>' if parent else "") + html.escape(short_name(name))
        lede = brief_html(r, cd)
        detail = r.blocks(cd.find("detaileddescription"))
        parts_html, toc = [], []
        if kind == "namespace":
            body, toc = self.namespace_body(r, cd)
        else:
            body, toc, decl = self.class_body(r, cd)
            parts_html.append(decl)
        if detail:
            parts_html.append(section("description", "Description", f'<div class="api-detail">{detail}</div>'))
            toc.insert(0, ("description", "Description"))
        description = first_sentence(cd) or f"{kind_of(cd)} {name} of Aurore's C++ API."
        article = (f'<p class="api-eyebrow">{eyebrow}</p>\n      <h1>{title_html}</h1>\n'
                   + (f'      <p class="lede">{lede}</p>\n' if lede else "") + '      <!--toc-->\n      '
                   + "".join(parts_html) + body)
        self.page(path, name, description, article, toc, self.theme_of.get(path), crumbs)
        self.search.append({"p": f"{kind_of(cd)}" + (f" · {parent}" if parent else ""), "h": name, "u": path,
                            "t": first_sentence(cd)})

    def class_body(self, r, cd):
        api = self.api
        name = cd.findtext("compoundname")
        # The declaration, as the header has it, in an editor's window: the
        # header's tab, its #include, the class and its bases.
        include = cd.findtext("includes") or (cd.find("location").get("file") if cd.find("location") is not None else "")
        location = cd.find("location")
        file = location.get("file") if location is not None else include
        tpl = cd.find("templateparamlist")
        bases = []
        for b in cd.findall("basecompoundref"):
            href = api.link(b.get("refid"), r.to_api) if b.get("refid") else None
            label = html.escape(b.text or "")
            label = f'<a href="{href}">{label}</a>' if href else label
            virt = '<span class="tok-keyword">virtual</span> ' if b.get("virt") == "virtual" else ""
            bases.append(f'<span class="tok-keyword">{b.get("prot")}</span> {virt}{label}')
        decl = ((r.template_line(tpl) + "\n") if tpl is not None else "") + \
            f'<span class="tok-keyword">{cd.get("kind")}</span> <span class="sig-name">{html.escape(name)}</span>' + \
            (f" : {', '.join(bases)}" if bases else "")
        source = f"{site.REPO_URL}/blob/main/library/include/{file}" + (f"#L{location.get('line')}" if location is not None else "")
        window = (f'<figure class="api-decl"><figcaption class="api-decl-bar"><span class="editor-dots" aria-hidden="true"><i></i><i></i><i></i></span>'
                  f'<a class="api-decl-tab" href="{source}">{html.escape(file).replace("/", "/<wbr>")}</a></figcaption>'
                  f'<pre tabindex="0"><code class="language-cpp"><span class="tok-pre">#include &lt;{html.escape(include)}&gt;</span>\n\n{decl}</code></pre></figure>')
        # The line of its bases, up to the first, and the classes that derive from it.
        chain, at = [], cd
        while True:
            first = at.find("basecompoundref")
            if first is None or first.get("refid") not in api.compounds:
                if first is not None:
                    chain.append(f"<span><code>{html.escape(first.text or '')}</code></span>")
                break
            at = api.compounds[first.get("refid")]
            chain.append(f'<a href="{r.to_api}{api.url[first.get("refid")]}"><code>{html.escape(at.findtext("compoundname"))}</code></a>')
        lineage = ""
        if chain:
            items = "".join(f"<li>{c}</li>" for c in reversed(chain)) + f'<li aria-current="page"><code>{html.escape(name)}</code></li>'
            lineage += f'<nav class="api-lineage" aria-label="Inheritance"><p class="api-label">Inherits</p><ol>{items}</ol></nav>'
        derived = [d for d in cd.findall("derivedcompoundref") if d.get("refid") in api.compounds]
        if derived:
            items = "".join(f'<li><a href="{r.to_api}{api.url[d.get("refid")]}"><code>{html.escape(d.text or "")}</code></a></li>'
                            for d in sorted(derived, key=lambda d: (d.text or "").lower()))
            lineage += f'<div class="api-lineage"><p class="api-label">Derived by</p><ul>{items}</ul></div>'
        # The members, by kind.
        groups = {k: [] for k, _ in SECTIONS}
        for m in api.shown_members(cd):
            groups[member_section(m, cd)].append(m)
        inner = [i.get("refid") for i in cd.findall("innerclass") if i.get("refid") in api.compounds]
        body, toc = [], []
        if inner:
            body.append(section("nested", "Nested classes", class_list(api, r, inner)))
            toc.append(("nested", "Nested classes"))
        for key, title in SECTIONS:
            members = groups[key]
            if not members:
                continue
            extra = ""
            if key == "functions":
                extra = self.inherited(r, cd)
            body.append(section(key, title, members_html(r, members, name) + extra, len(members)))
            toc.append((key, title))
        if not groups["functions"] and self.inherited(r, cd):
            body.append(section("functions", "Functions", self.inherited(r, cd)))
            toc.append(("functions", "Functions"))
        return "".join(body), toc, window + lineage

    def inherited(self, r, cd):
        """The functions a class has from its bases, a link to each base's."""
        api, out, at = self.api, [], cd
        while True:
            base = at.find("basecompoundref")
            if base is None or base.get("refid") not in api.compounds:
                break
            at = api.compounds[base.get("refid")]
            count = sum(1 for m in api.shown_members(at) if member_section(m, at) == "functions")
            if count:
                out.append(f'<a href="{r.to_api}{api.url[base.get("refid")]}#functions"><code>{html.escape(at.findtext("compoundname"))}</code></a>'
                           f' <span>({count})</span>')
        if not out:
            return ""
        return f'<p class="api-inherited">Also the functions of {", ".join(out)}.</p>'

    def namespace_body(self, r, cd):
        api = self.api
        name = cd.findtext("compoundname")
        namespaces = [i.get("refid") for i in cd.findall("innernamespace") if i.get("refid") in api.compounds]
        classes = [i.get("refid") for i in cd.findall("innerclass") if i.get("refid") in api.compounds
                   and api.compounds[i.get("refid")].findtext("compoundname").count("::") == name.count("::") + 1]
        groups = {"functions": [], "enums": [], "types": [], "variables": []}
        for m in api.shown_members(cd):
            k = m.get("kind")
            groups["functions" if k == "function" else "enums" if k == "enum" else
                   "types" if k == "typedef" else "variables"].append(m)
        body, toc = [], []
        for key, title in NS_SECTIONS:
            if key == "namespaces" and namespaces:
                content, n = class_list(api, r, namespaces), len(namespaces)
            elif key == "classes" and classes:
                content, n = class_list(api, r, classes), len(classes)
            elif key in groups and groups[key]:
                content, n = members_html(r, groups[key], name), len(groups[key])
            else:
                continue
            body.append(section(key, title, content, n))
            toc.append((key, title))
        return "".join(body), toc

    def every_class(self):
        api = self.api
        r = Render(api, "../")
        by_ns = {}
        for refid, cd in api.compounds.items():
            if cd.get("kind") == "namespace":
                continue
            name = cd.findtext("compoundname")
            scope = "::".join(name.split("::")[:-1])
            while scope and api.names.get(scope) and api.compounds[api.names[scope]].get("kind") != "namespace":
                scope = "::".join(scope.split("::")[:-1])
            by_ns.setdefault(scope, []).append(refid)
        body, toc = [], []
        for scope in sorted(by_ns, key=lambda s: (s != "aurore", s.lower())):
            id_ = "global" if not scope else slug(scope).lower()
            title = scope or "The global namespace"
            body.append(section(id_, f"<code>{html.escape(title)}</code>" if scope else title,
                                class_list(api, r, by_ns[scope]), len(by_ns[scope])))
            toc.append((id_, title))
        count = sum(len(v) for v in by_ns.values())
        article = (f'<p class="api-eyebrow">{self.t("api_title")}</p>\n      <h1>Every class</h1>\n'
                   f'      <p class="lede">The {count} classes and structs of Aurore\'s public headers, by namespace.</p>\n'
                   f'      <!--toc-->\n      ' + "".join(body))
        self.page("classes/", "Every class", "Every class and struct of Aurore's C++ API, by namespace.", article, toc)

    def macros(self):
        r = Render(self.api, "../")
        for m in self.api.macros:
            m.set("anchor", slug(m.findtext("name")))
        article = (f'<p class="api-eyebrow">{self.t("api_title")}</p>\n      <h1>Macros</h1>\n'
                   f'      <p class="lede">The macros an app writes: to bind a view of its XML, to load an activity\'s content, '
                   f'to log, to write a test.</p>\n      <!--toc-->\n      '
                   + section("macros", "Macros", members_html(r, self.api.macros, ""), len(self.api.macros)))
        self.page("macros/", "Macros", "The macros of Aurore's C++ API.", article, [])
        for m in self.api.macros:
            self.search.append({"p": "Macro", "h": m.findtext("name"), "u": f"macros/#{m.get('anchor')}",
                                "t": first_sentence(m)})

    def member_index(self):
        """The members in search.json: Class::member, its kind, its brief."""
        kinds = {"function": "Function", "variable": "Variable", "enum": "Enum", "typedef": "Type",
                 "friend": "Friend"}
        for refid, cd in self.api.compounds.items():
            owner = cd.findtext("compoundname")
            seen = set()
            for m in self.api.shown_members(cd):
                name = m.findtext("name")
                if name in seen:
                    continue
                seen.add(name)
                self.search.append({"p": f"{kinds.get(m.get('kind'), 'Member')} · {owner}", "h": f"{short_name(owner)}::{name}",
                                    "u": self.api.url[m.get("id")], "t": first_sentence(m)})
                for v in m.findall("enumvalue"):
                    self.search.append({"p": f"Value · {owner}::{name}", "h": f"{name}::{v.findtext('name')}",
                                        "u": self.api.url[v.get("id")], "t": ""})

    def redirects(self):
        """Doxygen's old pages (classaurore_1_1View.html…) to the new ones."""
        for refid, url in self.api.url.items():
            if refid in self.api.compounds:
                site.write(self.out, f"{refid}.html",
                           site.redirect_page(url, f"{site.SITE_URL}api/{url}", "en"))
        for old, new in (("annotated.html", "classes/"), ("classes.html", "classes/"), ("hierarchy.html", "classes/"),
                         ("namespaces.html", "./#namespaces"), ("files.html", ""), ("pages.html", "")):
            site.write(self.out, old, site.redirect_page(new or "./", f"{site.SITE_URL}api/{new.removeprefix('./').split('#')[0]}", "en"))


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    folder, out = sys.argv[1], sys.argv[2]
    site.ROOT = sys.argv[3] if len(sys.argv) > 3 else "."
    site.API_DIR = site.API_DIR or out   # the header's link to the API
    site.VERSIONS = site.boreale_versions()
    if not os.path.isfile(os.path.join(folder, "index.xml")):
        raise SystemExit(f"api: no {folder}/index.xml: run doxygen docs/Doxyfile first")
    api = Api(folder)
    builder = Builder(api, out)
    builder.home()
    for refid in sorted(api.compounds, key=lambda i: api.url[i]):
        builder.compound(refid)
    builder.every_class()
    builder.macros()
    builder.member_index()
    builder.redirects()
    site.write(out, "search.json", json.dumps(builder.search, ensure_ascii=False, separators=(",", ":")))
    for p in sorted(set(api.problems)):
        print(p, file=sys.stderr)
    if api.problems:
        return 1
    print(f"api: {builder.count} pages, {len(builder.search)} names in search.json, in {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
