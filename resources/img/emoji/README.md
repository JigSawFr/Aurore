# Emoji icons

Cadran draws an emoji from here where the console's font has none: the one a
heading or a bullet of a Markdown text opens on (`markdown_view`, the release
notes of an app), and the language picker's flags (`ui::pick_language`). A
file is named after the emoji's code point in lowercase hex, without the
U+FE0F variation selector: `1f680.png` for 🚀, `26a0.png` for ⚠️. An emoji
that is not here shows no icon.

The set is a palette, larger than what Cadran itself uses: an app's notes
can open on any of these without a new file. The flags (two code points,
joined by a dash as Twemoji names them: `1f1eb-1f1f7.png` for 🇫🇷) are the
language picker's: 🇺🇸 🇫🇷 🇨🇦 🇩🇪 🇪🇸 🇲🇽 🇮🇹 🇳🇱 🇵🇹 🇧🇷 🇷🇺 🇯🇵 🇰🇷 🇨🇳 🇹🇼, the
console's own language a 🌐; the others are there for an app's own lists
(countries, continents 🌎 🌏 🌍).

To add one, name it to `tools/make_emoji.py`, which fetches its 72×72 PNG
from the same Twemoji release (run without a name, it fetches every one
again):

    python3 tools/make_emoji.py <hex>

Graphics: [Twemoji](https://github.com/jdecked/twemoji) 17.0.3, © Twitter, Inc
and other contributors, licensed under
[CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/).

| | | | | | |
|---|---|---|---|---|---|
| 🚀 `1f680` | ⚠️ `26a0` | 🐞 `1f41e` | ✨ `2728` | 🧰 `1f9f0` | 🧪 `1f9ea` |
| 📚 `1f4da` | 🌟 `1f31f` | 📊 `1f4ca` | 🔒 `1f512` | 🕒 `1f552` | 📱 `1f4f1` |
| 🌍 `1f30d` | 🛡️ `1f6e1` | ⏱️ `23f1` | ⏳ `23f3` | ⏰ `23f0` | 🎮 `1f3ae` |
| 👪 `1f46a` | 🔄 `1f504` | 🛟 `1f6df` | 🆘 `1f198` | 💾 `1f4be` | 🐛 `1f41b` |
| 🔧 `1f527` | ✅ `2705` | 🎉 `1f389` | ⭐ `2b50` | 🧭 `1f9ed` | 🔔 `1f514` |
| ⚙️ `2699` | 🗓️ `1f5d3` | 📈 `1f4c8` | 🧒 `1f9d2` | 🔑 `1f511` | 📦 `1f4e6` |
| 🌙 `1f319` | 🎨 `1f3a8` | 💡 `1f4a1` | 📝 `1f4dd` | 🚨 `1f6a8` | 🛠️ `1f6e0` |
| 🔐 `1f510` | 🌐 `1f310` | 📤 `1f4e4` | ❤️ `2764` | 🚫 `1f6ab` | ⚡ `26a1` |
| 🧹 `1f9f9` | 🗑️ `1f5d1` | 📋 `1f4cb` | 👀 `1f440` | 🎯 `1f3af` | 🆕 `1f195` |
| 🔍 `1f50d` | | | | | |
