// SPDX-License-Identifier: MIT
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "Game/Dialog.hpp"

namespace TwentyThreeSeventeen::Game
{
    namespace
    {
        struct Stroke
        {
            int x, y, width, coverage;
        };

        struct Glyph
        {
            Uint32 codepoint;
            int advance;
            std::size_t first, count;
        };

#include "DialogFont.inc"

        struct Rect
        {
            int x, y, width, height;

            [[nodiscard]] auto
            Contains(float mouseX, float mouseY) const noexcept -> bool
            {
                return mouseX >= static_cast<float>(x)
                       && mouseX < static_cast<float>(x + width)
                       && mouseY >= static_cast<float>(y)
                       && mouseY < static_cast<float>(y + height);
            }
        };

        using Color = std::array<float, 3>;
        constexpr Color kText{ 0.92f, 0.94f, 0.95f };
        constexpr Color kPanel{ 0.105f, 0.143f, 0.170f };
        constexpr Color kAccent{ 1.0f, 0.78f, 0.49f };
        constexpr int kLineHeight = 23;
        constexpr int kButtonHeight = 32;
        constexpr int kButtonGap = 5;

        auto
        FindGlyph(Uint32 codepoint,
                  std::span<const Glyph> glyphs = BodyFont::kGlyphs)
            -> const Glyph &
        {
            const auto found = std::ranges::lower_bound(glyphs,
                                                        codepoint,
                                                        {},
                                                        &Glyph::codepoint);
            if (found != glyphs.end() && found->codepoint == codepoint)
                return *found;
            return glyphs[static_cast<std::size_t>('?' - ' ')];
        }

        auto
        Width(std::string_view text, std::span<const Glyph> glyphs) -> int
        {
            auto *position = text.data();
            auto remaining = text.size();
            int width = 0;
            while (remaining != 0)
                width += FindGlyph(SDL_StepUTF8(&position, &remaining), glyphs)
                             .advance;
            return width;
        }

        auto
        Wrap(std::string_view text, int width) -> std::vector<std::string_view>
        {
            std::vector<std::string_view> lines;
            while (!text.empty())
            {
                const char *position = text.data();
                auto remaining = text.size();
                std::size_t lastSpace = std::string_view::npos;
                std::size_t end = 0;
                int length = 0;
                while (remaining != 0)
                {
                    const auto offset = text.size() - remaining;
                    const auto codepoint = SDL_StepUTF8(&position, &remaining);
                    if (codepoint == '\n')
                    {
                        end = offset;
                        break;
                    }

                    length += FindGlyph(codepoint).advance;
                    if (length > width && end != 0)
                    {
                        if (lastSpace != std::string_view::npos)
                            end = lastSpace;
                        break;
                    }
                    end = text.size() - remaining;
                    if (codepoint == ' ')
                        lastSpace = offset;
                }
                lines.push_back(text.substr(0, end));
                text.remove_prefix(end);
                if (!text.empty() && text.front() == '\n')
                    text.remove_prefix(1);
                else
                    while (!text.empty() && text.front() == ' ')
                        text.remove_prefix(1);
            }
            return lines;
        }

        class Canvas final
        {
        public:
            Canvas(int width, int height)
                : width_{ static_cast<float>(width) }
                , height_{ static_cast<float>(height) }
            {}

            auto
            Fill(Rect area, Color color, float depth) -> void
            {
                const auto left
                    = 2.0f * static_cast<float>(area.x) / width_ - 1.0f;
                const auto right
                    = 2.0f * static_cast<float>(area.x + area.width) / width_
                      - 1.0f;
                const auto top
                    = 1.0f - 2.0f * static_cast<float>(area.y) / height_;
                const auto bottom
                    = 1.0f
                      - 2.0f * static_cast<float>(area.y + area.height)
                            / height_;
                for (auto point : std::array<std::array<float, 2>, 6>{ {
                         { left, bottom },
                         { right, bottom },
                         { left, top },
                         { right, bottom },
                         { right, top },
                         { left, top },
                     } })
                    vertices_.push_back({ point[0],
                                          point[1],
                                          depth,
                                          1.0f,
                                          color[0],
                                          color[1],
                                          color[2] });
            }

