using Godot;

namespace TwentyThreeSeventeen;

internal enum WitnessPose
{
    Idle,
    Statement,
    Clarification
}

internal sealed class WitnessActor
{
    private readonly Node3D _body;
    private readonly Node3D _model;
    private readonly Node3D _head;
    private readonly Node3D _eyes;
    private readonly Node3D _mouth;
    private readonly Node3D _torso;
    private readonly Node3D[] _shoulders = new Node3D[2];
    private readonly Node3D[] _elbows = new Node3D[2];
    private readonly bool _electrician;
    private WitnessPose _pose;
    private float _poseTime;
    private float _blinkTime;

    public Vector3 Focus => _body.GlobalPosition + Vector3.Up * 0.38f;

    public WitnessActor(Node3D body, bool electrician)
    {
        _body = body;
        _electrician = electrician;
        _model = new Node3D { Rotation = new Vector3(0, 0.45f, 0) };
        body.AddChild(_model);
        var coat = Paint(electrician ? "335a72" : "46574b");
        var collar = Paint(electrician ? "233c4b" : "303c36");
        var cloth = Paint("252a2d");
        var leather = Paint("222322");
        var skin = Paint(electrician ? "b87958" : "ae826c");
        var seam = Paint(electrician ? "223a48" : "333d34");
        var hair = Paint(electrician ? "36251f" : "383330");
        var shirt = Paint(electrician ? "d8cfad" : "c0b29a");
        var ball = new SphereMesh { Radius = 1, Height = 2, RadialSegments = 24, Rings = 12 };
        var leg = Taper(0.103f, 0.077f, 0.76f);
        var upperSleeve = Taper(0.079f, 0.063f, 0.25f);
        var lowerSleeve = Taper(0.064f, 0.050f, 0.23f);
        _torso = new Node3D();
        _model.AddChild(_torso);

        foreach (var side in new[] { -1, 1 })
        {
            Model(_model, leg, cloth, new(side * 0.12f, -0.44f, 0));
            Model(_model, ball, leather, new(side * 0.12f, -0.82f, -0.025f), new(0.092f, 0.065f, 0.159f));
            var slot = (side + 1) / 2;
            var shoulder = new Node3D
            {
                Position = new Vector3(side * (electrician ? 0.192f : 0.220f), 0.365f, 0.008f),
                Rotation = new Vector3(0.09f, 0, side * 0.13f)
            };
            _torso.AddChild(shoulder);
            _shoulders[slot] = shoulder;
            Model(shoulder, ball, coat, new(0, -0.016f, 0), new(0.079f, 0.092f, 0.078f));
            Model(shoulder, upperSleeve, coat, new(0, -0.127f, 0));
            var elbow = new Node3D { Position = new Vector3(0, -0.249f, 0) };
            shoulder.AddChild(elbow);
            _elbows[slot] = elbow;
            Model(elbow, lowerSleeve, coat, new(0, -0.109f, 0));
            Model(elbow, Taper(0.051f, 0.047f, 0.046f), seam, new(0, -0.217f, 0));
            Model(elbow, ball, skin, new(0, -0.265f, -0.008f), new(0.039f, 0.061f, 0.030f));
            Model(elbow, ball, skin, new(-side * 0.026f, -0.251f, -0.030f), new(0.016f, 0.038f, 0.012f));
        }

        Model(_torso, ball, coat, new(0, 0.157f, 0), new(electrician ? 0.223f : 0.255f, 0.372f, 0.151f));
        Model(_torso, Taper(0.098f, 0.135f, 0.15f), collar, new(0, 0.486f, 0));
        Model(_torso, ball, shirt, new(0, 0.453f, -0.101f), new(0.057f, 0.065f, 0.023f));
        Model(_torso, Taper(0.068f, 0.074f, 0.17f), skin, new(0, 0.538f, -0.021f));

        Model(_torso, new BoxMesh { Size = new Vector3(0.012f, 0.37f, 0.012f) }, seam, new(0, 0.203f, -0.151f));
        Model(_torso, new BoxMesh { Size = new Vector3(0.045f, 0.016f, 0.010f) }, shirt, new(0.09f, 0.310f, -0.141f));

        _head = new Node3D { Name = "WitnessHead", Position = new Vector3(0, 0.59f, -0.025f) };
        _torso.AddChild(_head);
        var complexion = Paint(electrician ? "b9876d" : "ad8675");
        complexion.VertexColorUseAsAlbedo = true;
        Model(_head, WitnessFace.Build(electrician), complexion, Vector3.Zero);
        if (!electrician)
            Model(_head, WitnessFace.Hair(), Paint("6e6660"), Vector3.Zero);
        _mouth = Model(_head, ball, Paint(electrician ? "88564b" : "795349"),
            new(0, 0.063f, -0.098f), new(0.028f, 0.0039f, 0.008f));

        if (electrician)
        {
            var helmet = Paint("ba923f");
            var reflective = Paint("c5b365");
            Model(_head, ball, helmet, new(0, 0.226f, 0.007f), new(0.117f, 0.083f, 0.116f));
            Model(_head, Taper(0.119f, 0.122f, 0.013f), helmet, new(0, 0.213f, -0.015f));
            Model(_head, ball, hair, new(0, 0.10f, 0.106f), new(0.067f, 0.076f, 0.061f));
            var stripe = new BoxMesh { Size = new Vector3(0.032f, 0.28f, 0.028f) };
            Model(_torso, stripe, reflective, new(-0.11f, 0.235f, -0.126f));
            Model(_torso, stripe, reflective, new(0.11f, 0.235f, -0.126f));
        }

        _eyes = new Node3D { Position = new Vector3(0, 0.152f, -0.097f) };
        _head.AddChild(_eyes);
        var sclera = Paint("c9baaa");
        var iris = Paint(electrician ? "584738" : "4e5651");
        var pupil = Paint("272323");
        foreach (var side in new[] { -1, 1 })
        {
            Model(_head, ball, skin, new(side * 0.083f, 0.120f, 0.001f), new(0.014f, 0.028f, 0.013f));
            Model(_eyes, ball, sclera, new(side * 0.033f, 0, 0.001f), new(0.0130f, 0.0043f, 0.005f));
            Model(_eyes, ball, iris, new(side * 0.033f, 0, -0.0035f), new(0.0046f, 0.0046f, 0.0018f));
            Model(_eyes, ball, pupil, new(side * 0.033f, 0, -0.0049f), new(0.0019f, 0.0020f, 0.0010f));
            var brow = Model(_head, ball, hair, new(side * 0.034f, 0.169f, -0.096f), new(0.015f, 0.0027f, 0.004f));
            brow.RotationDegrees = new Vector3(0, side * -15, side * -8);
        }
    }

