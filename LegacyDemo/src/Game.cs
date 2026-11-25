using Godot;
using System;
using System.Linq;
using System.Threading.Tasks;

namespace TwentyThreeSeventeen;

public partial class Game : Node3D
{
    private readonly Investigation _case = new();
    private GamePreferences _preferences = null!;
    private Apartment _apartment = null!;
    private Detective _detective = null!;
    private CaseAudio _audio = null!;
    private Control _ui = null!;
    private Control _hud = null!;
    private Control? _modal;
    private Label _objective = null!;
    private Label _controls = null!;
    private Label _prompt = null!;
    private PanelContainer _promptPanel = null!;
    private Label _counter = null!;
    private CaseItem? _lastTarget;
    private bool _dialogueOpen;
    private bool _quitting;
    private SaveStatus _saveStatus;
    private int _checkpoint;
    private bool _started;
    private bool _saveFailed;
    private Label _saveWarning = null!;
    private WitnessActor? _speaking;
    private Action? _cancelModal;

    public override void _Ready()
    {
        Bind("move_left", Key.A, Key.Left);
        Bind("move_right", Key.D, Key.Right);
        Bind("move_forward", Key.W, Key.Up);
        Bind("move_back", Key.S, Key.Down);
        Bind("interact", Key.E);
        Bind("notebook", Key.J);
        Bind("report", Key.R);
        Bind("pause_menu", Key.Escape);

        _apartment = new Apartment(this);
        _detective = new Detective { Name = "Detective", Position = Apartment.Entrance };
        AddChild(_detective);
        _audio = new CaseAudio(this);
        _detective.Footstep += _audio.PlayStep;
        GetTree().AutoAcceptQuit = false;

        var canvas = new CanvasLayer();
        AddChild(canvas);
        _ui = new Control
        {
            MouseFilter = Control.MouseFilterEnum.Ignore,
            Theme = GameTypography.CreateTheme()
        };
        canvas.AddChild(_ui);
        Fill(_ui);
        BuildHud();

        _preferences = GamePreferences.Load();
        if (OS.GetCmdlineUserArgs().Contains("--reference-high"))
        {
            _preferences.Resolution = 1;
            _preferences.SetPreset(GraphicsPreset.High);
        }

        ApplyPreferences();
        _audio.Start();
        _saveStatus = CaseSave.Load(_case, out _checkpoint);
        Briefing();
    }

    public override void _Process(double delta)
    {
        _apartment.UpdateVisibleFloors(_detective.EyePosition.Y);
        _apartment.AnimateWitnesses((float)delta, _detective.EyePosition);
        _audio.Process((float)delta, _dialogueOpen);
        if (_modal != null)
            return;

        var checkpoint = Apartment.LandingCheckpoint(_detective.GlobalPosition, _checkpoint);
        if (checkpoint != _checkpoint)
        {
            _checkpoint = checkpoint;
            SaveProgress();
        }

        var target = _detective.Target;
        if (target == _lastTarget)
            return;

        _lastTarget = target;
        _prompt.Text = target switch
        {
            null => "",
            { } item when Investigation.IsWitness(item) => "[E] " + Text("TALK") + " · " + ItemTitle(item),
            _ => "[E] " + ItemTitle(target.Value)
        };
        _promptPanel.Visible = target.HasValue;
    }

    public override void _ExitTree()
    {
        _audio?.Stop();
    }

    public override void _Notification(int what)
    {
        if (what == NotificationWMCloseRequest)
            Quit();
    }

    public override void _UnhandledInput(InputEvent input)
    {
        if (_quitting)
            return;

        if (_cancelModal is {} cancel && input.IsActionPressed("pause_menu"))
            cancel();
        else if (!_started || _cancelModal != null)
            return;
        else if (input.IsActionPressed("pause_menu"))
        {
            if (_modal == null)
                PauseMenu();
            else
                Resume();
        }
        else if (input.IsActionPressed("notebook"))
        {
            Notebook();
        }
        else if (input.IsActionPressed("report"))
        {
            Report();
        }
        else if (_modal == null && input.IsActionPressed("interact") && _detective.Target is { } item)
        {
            Inspect(item);
        }
        else
        {
            return;
        }

        GetViewport().SetInputAsHandled();
    }

