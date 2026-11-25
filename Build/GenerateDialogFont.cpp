// SPDX-License-Identifier: MIT
// Run with: c++ Build/GenerateDialogFont.cpp $(pkg-config --cflags --libs
// freetype2) -o /tmp/2317-dialog-font /tmp/2317-dialog-font
// LegacyDemo/assets/fonts/canada1500-regular.otf
// LegacyDemo/assets/fonts/canada1500-bold.otf > Sources/Game/DialogFont.inc
#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdio>
#include <vector>

namespace
{
    struct Span
    {
        int x, y, width, coverage;
    };

    struct Glyph
    {
        unsigned codepoint;
        int advance;
        std::size_t offset, count;
    };
} // namespace

auto
Generate(FT_Library library,
         const char *path,
         unsigned height,
         const char *name) -> bool
{
    FT_Face face{};
    if (FT_New_Face(library, path, 0, &face) != 0)
        return false;
    if (FT_Set_Pixel_Sizes(face, 0, height) != 0)
    {
        FT_Done_Face(face);
        return false;
    }
    std::vector<Glyph> glyphs;
    std::vector<Span> spans;
    for (unsigned codepoint = 32; codepoint <= 0x203f; ++codepoint)
    {
        if ((codepoint > 126 && codepoint < 160)
            || (codepoint > 0x17f && codepoint < 0x2000)
            || FT_Get_Char_Index(face, codepoint) == 0
            || FT_Load_Char(face,
                            codepoint,
                            FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)
                   != 0)
            continue;

        auto glyph = face->glyph;
        const auto &bitmap = glyph->bitmap;
        const auto offset = spans.size();
        for (unsigned row = 0; row < bitmap.rows; ++row)
        {
            for (unsigned x = 0; x < bitmap.width;)
            {
                const auto *pixels
                    = bitmap.buffer + row * static_cast<unsigned>(bitmap.pitch);
                constexpr auto coverageFor = [](unsigned char value) {
                    return (static_cast<int>(value) * 4 + 127) / 255;
                };
                const auto coverage = coverageFor(pixels[x]);
                if (coverage == 0)
                {
                    ++x;
                    continue;
                }
                const auto begin = x;
                while (x < bitmap.width && coverageFor(pixels[x]) == coverage)
                    ++x;
                spans.push_back({ glyph->bitmap_left + static_cast<int>(begin),
                                  static_cast<int>(row) - glyph->bitmap_top,
                                  static_cast<int>(x - begin),
                                  coverage });
            }
        }

        glyphs.push_back({ codepoint,
                           static_cast<int>(glyph->advance.x >> 6),
                           offset,
                           spans.size() - offset });
    }
    std::printf("namespace %s {\n", name);
    std::puts("constexpr Stroke kStrokes[]{");
    std::size_t index = 0;
    for (auto stroke : spans)
    {
        if (index % 6 == 0)
            std::printf("    ");
        ++index;
        std::printf("{%d,%d,%d,%d},%s",
                    stroke.x,
                    stroke.y,
                    stroke.width,
                    stroke.coverage,
                    index % 6 == 0 || index == spans.size() ? "\n" : " ");
    }
    std::puts("};\nconstexpr Glyph kGlyphs[]{");
    for (auto glyph : glyphs)
        std::printf("    {%u, %d, %zu, %zu},\n",
                    glyph.codepoint,
                    glyph.advance,
                    glyph.offset,
                    glyph.count);
    std::puts("};\n}");
    FT_Done_Face(face);
    return true;
}

auto
main(int argc, char **argv) -> int
{
    FT_Library library{};
    if (argc != 3 || FT_Init_FreeType(&library) != 0)
        return 1;
    std::puts("// Generated from Canada 1500 (CC0-1.0) by "
              "Build/GenerateDialogFont.cpp.");
    const bool generated = Generate(library, argv[1], 18, "BodyFont")
                           && Generate(library, argv[2], 32, "HeadingFont");
    FT_Done_FreeType(library);
    return generated ? 0 : 1;
}
