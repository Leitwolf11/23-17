using Godot;

namespace TwentyThreeSeventeen;

internal static class GameTypography
{
    public static readonly Color Text = new("f6f3ec");
    public static readonly Color Secondary = new("d2dce1");
    public static readonly Color Accent = new("f5c795");

    public static readonly FontFile Emphasis = GD.Load<FontFile>("res://assets/fonts/canada1500-bold.otf");

    private static readonly FontFile Body = GD.Load<FontFile>("res://assets/fonts/canada1500-regular.otf");

    public static Theme CreateTheme()
    {
        var theme = new Theme { DefaultFont = Body, DefaultFontSize = 22 };
        theme.SetConstant("line_spacing", "Label", 5);

        var normal = Surface("29353d", horizontal: 14, vertical: 8);
        var hover = Surface("3a4c58", horizontal: 14, vertical: 8);
        var pressed = Surface("213038", horizontal: 14, vertical: 8);
        var disabled = Surface("202930", horizontal: 14, vertical: 8);
        var focus = Surface("29353d", horizontal: 14, vertical: 8);
        focus.DrawCenter = false;
        focus.BorderColor = Accent;
        focus.SetBorderWidthAll(2);

        foreach (var control in new[] { "Button", "OptionButton" })
        {
            theme.SetFont("font", control, Emphasis);
            theme.SetFontSize("font_size", control, 20);
            theme.SetColor("font_color", control, Text);
            theme.SetColor("font_hover_color", control, Text);
            theme.SetColor("font_pressed_color", control, Text);
            theme.SetColor("font_focus_color", control, Text);
            theme.SetColor("font_disabled_color", control, new Color("a6b3bc"));
            theme.SetStylebox("normal", control, normal);
            theme.SetStylebox("hover", control, hover);
            theme.SetStylebox("pressed", control, pressed);
            theme.SetStylebox("disabled", control, disabled);
            theme.SetStylebox("focus", control, focus);
        }

        theme.SetFont("font", "PopupMenu", Body);
        theme.SetFontSize("font_size", "PopupMenu", 20);
        theme.SetColor("font_color", "PopupMenu", Text);
        theme.SetColor("font_hover_color", "PopupMenu", Text);
        theme.SetStylebox("panel", "PopupMenu", Surface("202a32", horizontal: 10, vertical: 8));
        theme.SetStylebox("hover", "PopupMenu", hover);
        theme.SetConstant("v_separation", "PopupMenu", 12);
        theme.SetFont("font", "TooltipLabel", Body);
        return theme;
    }

    public static StyleBoxFlat Surface(string color, int horizontal, int vertical) => new()
    {
        BgColor = new Color(color),
        CornerRadiusTopLeft = 7,
        CornerRadiusTopRight = 7,
        CornerRadiusBottomLeft = 7,
        CornerRadiusBottomRight = 7,
        ContentMarginLeft = horizontal,
        ContentMarginRight = horizontal,
        ContentMarginTop = vertical,
        ContentMarginBottom = vertical
    };
}