    private void BuildHud()
    {
        _hud = new Control { MouseFilter = Control.MouseFilterEnum.Ignore };
        _ui.AddChild(_hud);
        Fill(_hud);

        var objectivePanel = HudPanel(14, 9);
        objectivePanel.Position = new Vector2(24, 22);
        _hud.AddChild(objectivePanel);
        var objectiveRows = new VBoxContainer { MouseFilter = Control.MouseFilterEnum.Ignore };
        objectiveRows.AddThemeConstantOverride("separation", 2);
        objectivePanel.AddChild(objectiveRows);

        _objective = Caption("", 22, emphasis: true);
        objectiveRows.AddChild(_objective);
        _counter = Caption("", 19, subtle: true);
        objectiveRows.AddChild(_counter);

        _saveWarning = Caption("", 18, emphasis: true);
        _saveWarning.AddThemeColorOverride("font_color", GameTypography.Accent);
        objectiveRows.AddChild(_saveWarning);

        var controlsPanel = HudPanel(14, 8);
        _hud.AddChild(controlsPanel);
        controlsPanel.SetAnchorsAndOffsetsPreset(Control.LayoutPreset.BottomLeft);
        controlsPanel.OffsetLeft = 24;
        controlsPanel.OffsetBottom = -20;
        controlsPanel.GrowVertical = Control.GrowDirection.Begin;
        _controls = Caption("", 19, subtle: true);
        controlsPanel.AddChild(_controls);

        var reticle = new ColorRect
        {
            Color = new Color("faead0"),
            MouseFilter = Control.MouseFilterEnum.Ignore
        };
        _hud.AddChild(reticle);
        ReticleAnchor(reticle, -2, -2, 2, 2);

        var promptCenter = new CenterContainer { MouseFilter = Control.MouseFilterEnum.Ignore };
        _hud.AddChild(promptCenter);
        ReticleAnchor(promptCenter, -360, 24, 360, 76);
        _promptPanel = HudPanel(14, 7);
        _prompt = Caption("", 22, emphasis: true);
        _promptPanel.AddChild(_prompt);
        promptCenter.AddChild(_promptPanel);
        _promptPanel.Hide();
    }

    private void Briefing()
    {
        var panel = Open("CASE_TITLE");
        Paragraph(panel, "BRIEF");
        Paragraph(panel, "CONTROLS", subtle: true);
        if (_saveStatus == SaveStatus.Invalid)
            Paragraph(panel, "SAVE_INVALID");

        if (_saveStatus == SaveStatus.Ready)
        {
            Button(panel, "CONTINUE", () =>
            {
                _detective.ReturnTo(Apartment.Checkpoint(_checkpoint));
                _started = true;
                Resume();
            });
            Button(panel, "RESTART", () => ConfirmRestart(Briefing));
        }
        else
        {
            Button(panel, "START", () =>
            {
                _started = true;
                SaveProgress();
                Resume();
            });
        }
    }

    private void Inspect(CaseItem item)
    {
        if (_case.Inspect(item))
            SaveProgress();

        if (Investigation.IsWitness(item))
        {
            ShowWitness(item);
            return;
        }

        var panel = Open(ItemKey(item) + "_TITLE");
        EvidenceText(panel, item);
        Paragraph(panel, "ADDED", subtle: true);
        Button(panel, "BACK_TO_CASE", Resume);
    }