    public void SetPose(WitnessPose pose)
    {
        _pose = pose;
        _poseTime = 0;
    }

    public void Animate(float delta, Vector3 observer)
    {
        _poseTime += delta;
        _blinkTime = (_blinkTime + delta) % 3.7f;
        var direction = observer - _body.GlobalPosition;
        var facing = _pose == WitnessPose.Idle ? (_electrician ? -0.32f : 0.45f) : Mathf.Atan2(-direction.X, -direction.Z);
        var blend = Mathf.Min(1f, 7f * delta);
        _model.Rotation = new Vector3(0, Mathf.LerpAngle(_model.Rotation.Y, facing, blend), 0);
        var turn = _pose switch
        {
            WitnessPose.Statement when _electrician =>
                new Vector3(-0.025f + 0.07f * Mathf.Sin(_poseTime * 5.5f), -0.16f, -0.04f),
            WitnessPose.Statement => new Vector3(-0.055f + 0.048f * Mathf.Sin(_poseTime * 4f), 0.20f, 0),
            WitnessPose.Clarification when _poseTime < 0.65f => new Vector3(-0.25f, -0.43f, 0.075f),
            WitnessPose.Clarification => new Vector3(-0.12f * Mathf.Sin((_poseTime - 0.65f) * 4.1f) *
                Mathf.Exp(-(_poseTime - 0.65f) * 1.2f), 0.16f, 0),
            _ => new Vector3(0, -0.10f + 0.08f * Mathf.Sin(_poseTime), 0)
        };
        _head.Rotation = _head.Rotation.Lerp(turn, blend);
        var blink = _blinkTime > 3.52f ? Mathf.Max(0.08f, Mathf.Abs(_blinkTime - 3.61f) / 0.09f) : 1f;
        _eyes.Scale = new Vector3(1, blink, 1);
        var speaking = _pose != WitnessPose.Idle && _poseTime < 4.1f;
        var syllable = speaking ? Mathf.Max(0, Mathf.Sin(_poseTime * 11f) * Mathf.Sin(_poseTime * 5.9f + 0.9f)) : 0f;
        _mouth.Scale = new Vector3(0.028f - 0.004f * syllable, 0.0039f + 0.0056f * syllable, 0.008f);
        _torso.Rotation = new Vector3(0.009f * Mathf.Sin(_blinkTime * 1.7f), 0, 0);
        for (var index = 0; index < _shoulders.Length; index++)
        {
            var side = index == 0 ? -1 : 1;
            var gesture = speaking && (side < 0) == _electrician
                ? Mathf.Pow(Mathf.Max(0, Mathf.Sin(_poseTime * 2f - 0.7f)), 2f) : 0f;
            _shoulders[index].Rotation = _shoulders[index].Rotation.Lerp(
                new Vector3(0.08f - 0.24f * gesture, 0.12f * side * gesture, side * (0.12f + 0.16f * gesture)), blend);
            _elbows[index].Rotation = _elbows[index].Rotation.Lerp(new Vector3(-0.10f + 0.92f * gesture, 0, 0), blend);
        }
    }

    private static StandardMaterial3D Paint(string color) =>
        new() { AlbedoColor = new Color(color), Roughness = 0.88f };

    private static CylinderMesh Taper(float top, float bottom, float height) =>
        new() { TopRadius = top, BottomRadius = bottom, Height = height, RadialSegments = 16, Rings = 1 };

    private static MeshInstance3D Model(Node3D parent, Mesh mesh, StandardMaterial3D material,
        Vector3 position, Vector3? scale = null)
    {
        var model = new MeshInstance3D { Mesh = mesh, MaterialOverride = material, Position = position, Scale = scale ?? Vector3.One };
        parent.AddChild(model);
        return model;
    }
}
