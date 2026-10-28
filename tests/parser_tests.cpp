/*
    Copyright 2026 JigSawFr

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

// The parsers of text from outside: Cadran's safe_text and Markdown (what
// text from GitHub keeps: valid UTF-8, no control, bidi or zero-width
// character, notes cut after a whole line; GitHub-flavoured Markdown as
// blocks, with its bounds and in linear time), and XML layouts (a string
// read to its size, nesting bounded, numbers and durations in range, a
// custom resource in error falling back to the built-in one).

#include <borealis.hpp>
#include <borealis/cadran/util/markdown.hpp>
#include <borealis/cadran/util/safe_text.hpp>
#include <borealis/core/hot_reload.hpp>
#include <borealis/test/harness.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

using brls::test::Harness;
using markdown::Block;

namespace
{

double millisecondsToParse(const std::string& md, size_t* blocks = nullptr)
{
    auto start   = std::chrono::steady_clock::now();
    auto parsed  = markdown::parse(md);
    double spent = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    if (blocks)
        *blocks = parsed.size();
    return spent;
}

bool validUtf8(const std::string& s)
{
    return safe_text::clean(s, s.size(), true) == s;
}

void writeFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

// The message of the error creating the view from `xml` throws ("" when
// none)
std::string xmlError(const std::string& xml)
{
    try
    {
        delete brls::View::createFromXMLString(xml);
        return "";
    }
    catch (const std::exception& e)
    {
        return std::string(typeid(e).name()) + ": " + e.what();
    }
}

} // namespace

BRLS_TEST(markdown_safe_text)
{
    using safe_text::clean;
    BRLS_CHECK(clean("plain", 100, false) == "plain");
    BRLS_CHECK(clean("caf\xC3\xA9 \xE2\x9C\x93", 100, false) == "caf\xC3\xA9 \xE2\x9C\x93"); // café ✓ kept
    // Invalid UTF-8: a stray continuation byte, an overlong '/', a surrogate, past U+10FFFF, cut short.
    BRLS_CHECK(clean("a\x80"
                     "b",
                   100, false)
        == "ab");
    BRLS_CHECK(clean("a\xC0\xAF"
                     "b",
                   100, false)
        == "ab");
    BRLS_CHECK(clean("a\xED\xA0\x80"
                     "b",
                   100, false)
        == "ab");
    BRLS_CHECK(clean("a\xF4\x90\x80\x80"
                     "b",
                   100, false)
        == "ab");
    BRLS_CHECK(clean("a\xE2\x82", 100, false) == "a");
    // Controls: gone; tab: a space; CRLF: LF; LF: a space unless multiline.
    BRLS_CHECK(clean(std::string("a\0b\x1B[31mc\x7F", 9), 100, false) == "ab[31mc");
    BRLS_CHECK(clean("a\xC2\x85"
                     "b",
                   100, false)
        == "ab"); // NEL (C1)
    BRLS_CHECK(clean("a\tb\r\nc", 100, true) == "a b\nc");
    BRLS_CHECK(clean("a\nb", 100, false) == "a b");
    BRLS_CHECK(clean("a\xE2\x80\xA8"
                     "b",
                   100, true)
        == "a\nb");
    // Bidi overrides and isolates, marks, zero-width characters, BOM: gone
    // (a right-to-left override no longer makes "evil" + "txt.exe" read as "evilexe.txt").
    BRLS_CHECK(clean("evil\xE2\x80\xAEtxt.exe", 100, false) == "eviltxt.exe");
    BRLS_CHECK(clean("\xE2\x81\xA6x\xE2\x81\xA9\xE2\x80\x8F\xD8\x9C\xE2\x80\x8B\xE2\x80\x8C\xE2\x81\xA0\xEF\xBB\xBFy", 100, false) == "xy");
    // Cut between two characters, never inside one.
    BRLS_CHECK(clean("ab\xC3\xA9", 3, false) == "ab");
    BRLS_CHECK(clean("ab\xC3\xA9", 4, false) == "ab\xC3\xA9");

    // Notes: cleaned on several lines, cut at the end of a whole line.
    using safe_text::clean_notes;
    BRLS_CHECK(clean_notes("one\r\ntwo\nthree", 100) == "one\ntwo\nthree");
    BRLS_CHECK(clean_notes("one\ntwo\nthree", 13) == "one\ntwo\nthree" && clean_notes("one\ntwo\nthree", 12) == "one\ntwo");
    BRLS_CHECK(clean_notes("one\ntwo\nthree", 7) == "one\ntwo" && clean_notes("one\ntwo\nthree", 6) == "one");
    BRLS_CHECK(clean_notes("a long first line\nx", 5).empty() && clean_notes("", 0).empty());
    BRLS_CHECK(clean_notes("a\x80\nb", 3) == "a\nb"); // measured once cleaned

    BRLS_CHECK(safe_text::is_version("1.2.0") && safe_text::is_version("1.3.0-rc.1") && safe_text::is_version("2.0.0+build_5"));
    BRLS_CHECK(!safe_text::is_version("") && !safe_text::is_version("../1.0") && !safe_text::is_version("1.0 beta") && !safe_text::is_version("-1") && !safe_text::is_version("1.0\xE2\x80\xAE") && !safe_text::is_version(std::string(41, '1')));
}

BRLS_TEST(markdown_inline)
{
    using markdown::inline_text;
    BRLS_CHECK(inline_text("**bold** and *it* and _it_ and ~~gone~~") == "bold and it and it and gone");
    // "_" inside a word is no emphasis, "*" is (as on GitHub); alone, both stay.
    BRLS_CHECK(inline_text("snake_case_name, 2*3*4, a * b _ c") == "snake_case_name, 234, a * b _ c");
    BRLS_CHECK(inline_text("see [the docs](https://x/y_(z)) or [ref][1]") == "see the docs or ref");
    BRLS_CHECK(inline_text("![logo](https://x/l.png) shot") == "logo shot");
    BRLS_CHECK(inline_text("run `make **check**` now") == "run make **check** now");
    BRLS_CHECK(inline_text("a <b>bold</b> <img src=x onerror=alert(1)> b") == "a bold b");
    BRLS_CHECK(inline_text("line<br>next") == "line\nnext");
    BRLS_CHECK(inline_text("<https://github.com>") == "https://github.com");
    BRLS_CHECK(inline_text("a &amp; b &lt;c&gt; &#233; &#x2192; &bogus; &") == "a & b <c> \xC3\xA9 \xE2\x86\x92 &bogus; &");
    // A reference cannot bring a control or bidi character back.
    BRLS_CHECK(inline_text("x&#x202E;y&#27;z&#0;") == "xyz&#0;");
    BRLS_CHECK(inline_text("\\*not emphasis\\* \\[x\\]") == "*not emphasis* [x]");
    BRLS_CHECK(inline_text("\xF0\x9F\x9A\x80 Features \xE2\x9C\xA8") == "Features"); // 🚀 … ✨
    BRLS_CHECK(inline_text("[[[[[deep]]]]](u)").find("deep") != std::string::npos);
}

BRLS_TEST(markdown_blocks)
{
    const std::string md       = "# Title\n"
                                 "Setext\n"
                                 "======\n"
                                 "## \xF0\x9F\x9A\x80 Features ##\n"
                                 "First line\n"
                                 "second line\n"
                                 "\n"
                                 "- item **one**\n"
                                 "  continued\n"
                                 "  - nested\n"
                                 "        - deep\n"
                                 "1. first\n"
                                 "12) twelfth\n"
                                 "- [x] done\n"
                                 "- [ ] todo\n"
                                 "* plain\n"
                                 "\n"
                                 "```cpp\n"
                                 "int main() { return 0; }\n"
                                 "  indented\n"
                                 "```\n"
                                 "> quoted\n"
                                 "> more\n"
                                 "\n"
                                 "> [!WARNING]\n"
                                 "> Careful\n"
                                 "\n"
                                 "---\n"
                                 "| a | b \\| c |\n"
                                 "|---|:-:|\n"
                                 "| 1 | 2 |\n"
                                 "| 3 |\n"
                                 "\n"
                                 "<!-- hidden\n"
                                 "still hidden -->\n"
                                 "<details>\n"
                                 "<summary>More</summary>\n"
                                 "inside <!-- gone --> text\n"
                                 "</details>\n";
    const std::vector<Block> b = markdown::parse(md);
    BRLS_CHECK_EQ(b.size(), (size_t)19);
    if (b.size() != 19)
        return;
    BRLS_CHECK(b[0].kind == Block::Heading && b[0].level == 1 && b[0].text == "Title");
    BRLS_CHECK(b[1].kind == Block::Heading && b[1].level == 1 && b[1].text == "Setext");
    BRLS_CHECK(b[2].kind == Block::Heading && b[2].level == 2 && b[2].text == "Features" && b[2].icon == "1f680");
    BRLS_CHECK(b[3].kind == Block::Paragraph && b[3].text == "First line\nsecond line");
    BRLS_CHECK(b[4].kind == Block::Item && b[4].level == 0 && b[4].number == 0 && b[4].text == "item one continued" && b[4].task < 0);
    BRLS_CHECK(b[5].kind == Block::Item && b[5].level == 1 && b[5].text == "nested");
    BRLS_CHECK(b[6].kind == Block::Item && b[6].level == markdown::MAX_DEPTH && b[6].text == "deep");
    BRLS_CHECK(b[7].kind == Block::Item && b[7].number == 1 && b[7].text == "first");
    BRLS_CHECK(b[8].kind == Block::Item && b[8].number == 12 && b[8].text == "twelfth");
    BRLS_CHECK(b[9].task == 1 && b[9].text == "done" && b[10].task == 0 && b[10].text == "todo");
    BRLS_CHECK(b[11].kind == Block::Item && b[11].text == "plain");
    BRLS_CHECK(b[12].kind == Block::Code && b[12].lines.size() == 2 && b[12].lines[1] == "  indented");
    BRLS_CHECK(b[13].kind == Block::Quote && b[13].alert.empty() && b[13].text == "quoted\nmore");
    BRLS_CHECK(b[14].kind == Block::Quote && b[14].alert == "warning" && b[14].text == "Careful");
    BRLS_CHECK(b[15].kind == Block::Rule);
    BRLS_CHECK(b[16].kind == Block::Table && b[16].rows.size() == 3 && b[16].rows[0].size() == 2 && b[16].rows[0][1] == "b | c" && b[16].rows[1][0] == "1" && b[16].rows[2].size() == 1);
    BRLS_CHECK(b[17].kind == Block::Heading && b[17].level == 4 && b[17].text == "More");
    BRLS_CHECK(b[18].kind == Block::Paragraph && b[18].text == "inside text");
}

BRLS_TEST(markdown_bounds)
{
    // Never more than MAX_BLOCKS blocks, MAX_CODE_LINES (and "…") code lines, MAX_COLUMNS columns.
    std::string many;
    for (int i = 0; i < 1000; i++)
        many += "- item\n";
    BRLS_CHECK(markdown::parse(many).size() == markdown::MAX_BLOCKS);
    std::string code = "```\n";
    for (int i = 0; i < 500; i++)
        code += "line\n";
    const auto c = markdown::parse(code); // never closed: to the end
    BRLS_CHECK(c.size() == 1 && c[0].lines.size() == markdown::MAX_CODE_LINES + 1);
    const auto t = markdown::parse("|1|2|3|4|5|6|7|8|\n|-|-|-|-|-|-|-|-|\n");
    BRLS_CHECK(t.size() == 1 && t[0].rows[0].size() == markdown::MAX_COLUMNS);
    // Hostile input: no crash, nothing left that could be read as markup by the view.
    BRLS_CHECK(markdown::parse("").empty() && markdown::parse("\n\n\n").empty());
    BRLS_CHECK(markdown::parse(std::string(100000, '[')).size() == 1);
    BRLS_CHECK(markdown::parse(std::string(100000, '`')).size() <= 1);
    // Runs that used to be scanned to the end once per character.
    BRLS_CHECK(markdown::parse(std::string(100000, '<')).size() == 1);
    BRLS_CHECK(markdown::parse(std::string(100000, '<') + ">").size() == 1);
    BRLS_CHECK(markdown::parse(std::string(100000, '&')).size() == 1);
    std::string scripted;
    for (int i = 0; i < 10000; i++)
        scripted += "<scripted ";
    BRLS_CHECK(markdown::parse(scripted).size() == 1); // not an element: its text stays
    BRLS_CHECK(markdown::inline_text("a << b <br> c &amp; d &toolongname; e") == "a << b\nc & d &toolongname; e");
    BRLS_CHECK(markdown::parse("<!--").empty());
    // Raw HTML elements go with what they hold, whatever their case; a
    // look-alike tag does not swallow the text after it.
    const auto html = markdown::parse("a <SCRIPT type=x>alert(1)</script> b <style>p{}</style>c <scripted> d <script>never closed");
    BRLS_CHECK(html.size() == 1 && html[0].text == "a b c d");
    const auto bidi = markdown::parse("# a\xE2\x80\xAE"
                                      "b");
    BRLS_CHECK(bidi.size() == 1 && bidi[0].text == "ab");
}

// Regression material of the 2026 audit (outils/securite/md_time.cpp): runs
// of one character that took quadratic time. The bounds are generous: they
// are met hundreds of times over, where the quadratic parse missed them.
BRLS_TEST(markdown_runs_parse_in_linear_time)
{
    // 60000 '>': 110 ms when each level copied the line, under 1 ms now
    BRLS_CHECK(millisecondsToParse(std::string(60000, '>')) < 60);
    BRLS_CHECK(millisecondsToParse(std::string(60000, '>') + " quoted") < 60);
    std::string spaced;
    for (int i = 0; i < 20000; i++)
        spaced += "> ";
    size_t blocks = 0;
    BRLS_CHECK(millisecondsToParse(spaced + "deep", &blocks) < 60);
    BRLS_CHECK_EQ(blocks, (size_t)1);

    // Past MAX_SOURCE, the rest is not read: a megabyte of anything is quick
    for (char c : { '>', '[', '<', '`', '*', '&', '|' })
        BRLS_CHECK(millisecondsToParse(std::string(1 << 20, c)) < 250);
}

BRLS_TEST(markdown_nested_quotes_are_flattened)
{
    auto b = markdown::parse("> > > deep\n>> also\n> >\t> [!TIP]\n> tip");
    BRLS_CHECK_EQ(b.size(), (size_t)2);
    if (b.size() != 2)
        return;
    BRLS_CHECK(b[0].kind == Block::Quote && b[0].text == "deep\nalso");
    BRLS_CHECK(b[1].kind == Block::Quote && b[1].alert == "tip" && b[1].text == "tip");
}

BRLS_TEST(markdown_reads_the_first_max_source_bytes)
{
    std::string md = "# Kept\n" + std::string(markdown::MAX_SOURCE, 'a') + "\n# Past the bound\n";
    auto b         = markdown::parse(md);
    BRLS_CHECK(!b.empty() && b[0].text == "Kept");
    for (const Block& block : b)
        BRLS_CHECK(block.text.find("Past") == std::string::npos);
}

// What the fuzzer (outils/securite/fuzz_md.cpp) checks on each input, here
// on the hostile ones known
BRLS_TEST(markdown_hostile_inputs_keep_the_bounds)
{
    std::vector<std::string> inputs = {
        std::string(100000, '['),
        std::string(100000, '(') + "[](",
        "[a](" + std::string(5000, '(') + ")",
        std::string(3000, '-') + " x",
        "```\n" + std::string(100000, '\n'),
        std::string(20000, '|') + "\n" + std::string(20000, '-'),
        "<details>" + std::string(2000, '<') + "<summary>",
        std::string("a\0b\xff\xfe\xc0\x80", 8),
        "&#x110000; &#xD800; &#99999999999; &;",
    };
    std::string nested;
    for (int i = 0; i < 200; i++)
        nested += std::string(i * 2, ' ') + "- item\n";
    inputs.push_back(nested);

    for (const std::string& in : inputs)
    {
        for (const Block& b : markdown::parse(in))
        {
            BRLS_CHECK(validUtf8(b.text) && validUtf8(b.alert) && validUtf8(b.icon));
            BRLS_CHECK(b.icon.size() <= 8);
            if (b.kind == Block::Heading)
                BRLS_CHECK(b.level >= 1 && b.level <= 6);
            if (b.kind == Block::Item)
                BRLS_CHECK(b.level >= 0 && b.level <= markdown::MAX_DEPTH);
            BRLS_CHECK(b.lines.size() <= markdown::MAX_CODE_LINES + 1);
            BRLS_CHECK(b.rows.size() <= markdown::MAX_ROWS + 1);
            for (const auto& row : b.rows)
                BRLS_CHECK(row.size() <= markdown::MAX_COLUMNS);
        }
        BRLS_CHECK(markdown::parse(in).size() <= markdown::MAX_BLOCKS);
        (void)markdown::inline_text(in);
    }
}

BRLS_TEST(xml_string_view_is_read_to_its_size)
{
    Harness harness;
    // The view ends before the text after it, which is no XML: read past
    // its size (as a C string), it made the parse fail
    std::string buffer = "<brls:Box id=\"sized\" width=\"40\"/><<<not xml";
    std::string_view xml(buffer.data(), buffer.find("<<<"));
    brls::View* view = brls::View::createFromXMLString(xml);
    BRLS_CHECK(view != nullptr && view->getId() == "sized");
    delete view;

    struct Inflated : brls::Box
    {
        using brls::Box::inflateFromXMLString;
    };
    Inflated* box     = new Inflated();
    std::string inner = "<brls:Box id=\"inflated\"><brls:Rectangle/></brls:Box>&&&";
    box->inflateFromXMLString(std::string_view(inner.data(), inner.size() - 3));
    BRLS_CHECK_EQ(box->getChildren().size(), (size_t)1);
    delete box;
}

BRLS_TEST(xml_nesting_is_bounded)
{
    Harness harness;
    // Deep but reasonable: read
    std::string deep;
    for (int i = 0; i < 40; i++)
        deep += "<brls:Box>";
    for (int i = 0; i < 40; i++)
        deep += "</brls:Box>";
    BRLS_CHECK_EQ(xmlError(deep), std::string());

    // Past the bound: an error, not a stack overflow
    std::string deeper;
    for (int i = 0; i < 100; i++)
        deeper += "<brls:Box>";
    for (int i = 0; i < 100; i++)
        deeper += "</brls:Box>";
    BRLS_CHECK(xmlError(deeper).find("nested more than") != std::string::npos);
}

BRLS_TEST(xml_file_including_itself_is_an_error)
{
    namespace fs  = std::filesystem;
    fs::path root = fs::temp_directory_path() / ("aurore-xml-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    // Hot reload reads @res/ from this folder: its cell.xml includes itself
    writeFile(root / "xml/cells/cell.xml", "<brls:Box><brls:View xml=\"@res/xml/cells/cell.xml\"/></brls:Box>");

    Harness harness;
    brls::HotReload::setEnabled(true, root.string() + "/");
    std::string error;
    try
    {
        delete brls::View::createFromXMLResource("cells/cell.xml");
    }
    catch (const std::exception& e)
    {
        error = e.what();
    }
    brls::HotReload::setEnabled(false);
    // An error hot reload logs (the screen stays as it was), not a stack
    // overflow
    BRLS_CHECK(error.find("nested more than") != std::string::npos);
    fs::remove_all(root);
}

BRLS_TEST(xml_custom_resource_in_error_falls_back_to_the_built_in_one)
{
    namespace fs  = std::filesystem;
    fs::path root = fs::temp_directory_path() / ("aurore-custom-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    writeFile(root / "xml/cells/cell.xml", "<brls:Box><brls:Unknown/></brls:Box>");
    writeFile(root / "xml/views/captioned_image.xml", "<brls:Box><unclosed></brls:Box>");

    Harness harness;
    std::string before                = brls::View::CUSTOM_RESOURCES_PATH;
    brls::View::CUSTOM_RESOURCES_PATH = root.string() + "/";
    brls::View* view                  = nullptr;
    try
    {
        view = brls::View::createFromXMLResource("cells/cell.xml");
    }
    catch (const std::exception& e)
    {
        brls::Logger::error("{}", e.what());
    }
    brls::View* invalid = nullptr;
    try
    {
        invalid = brls::View::createFromXMLResource("views/captioned_image.xml");
    }
    catch (const std::exception& e)
    {
        brls::Logger::error("{}", e.what());
    }
    brls::View::CUSTOM_RESOURCES_PATH = before;

    // The built-in ones: an image and a label each
    for (brls::View* built : { view, invalid })
    {
        brls::Box* box = dynamic_cast<brls::Box*>(built);
        BRLS_CHECK(box != nullptr);
        if (box)
            BRLS_CHECK_EQ(box->getChildren().size(), (size_t)2);
        delete built;
    }
    fs::remove_all(root);
}

BRLS_TEST(xml_numbers_out_of_range_are_refused)
{
    Harness harness;
    BRLS_CHECK_EQ(xmlError("<brls:Box width=\"40\" height=\"25%\" marginTop=\"2.5px\"/>"), std::string());
    // Not a number, infinite, past a float: an error on the attribute (not
    // NaN in the layout, nor an out_of_range thrown from std::stof)
    for (const char* value : { "nan", "inf", "-inf", "1e99", "nanpx", "1e99px", "nan%", "1e99%" })
    {
        std::string error = xmlError(std::string("<brls:Box width=\"") + value + "\"/>");
        BRLS_CHECK(!error.empty());
        BRLS_CHECK(error.find("out_of_range") == std::string::npos);
        BRLS_CHECK(error.find(value) != std::string::npos);
    }
}

BRLS_TEST(xml_durations_out_of_range_are_refused)
{
    Harness harness;
    brls::Box box;
    BRLS_CHECK(box.setTransition("opacity 250ms"));
    BRLS_CHECK(box.setTransition("opacity 2s cubicOut"));
    BRLS_CHECK(box.setTransition("opacity 60s"));
    for (const char* spec : { "opacity 1e30s", "opacity 3e9ms", "opacity nans", "opacity infms", "opacity -1s", "opacity 61s" })
        BRLS_CHECK(!box.setTransition(spec));
}

int main(int argc, char** argv)
{
    return brls::test::runTests(argc, argv);
}