    private void ShowWitness(CaseItem item)
    {
        var panel = Open(ItemKey(item) + "_TITLE", dialogue: true);
        _speaking = _apartment.Witness(item);
        _detective.FrameWitness(_speaking.Focus);
        Paragraph(panel, ItemKey(item) + "_SOURCE", subtle: true);
        var clarified = item == CaseItem.Witness && _case.Challenged;
        Paragraph(panel, clarified ? "WITNESS_CLARIFICATION" : ItemKey(item) + "_BODY");
        _speaking.SetPose(clarified ? WitnessPose.Clarification : WitnessPose.Statement);

        if (item == CaseItem.Witness && !clarified)
        {
            if (_case.CanChallenge)
            {
                Button(panel, "CHALLENGE", () =>
                {
                    if (_case.Challenge())
                        SaveProgress();
                    ShowWitness(item);
                });
            }
            else
            {
                Paragraph(panel, "WITNESS_NEEDS_EVIDENCE", subtle: true);
            }
        }

        Button(panel, "BACK_TO_CASE", Resume);
    }

    private void Notebook()
    {
        var panel = Open("NOTEBOOK");
        var notes = ScrollBody(panel, 360);
        var sources = Investigation.SourceOrder.Where(_case.Has).ToArray();
        if (sources.Length == 0)
            Paragraph(notes, "NOTEBOOK_EMPTY", subtle: true);

        var matches = Enum.GetValues<Comparison>().Where(_case.Has).ToArray();
        if (matches.Length > 0)
        {
            notes.AddChild(Caption(Text("DEDUCTIONS"), 24, emphasis: true));
            foreach (var match in matches)
                Paragraph(notes, ComparisonKey(match));
        }

        foreach (var item in sources)
        {
            notes.AddChild(Caption(ItemTitle(item), 24, emphasis: true));
            EvidenceText(notes, item);
            if (item == CaseItem.Witness && _case.Challenged)
                Paragraph(notes, "WITNESS_CLARIFICATION", subtle: true);
        }

        if (sources.Length >= 2)
            Button(panel, "COMPARE", () => CompareSources(sources));
        else
            Paragraph(panel, "COMPARE_NEED_SOURCES", subtle: true);
        Button(panel, "REPORT", Report);
        Button(panel, "BACK_TO_CASE", Resume);
    }

    private void CompareSources(CaseItem[] sources)
    {
        var panel = Open("COMPARE");
        Paragraph(panel, "COMPARE_HINT", subtle: true);
        string[] options = [Text("SELECT"), .. sources.Select(ItemTitle)];
        var first = 0;
        var second = 0;
        var check = new Button { Text = Text("CHECK_COMPARISON"), Disabled = true, CustomMinimumSize = new Vector2(0, 40) };
        var feedback = Caption("", 22);
        feedback.AutowrapMode = TextServer.AutowrapMode.WordSmart;

        Setting(panel, "SOURCE_ONE", options, first, selected =>
        {
            first = selected;
            RefreshSelection();
        });
        Setting(panel, "SOURCE_TWO", options, second, selected =>
        {
            second = selected;
            RefreshSelection();
        });
        panel.AddChild(check);
        panel.AddChild(feedback);
        Button(panel, "NOTEBOOK", Notebook);
        check.Pressed += () =>
        {
            if (_case.Compare(sources[first - 1], sources[second - 1]) is { } match)
            {
                feedback.Text = Text(ComparisonKey(match));
                SaveProgress();
            }
            else
            {
                feedback.Text = Text("COMPARE_NO_MATCH");
            }
        };

        void RefreshSelection()
        {
            feedback.Text = "";
            check.Disabled = first == 0 || second == 0 || first == second;
        }
    }

    private void Report()
    {
        var panel = Open("REPORT");
        var form = ScrollBody(panel, 450);
        Paragraph(form, "REPORT_INTRO", subtle: true);
        BuildTimeline(form);
        var submit = new Button { Text = Text("SUBMIT"), CustomMinimumSize = new Vector2(0, 40) };
        string[][] choices =
        [
            ["ARRIVAL", "23:14", "23:18", "23:22"],
            ["IDENTITY", "IDENTITY_CERTAIN", "IDENTITY_UNCERTAIN", "IDENTITY_NOBODY"],
            ["CONCLUSION", "CONCLUSION_ACCIDENT", "CONCLUSION_HOMICIDE", "CONCLUSION_OPEN"]
        ];

        for (var index = 0; index < choices.Length; index++)
        {
            var answer = index;
            var options = choices[index];
            form.AddChild(Caption(Text(options[0]), 21, emphasis: true));
            Select(form, [Text("SELECT"), .. options.Skip(1).Select(Text)], _case.ReportAnswers[index] + 1, value =>
            {
                _case.SetReportAnswer(answer, value - 1);
                submit.Disabled = _case.ReportAnswers.Contains(-1);
                SaveProgress();
            });
        }

        submit.Disabled = _case.ReportAnswers.Contains(-1);
        submit.Pressed += FinishReport;
        panel.AddChild(submit);
        Button(panel, "NOTEBOOK", Notebook);
    }

