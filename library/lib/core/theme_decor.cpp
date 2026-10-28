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

// A theme's decorations: its backdrops and strips (pictures, through
// ThemeImages) and its particles, drawn with nanovg. The particles are a
// fixed pool whose places are a function of the time (no state moves from
// frame to frame): the same time draws the same picture, whatever the
// frame rate, which keeps a test's screenshot and redraw debugging true.

#include <aurore/core/application.hpp>
#include <aurore/core/theme_decor.hpp>
#include <aurore/core/theme_image.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace aurore
{

namespace
{

    constexpr Time SECOND   = 1000000;
    constexpr Time PERIOD   = SECOND / ThemeDecorations::PARTICLE_FPS;
    constexpr Time IDLE     = SECOND * ThemeDecorations::IDLE_SECONDS;
    constexpr Time FADE_IN  = SECOND * 3 / 2;
    constexpr Time FADE_OUT = SECOND * 3;
    constexpr double TAU    = 6.283185307179586;

    // ---------------------------------------------------------------------------
    // Pictures
    // ---------------------------------------------------------------------------

    // An area's picture: the one drawn, and the next one while it loads
    struct Held
    {
        ThemePicture spec;
        std::shared_ptr<ThemeImage> shown, loading;
    };

    Held held[THEME_AREA_COUNT];

    bool drawable(const ThemeImage* image)
    {
        return image && image->ready() && image->width > 0 && image->height > 0;
    }

    void onLoaded(const ThemeImage* image)
    {
        for (Held& area : held)
            if (area.loading.get() == image)
            {
                area.shown = std::move(area.loading);
                area.loading.reset();
            }
    }

    enum class Anchor
    {
        CENTER,
        TOP,
        BOTTOM,
    };

    // `image` over x, y, width, height as `spec` fits it (a tile `tileScale`
    // times its size), with corners of `radius`
    void drawPicture(NVGcontext* vg, const ThemeImage* image, const ThemePicture& spec, float x, float y, float width,
        float height, float radius, Anchor anchor, float tileScale)
    {
        float w = (float)image->width, h = (float)image->height;
        float scale = 1.0f;
        bool covers = false;
        switch (spec.fit)
        {
            case BackgroundFit::COVER:
                scale  = std::max(width / w, height / h);
                covers = true;
                break;
            case BackgroundFit::CONTAIN:
                scale = std::min(width / w, height / h);
                break;
            case BackgroundFit::CENTER:
                break;
            case BackgroundFit::TILE:
                scale  = tileScale;
                covers = true;
                break;
        }
        w *= scale;
        h *= scale;
        float px = spec.fit == BackgroundFit::TILE ? x : x + (width - w) / 2;
        float py = anchor == Anchor::TOP ? y : anchor == Anchor::BOTTOM ? y + height - h
                                                                        : y + (height - h) / 2;
        if (spec.fit == BackgroundFit::CENTER)
        {
            px = std::round(px);
            py = std::round(py);
        }

        float left = x, top = y, right = x + width, bottom = y + height;
        if (!covers)
        {
            left   = std::max(left, px);
            top    = std::max(top, py);
            right  = std::min(right, px + w);
            bottom = std::min(bottom, py + h);
        }
        if (right <= left || bottom <= top || spec.opacity <= 0.0f)
            return;
        nvgBeginPath(vg);
        if (radius > 0.0f)
            nvgRoundedRect(vg, left, top, right - left, bottom - top, radius);
        else
            nvgRect(vg, left, top, right - left, bottom - top);
        nvgFillPaint(vg, nvgImagePattern(vg, px, py, w, h, 0, image->texture, spec.opacity));
        nvgFill(vg);
    }

    // A strip's height on a screen `width` wide
    float stripHeight(const ThemeImage* image, const ThemePicture& spec, float width)
    {
        if (spec.height > 0.0f)
            return spec.height;
        if (spec.fit == BackgroundFit::TILE || spec.fit == BackgroundFit::CENTER)
            return (float)image->height;
        return width * (float)image->height / (float)image->width;
    }

    // ---------------------------------------------------------------------------
    // Particles
    // ---------------------------------------------------------------------------

    // How a kind moves and how large it is, in layout units and seconds; a
    // negative speed rises
    struct Look
    {
        float share; // of MAX_PARTICLES at a density of 1
        float sizeMin, sizeMax;
        float speedMin, speedMax;
        float sway; // sideways, either way
        float swayMin, swayMax; // its frequency
        float spinMin, spinMax; // turns a second, either way
        float flipMin, flipMax; // the turns a second it shows its edge (0: flat)
        float wind; // a drift sideways, units a second
    };

    const Look LOOKS[] = {
        {}, // NONE
        { 1.00f, 2.0f, 4.2f, 22, 40, 16, 0.20f, 0.50f, 0, 0, 0, 0, 7 }, // SNOW
        { 0.60f, 10.0f, 15.0f, 30, 48, 44, 0.16f, 0.32f, 0.08f, 0.22f, 0.20f, 0.45f, 14 }, // LEAVES
        { 0.80f, 6.0f, 9.0f, 20, 34, 34, 0.18f, 0.40f, 0.08f, 0.25f, 0.30f, 0.70f, 18 }, // PETALS
        { 1.00f, 2.2f, 3.8f, -46, -22, 10, 0.40f, 0.90f, 0, 0, 0, 0, 3 }, // EMBERS
        { 0.90f, 4.4f, 6.4f, 46, 74, 22, 0.40f, 0.90f, 0.25f, 0.60f, 0.50f, 1.10f, 0 }, // CONFETTI
        { 0.60f, 5.0f, 9.0f, -5, -2, 3, 0.10f, 0.20f, 0, 0, 0, 0, 0 }, // SPARKLES
        { 0.55f, 7.0f, 10.5f, -30, -17, 18, 0.18f, 0.34f, 0, 0, 0, 0, 0 }, // HEARTS
        { 0.55f, 5.0f, 11.0f, -34, -17, 11, 0.30f, 0.70f, 0, 0, 0, 0, 0 }, // BUBBLES
    };

    // Three depths: the far ones smaller, slower, fainter
    constexpr int DEPTHS                = 3;
    constexpr float DEPTH_SIZE[DEPTHS]  = { 0.62f, 0.82f, 1.0f };
    constexpr float DEPTH_SPEED[DEPTHS] = { 0.62f, 0.82f, 1.0f };
    constexpr float DEPTH_ALPHA[DEPTHS] = { 0.42f, 0.66f, 0.92f };
    // The front layer, over the screens: fainter still
    constexpr float FRONT_ALPHA = 0.55f;
    constexpr float MARGIN      = 24.0f;

    struct Particle
    {
        uint32_t seed;
        int depth, color;
        float size, speed, offset;
        float sway, swayFrequency, swayPhase;
        float spin, spinPhase, flip, flipPhase;
    };

    struct Pool
    {
        ThemeParticles spec;
        std::vector<NVGcolor> colors; // the spec's, or the palette's
        bool dark = true; // the background is dark
        std::vector<Particle> particles; // by depth, then colour
        Time epoch      = 0; // their time 0
        Time activity   = 0; // the last input, or the theme's change
        Time shownSince = 0; // when they began to show (fade in)
        Time tick       = -1; // the PERIOD they are drawn at now
        Time drawnTick  = -1; // the one drawn last
        bool running    = false;
    } pool;

    uint32_t hash(uint32_t x)
    {
        x ^= x >> 16;
        x *= 0x7feb352dU;
        x ^= x >> 15;
        x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    // 0 to 1, from a seed and what it is for
    float unit(uint32_t seed, uint32_t salt)
    {
        return (float)(hash(seed * 0x9e3779b9U + salt * 0x85ebca6bU + 0x165667b1U) >> 8) / 16777216.0f;
    }

    float between(uint32_t seed, uint32_t salt, float low, float high)
    {
        return low + (high - low) * unit(seed, salt);
    }

    NVGcolor mix(NVGcolor a, NVGcolor b, float t)
    {
        return nvgRGBAf(a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t);
    }

    NVGcolor faded(NVGcolor color, float alpha)
    {
        color.a *= alpha;
        return color;
    }

    float smoothstep(float from, float to, float x)
    {
        float t = std::clamp((x - from) / (to - from), 0.0f, 1.0f);
        return t * t * (3 - 2 * t);
    }

    // The palette's colours for `kind`, in `theme`
    std::vector<NVGcolor> paletteColors(ParticleKind kind, Theme theme, bool* isDark)
    {
        auto key = [&theme](const char* name, NVGcolor fallback)
        { return theme.hasColor(name) ? theme.getColor(name) : fallback; };
        NVGcolor background = key("aurore/background", nvgRGB(45, 45, 45));
        NVGcolor text       = key("aurore/text", nvgRGB(255, 255, 255));
        NVGcolor accent     = key("aurore/accent", text);
        NVGcolor white      = nvgRGB(255, 255, 255);
        bool dark           = 0.2126f * background.r + 0.7152f * background.g + 0.0722f * background.b < 0.45f;
        *isDark             = dark;
        // Light on a dark background; on a light one, the accent's tint
        NVGcolor pale  = dark ? mix(text, white, 0.5f) : mix(accent, background, 0.4f);
        NVGcolor amber = key("cadran/amber", accent), coral = key("cadran/coral", accent);
        NVGcolor pink = key("cadran/pink", accent), blue = key("cadran/blue", accent), violet = key("cadran/violet", accent);
        switch (kind)
        {
            case ParticleKind::SNOW:
            case ParticleKind::BUBBLES:
                return { pale };
            case ParticleKind::LEAVES:
                return { accent, amber, mix(accent, coral, 0.5f) };
            case ParticleKind::PETALS:
                return { accent, dark ? mix(accent, white, 0.35f) : mix(accent, background, 0.3f) };
            case ParticleKind::EMBERS:
                return { amber, accent };
            case ParticleKind::CONFETTI:
                return { accent, pink, blue, amber, violet };
            case ParticleKind::SPARKLES:
                return { accent, dark ? pale : amber };
            case ParticleKind::HEARTS:
                return { accent, pink };
            case ParticleKind::NONE:
                break;
        }
        return { accent };
    }

    void makePool(const ThemeParticles& spec, int count)
    {
        const Look& look = LOOKS[(int)spec.kind];
        pool.particles.clear();
        int colors = (int)pool.colors.size();
        for (int i = 0; i < count; i++)
        {
            Particle p;
            p.seed          = hash((uint32_t)i * 2654435761U + (uint32_t)spec.kind);
            p.depth         = i % DEPTHS;
            p.color         = (int)(unit(p.seed, 1) * (float)colors) % colors;
            p.size          = between(p.seed, 2, look.sizeMin, look.sizeMax) * DEPTH_SIZE[p.depth];
            p.speed         = between(p.seed, 3, look.speedMin, look.speedMax) * DEPTH_SPEED[p.depth];
            p.offset        = unit(p.seed, 4);
            p.sway          = look.sway * between(p.seed, 5, 0.5f, 1.0f) * DEPTH_SPEED[p.depth];
            p.swayFrequency = between(p.seed, 6, look.swayMin, look.swayMax);
            p.swayPhase     = unit(p.seed, 7) * (float)TAU;
            p.spin          = between(p.seed, 8, look.spinMin, look.spinMax) * (unit(p.seed, 9) < 0.5f ? -1.0f : 1.0f);
            p.spinPhase     = unit(p.seed, 10) * (float)TAU;
            p.flip          = between(p.seed, 11, look.flipMin, look.flipMax);
            p.flipPhase     = unit(p.seed, 12) * (float)TAU;
            pool.particles.push_back(p);
        }
        std::stable_sort(pool.particles.begin(), pool.particles.end(), [](const Particle& a, const Particle& b)
            { return a.depth != b.depth ? a.depth < b.depth : a.color < b.color; });
    }

    // Points of a shape in its own units, turned and scaled where it is drawn
    struct Pen
    {
        NVGcontext* vg;
        float x, y, cosine, sine, sx, sy;

        Pen(NVGcontext* vg, float x, float y, float angle, float sx, float sy)
            : vg(vg)
            , x(x)
            , y(y)
            , cosine(std::cos(angle))
            , sine(std::sin(angle))
            , sx(sx)
            , sy(sy)
        {
        }

        float px(float u, float v) const { return x + u * sx * cosine - v * sy * sine; }
        float py(float u, float v) const { return y + u * sx * sine + v * sy * cosine; }
        void move(float u, float v) { nvgMoveTo(vg, px(u, v), py(u, v)); }
        void line(float u, float v) { nvgLineTo(vg, px(u, v), py(u, v)); }
        void curve(float u1, float v1, float u2, float v2, float u, float v)
        {
            nvgBezierTo(vg, px(u1, v1), py(u1, v1), px(u2, v2), py(u2, v2), px(u, v), py(u, v));
        }
    };

    // A leaf: its tip up, its base round, from -1 to 0.9
    void leaf(Pen pen)
    {
        pen.move(0, -1);
        pen.curve(0.55f, -0.75f, 0.95f, 0.15f, 0, 0.85f);
        pen.curve(-0.95f, 0.15f, -0.55f, -0.75f, 0, -1);
        nvgClosePath(pen.vg);
    }

    // Its midrib and stalk
    void vein(Pen pen)
    {
        pen.move(0, -0.75f);
        pen.line(0, 1.25f);
    }

    // A cherry blossom's petal: round, a notch at its tip
    void petal(Pen pen)
    {
        pen.move(0, 1);
        pen.curve(-0.95f, 0.5f, -0.8f, -0.85f, -0.24f, -0.96f);
        pen.line(0, -0.7f);
        pen.line(0.24f, -0.96f);
        pen.curve(0.8f, -0.85f, 0.95f, 0.5f, 0, 1);
        nvgClosePath(pen.vg);
    }

    void slip(Pen pen)
    {
        pen.move(-1, -0.5f);
        pen.line(1, -0.5f);
        pen.line(1, 0.5f);
        pen.line(-1, 0.5f);
        nvgClosePath(pen.vg);
    }

    // A four-pointed star, its arms curved in
    void star(Pen pen)
    {
        pen.move(0, -1);
        pen.curve(0.1f, -0.25f, 0.25f, -0.1f, 1, 0);
        pen.curve(0.25f, 0.1f, 0.1f, 0.25f, 0, 1);
        pen.curve(-0.1f, 0.25f, -0.25f, 0.1f, -1, 0);
        pen.curve(-0.25f, -0.1f, -0.1f, -0.25f, 0, -1);
        nvgClosePath(pen.vg);
    }

    void heart(Pen pen)
    {
        pen.move(0, 0.9f);
        pen.curve(-1.15f, 0.12f, -0.85f, -1.05f, 0, -0.4f);
        pen.curve(0.85f, -1.05f, 1.15f, 0.12f, 0, 0.9f);
        nvgClosePath(pen.vg);
    }

    // Where a particle is at `t` seconds, its size there (0: not drawn), and
    // how it is turned
    struct Place
    {
        float x, y, size, angle, flip;
    };

    Place placeOf(const Particle& p, ParticleKind kind, double t, float width, float height, float speedFactor)
    {
        const Look& look = LOOKS[(int)kind];
        Place place { 0, 0, p.size, 0, 1 };
        double sway = std::sin(TAU * p.swayFrequency * t + p.swayPhase);

        if (kind == ParticleKind::SPARKLES)
        {
            // Each one twinkles once where it is, then somewhere else
            double life  = (2.6 + 2.2 * unit(p.seed, 13)) / speedFactor;
            double local = t + p.offset * life;
            double gen   = std::floor(local / life);
            double phase = (local - gen * life) / life;
            uint32_t g   = (uint32_t)(int64_t)gen;
            float s      = (float)std::sin(3.141592653589793 * phase);
            place.x      = unit(p.seed ^ hash(g), 14) * width;
            place.y      = unit(p.seed ^ hash(g), 15) * height + (float)(phase * life * p.speed);
            place.size   = p.size * s * s;
            place.angle  = (float)(0.3 * sway);
            return place;
        }

        // Down (or up) the screen and round again, somewhere else each time
        float speed  = p.speed * speedFactor;
        double span  = height + 2 * MARGIN;
        double dist  = std::fabs(speed) * t + p.offset * span;
        double gen   = std::floor(dist / span);
        double along = dist - gen * span;
        uint32_t g   = (uint32_t)(int64_t)gen;
        place.y      = (float)(speed > 0 ? along - MARGIN : height + MARGIN - along);
        double x     = unit(p.seed ^ hash(g), 16) * (width + 2 * MARGIN) - MARGIN + p.sway * sway
            + look.wind * speedFactor * along / std::fabs(speed);
        double wide = width + 2 * MARGIN;
        place.x     = (float)(std::fmod(std::fmod(x + MARGIN, wide) + wide, wide) - MARGIN);

        float progress = (float)(along / span);
        if (look.spinMax > 0)
            place.angle = (float)(TAU * p.spin * speedFactor * t) + p.spinPhase;
        if (look.flipMax > 0)
            place.flip = std::max(0.3f, (float)std::fabs(std::cos(TAU * p.flip * speedFactor * t + p.flipPhase)));
        switch (kind)
        {
            case ParticleKind::EMBERS:
                // They die out on the way up, flickering
                place.size *= (1.0f - smoothstep(0.25f, 0.9f, progress))
                    * (0.78f + 0.22f * (float)std::sin(TAU * 1.7 * t + p.spinPhase));
                break;
            case ParticleKind::HEARTS:
                place.size *= 1.0f - smoothstep(0.72f, 1.0f, progress);
                place.angle = (float)(0.28 * sway);
                break;
            case ParticleKind::LEAVES:
                // A leaf rocks as it swings
                place.angle += (float)(0.5 * sway);
                break;
            default:
                break;
        }
        return place;
    }

    void drawGroup(NVGcontext* vg, ParticleKind kind, const Particle* begin, const Particle* end, NVGcolor color,
        double t, float width, float height, float speedFactor)
    {
        NVGcolor white = nvgRGBAf(1, 1, 1, color.a);
        auto each      = [&](auto shape)
        {
            nvgBeginPath(vg);
            for (const Particle* p = begin; p != end; p++)
            {
                Place place = placeOf(*p, kind, t, width, height, speedFactor);
                if (place.size > 0.05f)
                    shape(place);
            }
        };
        auto fill = [&](NVGcolor c, auto shape)
        {
            each(shape);
            nvgFillColor(vg, c);
            nvgFill(vg);
        };
        auto stroke = [&](NVGcolor c, float strokeWidth, auto shape)
        {
            each(shape);
            nvgStrokeColor(vg, c);
            nvgStrokeWidth(vg, strokeWidth);
            nvgStroke(vg);
        };
        // A light fading from `c` at the centre to nothing at `reach` times the
        // size: a gradient each (a fill each), for the few that have one
        auto glow = [&](NVGcolor c, float reach)
        {
            NVGcolor clear = faded(c, 0.0f);
            for (const Particle* p = begin; p != end; p++)
            {
                Place at = placeOf(*p, kind, t, width, height, speedFactor);
                if (at.size <= 0.05f)
                    continue;
                nvgBeginPath(vg);
                nvgCircle(vg, at.x, at.y, at.size * reach);
                nvgFillPaint(vg, nvgRadialGradient(vg, at.x, at.y, at.size * 0.5f, at.size * reach, c, clear));
                nvgFill(vg);
            }
        };
        int depth = begin->depth;
        switch (kind)
        {
            case ParticleKind::SNOW:
                // A soft edge around the near ones (all of them on a light
                // background, where a flake is a tint, not a light)
                if (depth == DEPTHS - 1 || !pool.dark)
                    glow(faded(color, 0.45f), 2.4f);
                fill(color, [&](const Place& at)
                    { nvgCircle(vg, at.x, at.y, at.size); });
                break;
            case ParticleKind::EMBERS:
                glow(faded(color, 0.55f), 4.5f);
                fill(mix(color, white, 0.3f), [&](const Place& at)
                    { nvgCircle(vg, at.x, at.y, at.size); });
                break;
            case ParticleKind::LEAVES:
                fill(color, [&](const Place& at)
                    { leaf(Pen(vg, at.x, at.y, at.angle, at.size * 0.72f * at.flip, at.size)); });
                stroke(mix(color, nvgRGBAf(0, 0, 0, color.a), 0.35f), 0.9f,
                    [&](const Place& at)
                    { vein(Pen(vg, at.x, at.y, at.angle, at.size * at.flip, at.size)); });
                break;
            case ParticleKind::PETALS:
                fill(color, [&](const Place& at)
                    { petal(Pen(vg, at.x, at.y, at.angle, at.size * at.flip, at.size)); });
                break;
            case ParticleKind::CONFETTI:
                fill(color, [&](const Place& at)
                    { slip(Pen(vg, at.x, at.y, at.angle, at.size, at.size * 1.1f * at.flip)); });
                break;
            case ParticleKind::SPARKLES:
                if (depth == DEPTHS - 1)
                    fill(faded(color, 0.14f), [&](const Place& at)
                        { nvgCircle(vg, at.x, at.y, at.size * 1.3f); });
                fill(color, [&](const Place& at)
                    { star(Pen(vg, at.x, at.y, at.angle, at.size, at.size)); });
                break;
            case ParticleKind::HEARTS:
                fill(color, [&](const Place& at)
                    { heart(Pen(vg, at.x, at.y, at.angle, at.size, at.size)); });
                break;
            case ParticleKind::BUBBLES:
                fill(faded(color, 0.1f), [&](const Place& at)
                    { nvgCircle(vg, at.x, at.y, at.size); });
                stroke(color, 1.1f, [&](const Place& at)
                    { nvgCircle(vg, at.x, at.y, at.size); });
                fill(mix(color, white, 0.6f), [&](const Place& at)
                    { nvgCircle(vg, at.x - at.size * 0.38f, at.y - at.size * 0.38f, std::max(0.7f, at.size * 0.2f)); });
                break;
            case ParticleKind::NONE:
                break;
        }
    }

} // namespace

void ThemeDecorations::update(ThemeVariant variant)
{
    static bool listening = false;
    if (!listening)
    {
        ThemeImages::getLoadedEvent()->subscribe(onLoaded);
        listening = true;
    }

    ThemeDecor decor = Theme::getDecor(variant);
    for (int i = 0; i < THEME_AREA_COUNT; i++)
    {
        Held& area               = held[i];
        const ThemePicture& spec = decor.pictures[i];
        area.spec                = spec;
        if (!spec.hasImage())
        {
            area.shown.reset();
            area.loading.reset();
            continue;
        }
        int flags = spec.fit == BackgroundFit::TILE ? NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY : 0;
        if (area.shown && area.shown->path == spec.image && area.shown->flags == flags)
        {
            area.loading.reset();
            continue;
        }
        std::shared_ptr<ThemeImage> image = ThemeImages::load(spec.image, flags);
        if (image->state == ThemeImage::State::LOADING)
            area.loading = std::move(image);
        else
        {
            area.shown = std::move(image);
            area.loading.reset();
        }
    }

    // The particles: the same pool for the same kind and number (another
    // variant of the theme), in that variant's colours
    const ThemeParticles& spec = decor.particles;
    Time now                   = getCPUTimeUsec();
    pool.colors                = paletteColors(spec.kind, Theme::get(variant), &pool.dark);
    if (!spec.colors.empty())
        pool.colors = spec.colors;
    if (pool.colors.empty())
        pool.colors.push_back(nvgRGB(255, 255, 255));
    int count = spec.enabled()
        ? std::max(1, (int)std::lround(MAX_PARTICLES * spec.density * LOOKS[(int)spec.kind].share))
        : 0;
    // (The same pool is made again: their places go on where they were.)
    if (spec.kind != pool.spec.kind || count != (int)pool.particles.size())
        pool.epoch = now;
    makePool(spec, count);
    pool.spec     = spec;
    pool.activity = now;
    Application::requestRedraw();
}

bool ThemeDecorations::step(Time now)
{
    bool wanted = !pool.particles.empty() && !Application::getReduceMotion();
    Time last   = std::max(Application::getLastInputTime(), pool.activity);
    bool run    = wanted && now - last < IDLE + FADE_OUT;
    if (run && !pool.running)
        pool.shownSince = now; // they start, or come back after a pause: they fade in
    if (!run && pool.running)
        Application::requestRedraw(); // a frame without them
    pool.running = run;
    if (!run)
        return false;
    pool.tick = now / PERIOD;
    return pool.tick != pool.drawnTick;
}

bool ThemeDecorations::particlesRunning()
{
    return pool.running;
}

int ThemeDecorations::particleCount()
{
    return (int)pool.particles.size();
}

void ThemeDecorations::drawParticles(NVGcontext* vg, ParticleLayer layer, float width, float height)
{
    if (!pool.running || pool.spec.layer != layer || pool.particles.empty())
        return;
    pool.drawnTick = pool.tick;
    Time at        = pool.tick * PERIOD;
    Time last      = std::max(Application::getLastInputTime(), pool.activity);
    float alpha    = smoothstep(0.0f, 1.0f, (float)(at - pool.shownSince) / (float)FADE_IN)
        * (1.0f - smoothstep(0.0f, 1.0f, (float)(at - last - IDLE) / (float)FADE_OUT));
    if (layer == ParticleLayer::FRONT)
        alpha *= FRONT_ALPHA;
    if (alpha <= 0.0f)
        return;

    double t                  = (double)(at - pool.epoch) / (double)SECOND;
    const Particle* particles = pool.particles.data();
    size_t count              = pool.particles.size();
    for (size_t first = 0; first < count;)
    {
        size_t end = first;
        while (end < count && particles[end].depth == particles[first].depth && particles[end].color == particles[first].color)
            end++;
        NVGcolor color = faded(pool.colors[particles[first].color], DEPTH_ALPHA[particles[first].depth] * alpha);
        drawGroup(vg, pool.spec.kind, particles + first, particles + end, color, t, width, height, pool.spec.speed);
        first = end;
    }
}

void ThemeDecorations::drawBackdrop(ThemeArea area, NVGcontext* vg, float x, float y, float width, float height, float radius)
{
    if (area == ThemeArea::NONE || area == ThemeArea::TOP || area == ThemeArea::BOTTOM)
        return;
    const Held& picture = held[(int)area - 1];
    if (!drawable(picture.shown.get()))
        return;
    drawPicture(vg, picture.shown.get(), picture.spec, x, y, width, height, radius, Anchor::CENTER, 1.0f);
}

void ThemeDecorations::drawStrips(NVGcontext* vg, float x, float y, float width, float height)
{
    float screenWidth = Application::contentWidth, screenHeight = Application::contentHeight;
    for (ThemeArea area : { ThemeArea::TOP, ThemeArea::BOTTOM })
    {
        const Held& strip       = held[(int)area - 1];
        const ThemeImage* image = strip.shown.get();
        if (!drawable(image))
            continue;
        float stripTall = stripHeight(image, strip.spec, screenWidth);
        float top       = area == ThemeArea::TOP ? 0.0f : screenHeight - stripTall;
        if (top >= y + height || top + stripTall <= y)
            continue;
        bool whole = x <= 0 && y <= 0 && x + width >= screenWidth && y + height >= screenHeight;
        if (!whole)
        {
            nvgSave(vg);
            nvgIntersectScissor(vg, x, y, width, height);
        }
        drawPicture(vg, image, strip.spec, 0, top, screenWidth, stripTall, 0.0f,
            area == ThemeArea::TOP ? Anchor::TOP : Anchor::BOTTOM, stripTall / (float)image->height);
        if (!whole)
            nvgRestore(vg);
    }
}

bool ThemeDecorations::hasPicture(ThemeArea area)
{
    return area != ThemeArea::NONE && drawable(held[(int)area - 1].shown.get());
}

void ThemeDecorations::reset()
{
    for (Held& area : held)
    {
        area.shown.reset();
        area.loading.reset();
        area.spec = ThemePicture();
    }
    pool = Pool();
}

} // namespace aurore
