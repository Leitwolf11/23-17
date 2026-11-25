using Godot;

namespace TwentyThreeSeventeen;

internal sealed class CaseAudio
{
    private readonly AudioStreamPlayer3D _rain;
    private readonly AudioStreamPlayer3D _hum;
    private readonly AudioStreamPlayer3D _vent;
    private readonly AudioStreamPlayer _score;
    private readonly AudioStreamPlayer _steps;
    private readonly AudioStream[] _stoneSteps =
        [Load("stone-step-1.wav"), Load("stone-step-2.wav"), Load("stone-step-3.wav")];

    private float _musicLevel;
    private float _duck = 1f;
    private int _nextStep;

    public CaseAudio(Node3D owner)
    {
        _rain = Ambience(owner, "EntranceRain", "entrance-rain.wav", new Vector3(0, 1.8f, 5.39f), 7f);
        _hum = Ambience(owner, "LandingHum", "fluorescent-hum.wav", new Vector3(0, 14.66f, 4.15f), 4.9f);
        _vent = Ambience(owner, "ServiceVent", "service-vent.wav", new Vector3(0, -0.55f, 9.3f), 4.8f);
        _score = new AudioStreamPlayer { Name = "CaseScore", Stream = Loop("case-score.wav") };
        _steps = new AudioStreamPlayer { Name = "Footsteps" };
        owner.AddChild(_score);
        owner.AddChild(_steps);
    }

    public void Start()
    {
        _rain.Play();
        _hum.Play();
        _vent.Play();
        _score.Play();
    }

    public void Stop()
    {
        _steps.Stop();
        _score.Stop();
        _rain.Stop();
        _hum.Stop();
        _vent.Stop();
    }

    public void Apply(GamePreferences preferences)
    {
        var ambient = preferences.AmbientVolume / 100f;
        SetLevel(_rain, ambient, -7);
        SetLevel(_hum, ambient, -12);
        SetLevel(_vent, ambient, -1);
        _musicLevel = preferences.MusicVolume / 100f;
        SetLevel(_steps, preferences.EffectsVolume / 100f, -7);
        SetLevel(_score, _musicLevel * _duck, -9);
    }

    public void Process(float delta, bool dialogueOpen)
    {
        var next = Mathf.MoveToward(_duck, dialogueOpen ? 0.25f : 1f, delta);
        if (next == _duck)
            return;

        _duck = next;
        SetLevel(_score, _musicLevel * _duck, -9);
    }

    public void PlayStep()
    {
        _steps.Stream = _stoneSteps[_nextStep];
        _nextStep = (_nextStep + 1) % _stoneSteps.Length;
        _steps.Play();
    }

    private static AudioStreamPlayer3D Ambience(Node3D owner, string name, string file, Vector3 position, float range)
    {
        var player = new AudioStreamPlayer3D
        {
            Name = name,
            Stream = Loop(file),
            Position = position,
            UnitSize = 1.8f,
            MaxDistance = range
        };
        owner.AddChild(player);
        return player;
    }

    private static AudioStream Load(string file) => GD.Load<AudioStream>("res://assets/audio/" + file);

    private static AudioStreamWav Loop(string file)
    {
        var stream = (AudioStreamWav)Load(file);
        stream.LoopMode = AudioStreamWav.LoopModeEnum.Forward;
        stream.LoopEnd = Mathf.RoundToInt(stream.GetLength() * stream.MixRate);
        return stream;
    }

    private static float VolumeDb(float level, float trim) =>
        level > 0 ? Mathf.LinearToDb(level) + trim : -80;

    private static void SetLevel(AudioStreamPlayer player, float level, float trim) =>
        player.VolumeDb = VolumeDb(level, trim);

    private static void SetLevel(AudioStreamPlayer3D player, float level, float trim) =>
        player.VolumeDb = VolumeDb(level, trim);
}