    private void BuildTimeline(VBoxContainer form)
    {
        form.AddChild(Caption(Text("TIMELINE"), 24, emphasis: true));
        Paragraph(form, "TIMELINE_HINT", subtle: true);
        var count = _case.VisibleTimeline.Count;
        if (count == 0)
            Paragraph(form, "TIMELINE_EMPTY", subtle: true);
        var cards = new (Label Title, Button Up, Button Down)[count];

        void Refresh()
        {
            var timeline = _case.VisibleTimeline;
            for (var index = 0; index < cards.Length; index++)
            {
                var item = timeline[index];
                var key = item == TimelineEvent.Arrival && _case.Has(Comparison.CameraClock)
                    ? "EVENT_ARRIVAL_CORRECTED" : "EVENT_" + item.ToString().ToUpperInvariant();
                var title = Text(key);
                var (label, up, down) = cards[index];
                label.Text = $"{index + 1}. {title}";
                up.AccessibilityName = $"{Text("MOVE_UP")} — {title}";
                down.AccessibilityName = $"{Text("MOVE_DOWN")} — {title}";
                up.TooltipText = up.AccessibilityName;
                down.TooltipText = down.AccessibilityName;
                up.Disabled = index == 0;
                down.Disabled = index == cards.Length - 1;
            }
        }

        for (var index = 0; index < cards.Length; index++)
        {
            var slot = index;
            var row = new HBoxContainer { Name = $"TimelineCard{index}" };
            row.AddThemeConstantOverride("separation", 8);
            form.AddChild(row);
            var title = Caption("", 21);
            title.AutowrapMode = TextServer.AutowrapMode.WordSmart;
            title.SizeFlagsHorizontal = Control.SizeFlags.ExpandFill;
            title.VerticalAlignment = VerticalAlignment.Center;
            row.AddChild(title);
            var up = new Button { Name = "MoveUp", Text = "↑", CustomMinimumSize = new Vector2(42, 34) };
            var down = new Button { Name = "MoveDown", Text = "↓", CustomMinimumSize = new Vector2(42, 34) };
            row.AddChild(up);
            row.AddChild(down);
            cards[index] = (title, up, down);
            up.Pressed += () => Move(slot, -1);
            down.Pressed += () => Move(slot, 1);
        }
        Refresh();

        void Move(int slot, int direction)
        {
            _case.MoveTimelineCard(slot, direction);
            SaveProgress();
            Refresh();
            var (_, up, down) = cards[slot + direction];
            var preferred = direction < 0 ? up : down;
            if (preferred.Disabled)
                preferred = direction < 0 ? down : up;
            preferred.GrabFocus();
        }
    }

    private void FinishReport()
    {
        var result = _case.Evaluate();
        var key = result switch
        {
            ReportFinding.Supported => "SUPPORTED",
            ReportFinding.Incomplete => "INCOMPLETE",
            _ => "UNSUPPORTED"
        };

        var panel = Open(key + "_TITLE");
        Paragraph(panel, key + "_BODY");
        Button(panel, "REVISE", Report);
        Button(panel, "BACK_TO_CASE", Resume);
        Button(panel, "RESTART", () => ConfirmRestart(FinishReport));
    }