            auto
            Text(std::string_view text,
                 int x,
                 int baseline,
                 Color color,
                 Color background,
                 int right,
                 bool heading = false,
                 float depth = 0.3f) -> void
            {
                const auto glyphs
                    = heading ? std::span<const Glyph>(HeadingFont::kGlyphs)
                              : std::span<const Glyph>(BodyFont::kGlyphs);
                const auto strokes
                    = heading ? std::span<const Stroke>(HeadingFont::kStrokes)
                              : std::span<const Stroke>(BodyFont::kStrokes);
                const auto *position = text.data();
                auto remaining = text.size();
                while (remaining != 0)
                {
                    const auto &glyph
                        = FindGlyph(SDL_StepUTF8(&position, &remaining),
                                    glyphs);
                    if (x + glyph.advance > right)
                        return;
                    for (const auto &stroke :
                         strokes.subspan(glyph.first, glyph.count))
                    {
                        const auto alpha
                            = static_cast<float>(stroke.coverage) * 0.25f;
                        const Color ink{
                            background[0] + (color[0] - background[0]) * alpha,
                            background[1] + (color[1] - background[1]) * alpha,
                            background[2] + (color[2] - background[2]) * alpha,
                        };
                        Fill({ x + stroke.x,
                               baseline + stroke.y,
                               stroke.width,
                               1 },
                             ink,
                             depth);
                    }
                    x += glyph.advance;
                }
            }

            [[nodiscard]] auto
            Vertices() const noexcept -> std::span<const Rendering::Vertex>
            {
                return vertices_;
            }

        private:
            float width_;
            float height_;
            std::vector<Rendering::Vertex> vertices_;
        };

        struct Layout
        {
            int windowWidth;
            int windowHeight;
            int width;
            int height;
            Rect panel;
            int bodyRows;
            std::vector<std::string_view> lines;
            std::vector<Rect> buttons;
        };

        auto
        Arrange(SDL_Window *window, std::string_view body, int choices)
            -> Layout
        {
            int width = 1, height = 1;
            SDL_GetWindowSize(window, &width, &height);
            width = std::max(1, width);
            height = std::max(1, height);
            Layout layout{ width,
                           height,
                           std::max(520, width),
                           std::max(420, height),
                           {},
                           0,
                           {},
                           {} };
            const int panelWidth
                = std::min(body.empty() ? 560 : 720, layout.width - 32);
            const int panelX = (layout.width - panelWidth) / 2;
            const int buttonArea = choices * (kButtonHeight + kButtonGap);
            const int available
                = std::max(kLineHeight, layout.height - 126 - buttonArea);
            layout.lines = Wrap(body, panelWidth - 54);
            layout.bodyRows = std::min(available / kLineHeight,
                                       static_cast<int>(layout.lines.size()));
            const int panelHeight
                = 92 + layout.bodyRows * kLineHeight + buttonArea;
            layout.panel = { panelX,
                             (layout.height - panelHeight) / 2,
                             panelWidth,
                             panelHeight };
            int y = layout.panel.y + 76 + layout.bodyRows * kLineHeight;
            for (int index = 0; index < choices; ++index)
            {
                layout.buttons.push_back(
                    { panelX + 20, y, panelWidth - 40, kButtonHeight });
                y += kButtonHeight + kButtonGap;
            }
            return layout;
        }

