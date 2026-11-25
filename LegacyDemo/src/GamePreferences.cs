using Godot;
using System;

namespace TwentyThreeSeventeen;

internal enum GraphicsPreset
{
    Low,
    Medium,
    High,
    Xhigh,
    Max,
    Custom
}

internal sealed class GamePreferences
{
    private const string FilePath = "user://preferences.cfg";
    private const string Section = "game";
    private const string AudioSection = "audio";

    // Preferences store the index, so append new modes and sort them in the menu.
    public static readonly Vector2I[] Resolutions =
        [new(960, 540), new(1280, 720), new(1920, 1080), new(2560, 1440), new(1366, 768)];

    public static readonly string[] Locales = ["en_US", "de_DE", "fr_FR"];
    public static readonly string[] LocaleNames = ["English", "Deutsch", "Français"];

    public int Resolution { get; set; }
    public int Language { get; set; }
    public GraphicsPreset Preset { get; private set; }
    public int AntiAliasing { get; private set; }
    public bool Details { get; private set; }
    public bool Shadows { get; private set; }
    public int AmbientVolume { get; set; } = 75;
    public int MusicVolume { get; set; } = 60;
    public int EffectsVolume { get; set; } = 80;

    public static GamePreferences Load()
    {
        var configuration = new ConfigFile();
        var preferences = new GamePreferences();
        var adapter = RenderingServer.GetVideoAdapterName();
        var olderAdapter = adapter.Contains("HD Graphics 4000", StringComparison.OrdinalIgnoreCase)
            || adapter.Contains("llvmpipe", StringComparison.OrdinalIgnoreCase);
        var display = DisplayServer.ScreenGetUsableRect().Size;
        var defaultResolution = olderAdapter || display.X < 1280 || display.Y < 720 ? 0 : 1;
        var language = Array.FindIndex(Locales, item => item.StartsWith(OS.GetLocaleLanguage() + "_",
            StringComparison.OrdinalIgnoreCase));

        preferences.Resolution = defaultResolution;
        preferences.Language = Math.Max(0, language);
        preferences.SetPreset(olderAdapter ? GraphicsPreset.Low : GraphicsPreset.Medium);

        if (configuration.Load(FilePath) != Error.Ok)
            return preferences;

        preferences.AmbientVolume = ReadVolume(configuration, "ambient", preferences.AmbientVolume);
        preferences.MusicVolume = ReadVolume(configuration, "music", preferences.MusicVolume);
        preferences.EffectsVolume = ReadVolume(configuration, "effects", preferences.EffectsVolume);
        preferences.Resolution = Math.Clamp(configuration.GetValue(Section, "resolution", defaultResolution).AsInt32(),
            0, Resolutions.Length - 1);
        var savedLanguage = configuration.GetValue(Section, "language", preferences.Language).AsInt32();
        if (savedLanguage >= 0 && savedLanguage < Locales.Length)
            preferences.Language = savedLanguage;
        var preset = (GraphicsPreset)Math.Clamp(configuration.GetValue(Section, "preset", (int)preferences.Preset).AsInt32(),
            0, (int)GraphicsPreset.Custom);
        preferences.SetPreset(preset);
        if (preset == GraphicsPreset.Custom)
        {
            preferences.AntiAliasing = Math.Clamp(configuration.GetValue(Section, "anti_aliasing", 0).AsInt32(), 0, 3);
            preferences.Details = configuration.GetValue(Section, "details", false).AsBool();
            preferences.Shadows = configuration.GetValue(Section, "shadows", false).AsBool();
        }

        return preferences;
    }

    public void SetPreset(GraphicsPreset preset)
    {
        Preset = preset;
        if (preset == GraphicsPreset.Custom)
            return;

        (AntiAliasing, Details, Shadows) = preset switch
        {
            GraphicsPreset.Low => (0, false, false),
            GraphicsPreset.Medium => (0, true, false),
            GraphicsPreset.High => (0, true, true),
            GraphicsPreset.Xhigh => (1, true, true),
            GraphicsPreset.Max => (2, true, true),
            _ => throw new ArgumentOutOfRangeException(nameof(preset))
        };
    }

    public void SetCustom(int antiAliasing, bool details, bool shadows)
    {
        Preset = GraphicsPreset.Custom;
        AntiAliasing = antiAliasing;
        Details = details;
        Shadows = shadows;
    }

    public void Save()
    {
        var configuration = new ConfigFile();
        configuration.SetValue(Section, "resolution", Resolution);
        configuration.SetValue(Section, "language", Language);
        configuration.SetValue(Section, "preset", (int)Preset);
        configuration.SetValue(Section, "anti_aliasing", AntiAliasing);
        configuration.SetValue(Section, "details", Details);
        configuration.SetValue(Section, "shadows", Shadows);
        configuration.SetValue(AudioSection, "ambient", AmbientVolume);
        configuration.SetValue(AudioSection, "music", MusicVolume);
        configuration.SetValue(AudioSection, "effects", EffectsVolume);
        if (configuration.Save(FilePath) != Error.Ok)
            GD.PushWarning("Could not save the game preferences.");
    }

    private static int ReadVolume(ConfigFile configuration, string key, int fallback) =>
        Math.Clamp(configuration.GetValue(AudioSection, key, fallback).AsInt32(), 0, 100);
}