    private void ConfirmRestart(Action cancel)
    {
        var panel = Open("RESTART");
        _cancelModal = cancel;
        Paragraph(panel, "RESTART_CONFIRM");
        var warning = Caption("", 20);
        warning.AutowrapMode = TextServer.AutowrapMode.WordSmart;
        panel.AddChild(warning);
        Button(panel, "RESTART_YES", () =>
        {
            if (!CaseSave.Clear())
            {
                warning.Text = Text("SAVE_FAILED");
                return;
            }

            _case.Reset();
            _checkpoint = 0;
            _saveStatus = SaveStatus.Missing;
            _saveFailed = false;
            _started = false;
            _detective.ReturnTo(Apartment.Entrance);
            Briefing();
        });
        Button(panel, "CANCEL", cancel);
    }

    private void SaveProgress()
    {
        _saveFailed = !CaseSave.Write(_case.Snapshot(_checkpoint));
        _saveWarning.Visible = _saveFailed;
    }

    private void PauseMenu()
    {
        var panel = Open("PAUSE");
        Button(panel, "BACK_TO_CASE", Resume);
        Button(panel, "SETTINGS", Settings);
        Button(panel, "RESTART", () => ConfirmRestart(PauseMenu));
        Button(panel, "QUIT", Quit);
    }

    private async void Quit()
    {
        if (_quitting)
            return;

        _quitting = true;
        if (_started && _saveFailed)
            SaveProgress();
        _detective.ControlsEnabled = false;
        _audio.Stop();
        // Let the audio thread release its final playback buffers before shutting down the server.
        await Task.Delay(100);
        if (IsInsideTree())
            GetTree().Quit();
    }

    private void Settings()
    {
        var panel = Open("SETTINGS");
        var general = ScrollBody(panel, 340);
        Setting(general, "LANGUAGE", GamePreferences.LocaleNames, _preferences.Language, index =>
        {
            _preferences.Language = index;
            SavePreferences();
        });
        var resolutionOrder = Enumerable.Range(0, GamePreferences.Resolutions.Length)
            .OrderBy(index => GamePreferences.Resolutions[index].X).ToArray();
        Setting(general, "RESOLUTION", resolutionOrder.Select(index => GamePreferences.Resolutions[index])
            .Select(size => $"{size.X} × {size.Y}").ToArray(), Array.IndexOf(resolutionOrder, _preferences.Resolution), index =>
            {
                _preferences.Resolution = resolutionOrder[index];
                SavePreferences();
            });
        Setting(general, "QUALITY", Enum.GetNames<GraphicsPreset>(), (int)_preferences.Preset, index =>
        {
            _preferences.SetPreset((GraphicsPreset)index);
            SavePreferences();
        });
        if (_preferences.Preset == GraphicsPreset.Custom)
        {
            Setting(general, "ANTIALIAS", [Text("OFF"), "2×", "4×", "8×"], _preferences.AntiAliasing,
                index => SetCustom(index, _preferences.Details, _preferences.Shadows));
            Setting(general, "DETAILS", [Text("OFF"), Text("ON")], _preferences.Details ? 1 : 0,
                index => SetCustom(_preferences.AntiAliasing, index == 1, _preferences.Shadows));
            Setting(general, "SHADOWS", [Text("OFF"), Text("ON")], _preferences.Shadows ? 1 : 0,
                index => SetCustom(_preferences.AntiAliasing, _preferences.Details, index == 1));
        }

        VolumeSetting(general, "AMBIENT_VOLUME", _preferences.AmbientVolume, value => _preferences.AmbientVolume = value);
        VolumeSetting(general, "MUSIC_VOLUME", _preferences.MusicVolume, value => _preferences.MusicVolume = value);
        VolumeSetting(general, "EFFECTS_VOLUME", _preferences.EffectsVolume, value => _preferences.EffectsVolume = value);
        Button(panel, "BACK", PauseMenu);
    }