        auto
        Paint(const Layout &layout,
              std::string_view title,
              const char *close,
              std::span<const char *const> options,
              int active,
              int scroll) -> Canvas
        {
            Canvas canvas(layout.width, layout.height);
            canvas.Fill({ 0, 0, layout.width, layout.height },
                        { 0.045f, 0.063f, 0.078f },
                        0.95f);
            canvas.Fill(layout.panel, kPanel, 0.8f);
            canvas.Fill(
                { layout.panel.x, layout.panel.y, layout.panel.width, 3 },
                kAccent,
                0.7f);
            const auto left = layout.panel.x + 24;
            const auto right = layout.panel.x + layout.panel.width - 24;
            const bool fitsHeading
                = Width(title, HeadingFont::kGlyphs) <= right - left;
            canvas.Text(title,
                        left,
                        layout.panel.y + 46,
                        kAccent,
                        kPanel,
                        right,
                        fitsHeading);
            for (int index = 0; index < layout.bodyRows; ++index)
                canvas.Text(
                    layout.lines[static_cast<std::size_t>(scroll + index)],
                    left,
                    layout.panel.y + 75 + kLineHeight * index,
                    kText,
                    kPanel,
                    right - 7);

            const auto maxScroll
                = static_cast<int>(layout.lines.size()) - layout.bodyRows;
            if (maxScroll > 0)
            {
                const auto track = layout.bodyRows * kLineHeight;
                const auto marker
                    = std::max(5,
                               track * layout.bodyRows
                                   / static_cast<int>(layout.lines.size()));
                canvas.Fill({ right, layout.panel.y + 60, 4, track },
                            { 0.21f, 0.27f, 0.31f },
                            0.65f);
                canvas.Fill({ right,
                              layout.panel.y + 60
                                  + (track - marker) * scroll / maxScroll,
                              4,
                              marker },
                            kAccent,
                            0.6f);
            }

            for (std::size_t index = 0; index < layout.buttons.size(); ++index)
            {
                const auto &button = layout.buttons[index];
                const bool selected = index == static_cast<std::size_t>(active);
                const Color surface = selected ? Color{ 0.27f, 0.35f, 0.40f }
                                               : Color{ 0.16f, 0.21f, 0.25f };
                canvas.Fill(button, surface, 0.6f);
                const auto label
                    = index < options.size() ? options[index] : close;
                const auto shortcut = index < options.size()
                                          ? std::to_string(index + 1)
                                          : std::string("Esc");
                const Rect key{ right - 36, button.y + 5, 34, 22 };
                constexpr Color kKey{ 0.12f, 0.16f, 0.19f };
                if (selected)
                    canvas.Fill({ button.x, button.y, 3, button.height },
                                kAccent,
                                0.5f);
                canvas.Fill(key, kKey, 0.5f);
                canvas.Text(label,
                            button.x + 12,
                            button.y + 22,
                            selected ? kAccent : kText,
                            surface,
                            key.x - 8);
                canvas.Text(
                    shortcut,
                    key.x
                        + (key.width - Width(shortcut, BodyFont::kGlyphs)) / 2,
                    button.y + 22,
                    kText,
                    kKey,
                    key.x + key.width);
            }
            return canvas;
        }
    } // namespace

    auto
    AppendCaption(std::string_view caption,
                  int width,
                  int height,
                  std::vector<Rendering::Vertex> &vertices) -> void
    {
        const auto boxWidth
            = std::min(width - 36, Width(caption, BodyFont::kGlyphs) + 34);
        if (boxWidth <= 40 || height < 100)
            return;
        const auto lines = Wrap(caption, boxWidth - 24);
        const auto lineCount = static_cast<int>(lines.size());
        const auto top = height - 34 - (lineCount * kLineHeight + 14);
        const auto left = (width - boxWidth) / 2;
        Canvas canvas(width, height);
        canvas.Fill({ left, top, boxWidth, lineCount * kLineHeight + 14 },
                    kPanel,
                    0.025f);
        for (std::size_t index = 0; index < lines.size(); ++index)
            canvas.Text(lines[index],
                        left + 12,
                        top + 24 + static_cast<int>(index) * kLineHeight,
                        kText,
                        kPanel,
                        left + boxWidth - 10,
                        false,
                        0.01f);
        const auto overlay = canvas.Vertices();
        vertices.insert(vertices.end(), overlay.begin(), overlay.end());
    }

    Dialog::Dialog(Rendering::Renderer &renderer) noexcept
        : renderer_{ renderer }
    {}

