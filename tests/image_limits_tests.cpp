// SPDX-FileCopyrightText: 2026 JigSawFr
// SPDX-License-Identifier: Apache-2.0

// Pictures larger than NVG_MAX_IMAGE_PIXELS are refused from their header,
// on every path that decodes one, before anything large is allocated; a
// font whose glyph runs past its data is refused too (stb_truetype)

#include <aurore.hpp>
#include <aurore/core/theme_image.hpp>
#include <aurore/test/harness.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#ifdef __unix__
#include <sys/resource.h>
#endif

#include "stb_truetype.h"

using aurore::test::Harness;

namespace
{

// A PNG of width x height transparent pixels, its zlib stream made of
// 258-byte runs of zeros: a few megabytes that decode to width x height x 4
std::vector<unsigned char> zeroPng(uint32_t width, uint32_t height)
{
    struct Bits
    {
        std::vector<unsigned char> out;
        uint32_t acc = 0;
        int count    = 0;
        void put(uint32_t value, int bits)
        {
            acc |= value << count;
            count += bits;
            while (count >= 8)
            {
                out.push_back(acc & 0xff);
                acc >>= 8;
                count -= 8;
            }
        }
        // A Huffman code goes most significant bit first
        void code(uint32_t value, int bits)
        {
            uint32_t reversed = 0;
            for (int i = 0; i < bits; i++)
                reversed |= ((value >> i) & 1) << (bits - 1 - i);
            put(reversed, bits);
        }
    };

    uint64_t raw = (uint64_t)height * (1 + (uint64_t)width * 4); // a filter byte a row
    Bits deflate;
    deflate.out = { 0x78, 0x01 };
    deflate.put(1, 1); // the last block
    deflate.put(1, 2); // fixed codes
    deflate.code(0x30, 8); // literal 0
    uint64_t left = raw - 1;
    for (; left >= 258; left -= 258)
    {
        deflate.code(0xc5, 8); // length 258
        deflate.code(0, 5); // distance 1
    }
    for (; left > 0; left--)
        deflate.code(0x30, 8);
    deflate.code(0, 7); // end of block
    if (deflate.count)
        deflate.put(0, 8 - deflate.count);
    uint32_t adler = (uint32_t)(raw % 65521) << 16 | 1;
    for (int i = 3; i >= 0; i--)
        deflate.out.push_back((adler >> (i * 8)) & 0xff);

    auto crc32 = [](const unsigned char* data, size_t size)
    {
        uint32_t crc = 0xffffffff;
        for (size_t i = 0; i < size; i++)
        {
            crc ^= data[i];
            for (int k = 0; k < 8; k++)
                crc = (crc >> 1) ^ (0xedb88320 & (0 - (crc & 1)));
        }
        return ~crc;
    };
    std::vector<unsigned char> png = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
    auto be32                      = [&](uint32_t v)
    {
        for (int i = 3; i >= 0; i--)
            png.push_back((v >> (i * 8)) & 0xff);
    };
    auto chunk = [&](const char* type, const std::vector<unsigned char>& data)
    {
        be32((uint32_t)data.size());
        size_t start = png.size();
        png.insert(png.end(), type, type + 4);
        png.insert(png.end(), data.begin(), data.end());
        be32(crc32(png.data() + start, png.size() - start));
    };
    std::vector<unsigned char> header;
    for (uint32_t v : { width, height })
        for (int i = 3; i >= 0; i--)
            header.push_back((v >> (i * 8)) & 0xff);
    header.insert(header.end(), { 8, 6, 0, 0, 0 }); // 8 bits, RGBA
    chunk("IHDR", header);
    chunk("IDAT", deflate.out);
    chunk("IEND", {});
    return png;
}

// The largest the process has been, in megabytes (0 where unknown)
long peakMegabytes()
{
#ifdef __unix__
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
    return usage.ru_maxrss / (1024 * 1024);
#else
    return usage.ru_maxrss / 1024;
#endif
#else
    return 0;
#endif
}

double millisecondsSince(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

struct TempDir
{
    std::filesystem::path dir;
    explicit TempDir(const char* name)
        : dir(std::filesystem::temp_directory_path() / name)
    {
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
    }
    ~TempDir() { std::filesystem::remove_all(dir); }

    std::string write(const std::string& name, const std::vector<unsigned char>& bytes)
    {
        std::ofstream(dir / name, std::ios::binary).write((const char*)bytes.data(), (std::streamsize)bytes.size());
        return (dir / name).generic_string();
    }

    // A picture of one colour, opaque
    std::string picture(const std::string& name, unsigned char r, unsigned char g, unsigned char b, int width, int height)
    {
        aurore::test::Image image;
        image.width  = width;
        image.height = height;
        image.rgba.reserve((size_t)width * height * 4);
        for (size_t i = 0; i < (size_t)width * height; i++)
            image.rgba.insert(image.rgba.end(), { r, g, b, 255 });
        image.savePng((dir / name).string());
        return (dir / name).generic_string();
    }
};

void settlePictures(Harness& h)
{
    for (int round = 0; round < 3; round++)
    {
        h.step();
        while (aurore::ThemeImages::decoding() > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        h.settle();
    }
}

} // namespace

AURORE_TEST(bomb_is_refused_from_its_header)
{
    // 8192 x 8192: 256 MB of pixels, from 1.7 MB
    std::vector<unsigned char> bomb = zeroPng(8192, 8192);
    TempDir temp("aurore_image_limits_bomb");
    std::string path = temp.write("bomb.png", bomb);
    long before      = peakMegabytes();
    auto start       = std::chrono::steady_clock::now();

    int w = 0, h = 0;
    AURORE_CHECK(nvgDecodeImageMem(bomb.data(), (int)bomb.size(), &w, &h) == nullptr);
    AURORE_CHECK_EQ(w, 8192); // read from the header
    AURORE_CHECK(nvgDecodeImage(path.c_str(), &w, &h) == nullptr);
    NVGcolor mean;
    AURORE_CHECK(!aurore::ThemeImages::meanColor(path, &mean));

    double elapsed = millisecondsSince(start);
    long grown     = peakMegabytes() - before;
    AURORE_CHECK(elapsed < 200);
    AURORE_CHECK(grown < 50);
    aurore::Logger::info("bomb refused in {:.1f} ms, peak memory +{} MB", elapsed, grown);
}

AURORE_TEST(bomb_is_refused_by_the_views_and_theme_pictures)
{
    Harness harness;
    std::vector<unsigned char> bomb = zeroPng(8192, 8192);
    TempDir temp("aurore_image_limits_views");
    std::string path = temp.write("bomb.png", bomb);
    long before      = peakMegabytes();

    auto* image = new aurore::Image();
    harness.push(new aurore::Activity(image));
    image->setImageFromMem(bomb.data(), (int)bomb.size());
    AURORE_CHECK_EQ(image->getTexture(), 0);
    image->setImageFromFile(path);
    AURORE_CHECK_EQ(image->getTexture(), 0);
    image->setImageFromFileAsync(path);
    harness.settle();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    harness.settle();
    AURORE_CHECK_EQ(image->getTexture(), 0);

    std::shared_ptr<aurore::ThemeImage> picture = aurore::ThemeImages::load(path);
    settlePictures(harness);
    AURORE_CHECK(picture->state == aurore::ThemeImage::State::FAILED);
    AURORE_CHECK(peakMegabytes() - before < 50);
}

AURORE_TEST(pictures_up_to_the_limit_still_load)
{
    Harness harness;
    TempDir temp("aurore_image_limits_ok");
    std::string jpeg = std::string(AURORE_SOURCE_RESOURCES) + "img/demo_icon.jpg";
    int w = 0, h = 0;
    unsigned char* pixels = nvgDecodeImage(jpeg.c_str(), &w, &h);
    AURORE_CHECK(pixels != nullptr && w > 0 && h > 0);
    nvgFreeImageData(pixels);

    // A transparent picture of exactly NVG_MAX_IMAGE_PIXELS
    std::vector<unsigned char> largest = zeroPng(NVG_MAX_IMAGE_SIDE, NVG_MAX_IMAGE_PIXELS / NVG_MAX_IMAGE_SIDE);
    pixels                             = nvgDecodeImageMem(largest.data(), (int)largest.size(), &w, &h);
    AURORE_CHECK(pixels != nullptr);
    AURORE_CHECK_EQ(w * h, NVG_MAX_IMAGE_PIXELS);
    nvgFreeImageData(pixels);
    std::vector<unsigned char> wider = zeroPng(NVG_MAX_IMAGE_SIDE + 1, 1);
    AURORE_CHECK(nvgDecodeImageMem(wider.data(), (int)wider.size(), &w, &h) == nullptr);

    auto* image = new aurore::Image();
    harness.push(new aurore::Activity(image));
    image->setImageFromFile(jpeg);
    AURORE_CHECK(image->getTexture() != 0);

    std::string png = temp.picture("red.png", 220, 20, 20, 300, 200);
    NVGcolor mean;
    AURORE_CHECK(aurore::ThemeImages::meanColor(png, &mean));
    AURORE_CHECK(mean.r > 0.8f && mean.g < 0.15f);
}

AURORE_TEST(downscale_keeps_the_colour_over_wide_spans)
{
    // A wide picture down to a few pixels: each output pixel averages
    // thousands of source pixels (sums kept in 64 bits)
    Harness harness;
    TempDir temp("aurore_image_limits_downscale");
    std::string path = temp.picture("wide.png", 255, 255, 255, NVG_MAX_IMAGE_SIDE, 8);
    aurore::ThemeImageOptions options;
    options.maxSide                             = 4;
    std::shared_ptr<aurore::ThemeImage> picture = aurore::ThemeImages::load(path, options);
    settlePictures(harness);
    AURORE_CHECK(picture->ready());
    AURORE_CHECK_EQ(picture->width, 4);
    AURORE_CHECK_EQ(picture->height, 1);
    AURORE_CHECK(picture->mean.r > 0.99f && picture->mean.g > 0.99f && picture->mean.b > 0.99f);
}

namespace
{

uint32_t readBe(const std::vector<unsigned char>& data, size_t at, int bytes)
{
    uint32_t value = 0;
    for (int i = 0; i < bytes; i++)
        value = value << 8 | data[at + i];
    return value;
}

void writeBe(std::vector<unsigned char>& data, size_t at, int bytes, uint32_t value)
{
    for (int i = bytes - 1; i >= 0; i--, value >>= 8)
        data[at + i] = value & 0xff;
}

// Where the table `tag` starts, its length in *length
size_t tableOf(const std::vector<unsigned char>& font, const char* tag, uint32_t* length = nullptr)
{
    int count = (int)readBe(font, 4, 2);
    for (int i = 0; i < count; i++)
    {
        size_t record = 12 + 16 * i;
        if (std::memcmp(font.data() + record, tag, 4) == 0)
        {
            if (length)
                *length = readBe(font, record + 12, 4);
            return readBe(font, record + 8, 4);
        }
    }
    return 0;
}

std::vector<unsigned char> readFile(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

// Draws "A" in two fonts, side by side, white on black
class TwoGlyphs : public aurore::View
{
  public:
    TwoGlyphs(int left, int right)
        : left(left)
        , right(right)
    {
        setWidth(400);
        setHeight(200);
    }

    void draw(NVGcontext* vg, float x, float y, float width, float height, aurore::Style style, aurore::FrameContext* ctx) override
    {
        nvgBeginPath(vg);
        nvgRect(vg, x, y, width, height);
        nvgFillColor(vg, nvgRGB(0, 0, 0));
        nvgFill(vg);
        nvgFontSize(vg, 150);
        nvgFillColor(vg, nvgRGB(255, 255, 255));
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFontFaceId(vg, left);
        nvgText(vg, x + 20, y + 20, "A", nullptr);
        nvgFontFaceId(vg, right);
        nvgText(vg, x + 220, y + 20, "A", nullptr);
    }

  private:
    int left, right;
};

AURORE_TEST(glyph_running_past_its_data_is_refused)
{
    // Kept after the harness: fontstash holds on to the fonts it loaded
    std::vector<unsigned char> good = readFile(std::string(AURORE_SOURCE_RESOURCES) + "font/sora_bold_latin.ttf");
    std::vector<unsigned char> bad  = good;
    AURORE_CHECK(good.size() > 1000);
    stbtt_fontinfo info;
    AURORE_CHECK(stbtt_InitFont(&info, bad.data(), 0));
    int glyph = stbtt_FindGlyphIndex(&info, 'A');
    AURORE_CHECK(glyph > 0);

    // 'A' cut after its header, which still says one contour of 4 points:
    // the points that follow, past its data, would make a square over its box
    bool shortLoca = readBe(bad, tableOf(bad, "head") + 50, 2) == 0;
    size_t loca    = tableOf(bad, "loca");
    size_t glyf    = tableOf(bad, "glyf");
    size_t entry   = loca + glyph * (shortLoca ? 2 : 4);
    size_t start   = glyf + readBe(bad, entry, shortLoca ? 2 : 4) * (shortLoca ? 2 : 1);
    int16_t x0 = (int16_t)readBe(bad, start + 2, 2), y0 = (int16_t)readBe(bad, start + 4, 2);
    int16_t x1 = (int16_t)readBe(bad, start + 6, 2), y1 = (int16_t)readBe(bad, start + 8, 2);
    writeBe(bad, start, 2, 1); // one contour
    writeBe(bad, start + 10, 2, 3); // its last point
    writeBe(bad, start + 12, 2, 0); // no instruction
    size_t end = start + 14 - glyf;
    writeBe(bad, entry + (shortLoca ? 2 : 4), shortLoca ? 2 : 4, (uint32_t)(shortLoca ? end / 2 : end));
    size_t past = start + 14;
    for (int i = 0; i < 4; i++)
        bad[past++] = 1; // on the curve, 16-bit coordinates
    for (int dx : { (int)x0, x1 - x0, 0, x0 - x1 })
        writeBe(bad, (past += 2) - 2, 2, (uint16_t)dx);
    for (int dy : { (int)y0, 0, y1 - y0, 0 })
        writeBe(bad, (past += 2) - 2, 2, (uint16_t)dy);

    Harness harness;
    NVGcontext* vg = aurore::Application::getNVGContext();
    int goodFont   = nvgCreateFontMem(vg, "limits-glyph-good", good.data(), (int)good.size(), 0);
    int badFont    = nvgCreateFontMem(vg, "limits-glyph-bad", bad.data(), (int)bad.size(), 0);
    AURORE_CHECK(goodFont >= 0 && badFont >= 0);
    harness.push(new aurore::Activity(new TwoGlyphs(badFont, goodFont)));
    harness.settle();

    // Lit pixels under each "A": the cut glyph draws nothing
    aurore::test::Image shot = harness.screenshot();
    int lit[2]               = { 0, 0 };
    for (int y = 0; y < std::min(shot.height, 200); y++)
        for (int x = 0; x < std::min(shot.width, 400); x++)
            if (shot.rgba[((size_t)y * shot.width + x) * 4] > 128)
                lit[x < 200 ? 0 : 1]++;
    AURORE_CHECK(lit[1] > 500);
    AURORE_CHECK_EQ(lit[0], 0);
}

AURORE_TEST(font_tables_past_the_data_are_refused)
{
    // Kept after the harness: fontstash holds on to the fonts it loaded
    std::vector<unsigned char> font = readFile(std::string(AURORE_SOURCE_RESOURCES) + "font/sora_bold_latin.ttf");
    std::vector<unsigned char> good = font, longer = font, past = font;
    Harness harness;
    NVGcontext* vg = aurore::Application::getNVGContext();
    AURORE_CHECK(nvgCreateFontMem(vg, "limits-good", good.data(), (int)good.size(), 0) >= 0);

    // The glyf table said longer than the file
    int count = (int)readBe(longer, 4, 2);
    for (int i = 0; i < count; i++)
        if (std::memcmp(longer.data() + 12 + 16 * i, "glyf", 4) == 0)
            writeBe(longer, 12 + 16 * i + 12, 4, (uint32_t)longer.size());
    AURORE_CHECK(nvgCreateFontMem(vg, "limits-longer", longer.data(), (int)longer.size(), 0) < 0);

    // A glyph (loca) past the end of the glyf table
    uint32_t glyfLength = 0;
    tableOf(past, "glyf", &glyfLength);
    bool shortLoca = readBe(past, tableOf(past, "head") + 50, 2) == 0;
    size_t loca    = tableOf(past, "loca");
    if (shortLoca)
        writeBe(past, loca + 2, 2, (glyfLength + 64) / 2);
    else
        writeBe(past, loca + 4, 4, glyfLength + 64);
    AURORE_CHECK(nvgCreateFontMem(vg, "limits-past", past.data(), (int)past.size(), 0) < 0);

    // Too short to hold a header
    AURORE_CHECK(nvgCreateFontMem(vg, "limits-short", font.data(), 8, 0) < 0);
}

int main(int argc, char** argv)
{
    return aurore::test::runTests(argc, argv);
}