    private void VolumeSetting(VBoxContainer rows, string key, int value, Action<int> setValue)
    {
        var caption = Caption($"{Text(key)}  {value}%", 21);
        rows.AddChild(caption);
        var slider = new HSlider
        {
            Name = key,
            AccessibilityName = Text(key),
            MaxValue = 100,
            Step = 1,
            Value = value,
            CustomMinimumSize = new Vector2(0, 30)
        };
        slider.ValueChanged += current =>
        {
            var percentage = (int)current;
            setValue(percentage);
            caption.Text = $"{Text(key)}  {percentage}%";
            _audio.Apply(_preferences);
            _preferences.Save();
        };
        rows.AddChild(slider);
    }

    private void SetCustom(int antialiasing, bool details, bool shadows)
    {
        _preferences.SetCustom(antialiasing, details, shadows);
        SavePreferences();
    }

    private void SavePreferences()
    {
        ApplyPreferences();
        _preferences.Save();
        Settings();
    }

    private void ApplyPreferences()
    {
        TranslationServer.SetLocale(GamePreferences.Locales[_preferences.Language]);
        var window = GetWindow();
        var desired = GamePreferences.Resolutions[_preferences.Resolution];
        window.Scaling3DScale = desired.X / 1280f;
        if (DisplayServer.GetName() != "headless")
        {
            if (window.Size != desired)
            {
                var usable = DisplayServer.ScreenGetUsableRect(window.CurrentScreen);
                window.Size = desired;
                window.Position = usable.Position + (usable.Size - desired) / 2;
            }
        }

        window.Msaa3D = (Viewport.Msaa)_preferences.AntiAliasing;
        _apartment.SetQuality(_preferences.Details, _preferences.Shadows);
        _apartment.LocalizeSigns();
        _audio.Apply(_preferences);
        UpdateHud();
    }

    private void UpdateHud()
    {
        var objective = _case.ReadyToReport ? "OBJECTIVE_REPORT"
            : !_case.Has(CaseItem.Dispatch) || !_case.Has(CaseItem.Camera) ? "OBJECTIVE_START"
            : "OBJECTIVE";
        _saveWarning.Text = Text("SAVE_FAILED_HUD");
        _saveWarning.Visible = _saveFailed;
        _objective.Text = Text(objective);
        _counter.Text = string.Format(Text("EVIDENCE_COUNT"), _case.EvidenceCount, Investigation.EvidenceOrder.Count);
        _controls.Text = Text("CONTROLS_SHORT");
        _lastTarget = null;
        _prompt.Text = "";
        _promptPanel.Hide();
    }

    private VBoxContainer Open(string title, bool dialogue = false)
    {
        CloseModal();
        var overlay = new ColorRect { Color = new Color(0.025f, 0.036f, 0.045f, dialogue ? 0.14f : 0.86f) };
        _ui.AddChild(overlay);
        Fill(overlay);
        _modal = overlay;
        _dialogueOpen = dialogue;

        var center = new CenterContainer();
        overlay.AddChild(center);
        Fill(center);
        if (dialogue)
        {
            center.AnchorTop = 0.52f;
            center.OffsetBottom = -14;
        }
        var frame = new PanelContainer { CustomMinimumSize = new Vector2(752, 0) };
        var surface = GameTypography.Surface("1b242b", horizontal: 28, vertical: 21);
        surface.BorderColor = new Color("53626c");
        surface.SetBorderWidthAll(1);
        frame.AddThemeStyleboxOverride("panel", surface);
        center.AddChild(frame);

        var rows = new VBoxContainer();
        rows.AddThemeConstantOverride("separation", 10);
        frame.AddChild(rows);
        var heading = Caption(Text(title), 35, emphasis: true);
        heading.AddThemeColorOverride("font_color", GameTypography.Accent);
        rows.AddChild(heading);

        _hud.Hide();
        _detective.ControlsEnabled = false;
        Input.MouseMode = Input.MouseModeEnum.Visible;
        return rows;
    }

    private void Resume()
    {
        CloseModal();
        UpdateHud();
        _hud.Show();
        _detective.ControlsEnabled = true;
        Input.MouseMode = Input.MouseModeEnum.Captured;
    }