    auto
    Dialog::Choose(std::string_view title,
                   std::string_view body,
                   const char *close,
                   std::span<const char *const> options) -> int
    {
        if (wantsQuit_)
            return -1;

        auto *window = renderer_.Window();
        const auto id = SDL_GetWindowID(window);
        const auto choices = static_cast<int>(options.size()) + 1;
        int active = 0;
        int scroll = 0;
        bool redraw = true;
        auto layout = Arrange(window, body, choices);
        const auto select = [&](int index) {
            return index < choices - 1 ? index : -1;
        };
        const auto scrollBy = [&](int count) {
            const auto next = std::clamp(scroll + count,
                                         0,
                                         static_cast<int>(layout.lines.size())
                                             - layout.bodyRows);
            if (next != scroll)
            {
                scroll = next;
                redraw = true;
            }
        };
        while (true)
        {
            if (redraw)
            {
                const auto canvas
                    = Paint(layout, title, close, options, active, scroll);
                if (const auto drawn = renderer_.Draw(canvas.Vertices());
                    !drawn)
                {
                    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                 "%s",
                                 drawn.error().c_str());
                    return -1;
                }
                redraw = false;
            }

            SDL_Event event{};
            // Keep SDL dispatching window protocol events while the player
            // reads.
            if (!SDL_WaitEventTimeout(&event, 50))
                continue;
            if (event.type == SDL_EVENT_QUIT
                || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
                    && event.window.windowID == id))
            {
                wantsQuit_ = true;
                return -1;
            }
            if ((event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
                 || event.type == SDL_EVENT_WINDOW_RESIZED
                 || event.type == SDL_EVENT_WINDOW_EXPOSED
                 || event.type == SDL_EVENT_WINDOW_RESTORED)
                && event.window.windowID == id)
            {
                layout = Arrange(window, body, choices);
                scrollBy(0);
                redraw = true;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.windowID == id)
            {
                const auto key = event.key.scancode;
                if (key == SDL_SCANCODE_ESCAPE)
                    return -1;
                if (!event.key.repeat
                    && (key == SDL_SCANCODE_RETURN
                        || key == SDL_SCANCODE_KP_ENTER))
                    return select(active);
                if (!event.key.repeat && key >= SDL_SCANCODE_1
                    && key <= SDL_SCANCODE_9)
                {
                    const auto index = key - SDL_SCANCODE_1;
                    if (index < choices - 1)
                        return index;
                }
                if (key == SDL_SCANCODE_PAGEUP || key == SDL_SCANCODE_PAGEDOWN)
                    scrollBy(key == SDL_SCANCODE_PAGEUP ? -layout.bodyRows
                                                        : layout.bodyRows);
                if (key == SDL_SCANCODE_UP || key == SDL_SCANCODE_DOWN
                    || key == SDL_SCANCODE_TAB)
                {
                    const auto back
                        = key == SDL_SCANCODE_UP
                          || (key == SDL_SCANCODE_TAB
                              && (event.key.mod & SDL_KMOD_SHIFT) != 0);
                    active = (active + (back ? choices - 1 : 1)) % choices;
                    redraw = true;
                }
            }
            if (event.type == SDL_EVENT_MOUSE_WHEEL
                && event.wheel.windowID == id)
                scrollBy(event.wheel.y > 0.0f   ? -3
                         : event.wheel.y < 0.0f ? 3
                                                : 0);
            if (event.type == SDL_EVENT_MOUSE_MOTION
                || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                const bool isClick = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                if ((isClick ? event.button.windowID : event.motion.windowID)
                    != id)
                    continue;
                const auto mouseX = isClick ? event.button.x : event.motion.x;
                const auto mouseY = isClick ? event.button.y : event.motion.y;
                const auto x = mouseX * static_cast<float>(layout.width)
                               / static_cast<float>(layout.windowWidth);
                const auto y = mouseY * static_cast<float>(layout.height)
                               / static_cast<float>(layout.windowHeight);
                for (std::size_t index = 0; index < layout.buttons.size();
                     ++index)
                {
                    if (!layout.buttons[index].Contains(x, y))
                        continue;
                    const auto hovered = static_cast<int>(index);
                    if (isClick && event.button.button == SDL_BUTTON_LEFT)
                        return select(hovered);
                    if (active != hovered)
                    {
                        active = hovered;
                        redraw = true;
                    }
                    break;
                }
            }
        }
    }

    auto
    Dialog::WantsQuit() const noexcept -> bool
    {
        return wantsQuit_;
    }
} // namespace TwentyThreeSeventeen::Game