    private void CloseModal()
    {
        if (_modal == null)
            return;
        _ui.RemoveChild(_modal);
        _modal.QueueFree();
        _modal = null;
        _cancelModal = null;
        _dialogueOpen = false;
        _speaking?.SetPose(WitnessPose.Idle);
        _speaking = null;
    }

    private static void Setting(VBoxContainer rows, string key, string[] options, int selected, Action<int> changed)
    {
        rows.AddChild(Caption(Text(key), 21, emphasis: true));
        Select(rows, options, selected, changed);
    }

    private static void Select(VBoxContainer rows, string[] options, int selected, Action<int> changed)
    {
        var select = new OptionButton { CustomMinimumSize = new Vector2(0, 38) };
        foreach (var option in options)
            select.AddItem(option);
        select.Select(selected);
        select.ItemSelected += index => changed((int)index);
        rows.AddChild(select);
    }

    private static VBoxContainer ScrollBody(VBoxContainer rows, float minHeight)
    {
        var scroll = new ScrollContainer
        {
            CustomMinimumSize = new Vector2(0, minHeight),
            HorizontalScrollMode = ScrollContainer.ScrollMode.Disabled,
            FollowFocus = true
        };
        rows.AddChild(scroll);
        var body = new VBoxContainer { SizeFlagsHorizontal = Control.SizeFlags.ExpandFill };
        body.AddThemeConstantOverride("separation", 10);
        scroll.AddChild(body);
        return body;
    }

    private static void Button(VBoxContainer rows, string key, Action action)
    {
        var button = new Button { Name = key, Text = Text(key), CustomMinimumSize = new Vector2(0, 40) };
        button.Pressed += action;
        rows.AddChild(button);
    }

    private static void Paragraph(VBoxContainer rows, string key, bool subtle = false)
    {
        var label = Caption(Text(key), subtle ? 20 : 23, subtle);
        label.AutowrapMode = TextServer.AutowrapMode.WordSmart;
        rows.AddChild(label);
    }

    private static void EvidenceText(VBoxContainer rows, CaseItem item)
    {
        Paragraph(rows, ItemKey(item) + "_SOURCE", subtle: true);
        Paragraph(rows, ItemKey(item) + "_BODY");
    }

    private static Label Caption(string text, int size, bool subtle = false, bool emphasis = false)
    {
        var label = new Label { Text = text, MouseFilter = Control.MouseFilterEnum.Ignore };
        label.AddThemeFontSizeOverride("font_size", size);
        label.AddThemeColorOverride("font_color", subtle ? GameTypography.Secondary : GameTypography.Text);
        if (emphasis)
            label.AddThemeFontOverride("font", GameTypography.Emphasis);
        return label;
    }

    private static PanelContainer HudPanel(int horizontal, int vertical)
    {
        var panel = new PanelContainer { MouseFilter = Control.MouseFilterEnum.Ignore };
        panel.AddThemeStyleboxOverride("panel", GameTypography.Surface("151d23ee", horizontal, vertical));
        return panel;
    }

    private static void Fill(Control control) => control.SetAnchorsAndOffsetsPreset(Control.LayoutPreset.FullRect);

    private static void ReticleAnchor(Control control, float left, float top, float right, float bottom)
    {
        control.SetAnchorsAndOffsetsPreset(Control.LayoutPreset.Center);
        control.OffsetLeft = left;
        control.OffsetTop = top;
        control.OffsetRight = right;
        control.OffsetBottom = bottom;
    }

    private static void Bind(string action, params Key[] keys)
    {
        InputMap.AddAction(action);
        foreach (var key in keys)
            InputMap.ActionAddEvent(action, new InputEventKey { PhysicalKeycode = key });
    }

    private static string ComparisonKey(Comparison match) => "MATCH_" + match.ToString().ToUpperInvariant();
    private static string ItemTitle(CaseItem item) => Text(ItemKey(item) + "_TITLE");
    private static string ItemKey(CaseItem item) => item.ToString().ToUpperInvariant();
    private static string Text(string key) => (string)TranslationServer.Translate(key);
}
