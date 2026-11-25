using Godot;
using System;

namespace TwentyThreeSeventeen;

public partial class Detective : CharacterBody3D
{
    private const float WalkSpeed = 3.1f;
    private const float MouseSensitivity = 0.0025f;
    private const float Gravity = 18f;
    private const float StrideLength = 1.03f;

    private Camera3D _camera = null!;
    private RayCast3D _aim = null!;
    private float _stride;
    internal event Action? Footstep;

    public bool ControlsEnabled { get; set; }
    public Vector3 EyePosition => _camera.GlobalPosition;
    internal CaseItem? Target => _aim.GetCollider() is StaticBody3D body && body.HasMeta("case_item")
        ? (CaseItem)body.GetMeta("case_item").AsInt32()
        : null;

    public override void _Ready()
    {
        FloorSnapLength = 0.35f;
        FloorConstantSpeed = true;

        AddChild(new CollisionShape3D
        {
            Position = new Vector3(0, 0.85f, 0),
            Shape = new CapsuleShape3D { Radius = 0.29f, Height = 1.7f }
        });

        _camera = new Camera3D
        {
            Position = new Vector3(0, 1.58f, 0),
            Near = 0.07f,
            Far = 40f,
            Fov = 74f,
            Current = true
        };
        AddChild(_camera);

        _aim = new RayCast3D { TargetPosition = new Vector3(0, 0, -2.7f) };
        _camera.AddChild(_aim);
        _aim.AddException(this);
    }

    public override void _UnhandledInput(InputEvent input)
    {
        if (!ControlsEnabled || input is not InputEventMouseMotion motion)
            return;

        RotateY(-motion.ScreenRelative.X * MouseSensitivity);
        var rotation = _camera.Rotation;
        rotation.X = Mathf.Clamp(rotation.X - motion.ScreenRelative.Y * MouseSensitivity, -1.48f, 1.48f);
        _camera.Rotation = rotation;
    }

    public override void _PhysicsProcess(double delta)
    {
        var keys = ControlsEnabled
            ? Input.GetVector("move_left", "move_right", "move_forward", "move_back")
            : Vector2.Zero;
        var direction = Transform.Basis * new Vector3(keys.X, 0, keys.Y);
        var vertical = IsOnFloor() ? -0.1f : Velocity.Y - Gravity * (float)delta;
        Velocity = new Vector3(direction.X * WalkSpeed, vertical, direction.Z * WalkSpeed);
        MoveAndSlide();
        _aim.ForceRaycastUpdate();

        if (keys == Vector2.Zero || !IsOnFloor())
        {
            _stride = 0;
            return;
        }

        var movement = GetPositionDelta();
        _stride += new Vector2(movement.X, movement.Z).Length();
        if (_stride >= StrideLength)
        {
            _stride -= StrideLength;
            Footstep?.Invoke();
        }
    }

    internal void FrameWitness(Vector3 position)
    {
        var direction = position - GlobalPosition;
        Rotation = new Vector3(0, Mathf.Atan2(-direction.X, -direction.Z), 0);
        _camera.LookAt(position);
    }

    public void ReturnTo(Vector3 position)
    {
        GlobalPosition = position;
        Rotation = Vector3.Zero;
        _camera.Rotation = Vector3.Zero;
        Velocity = Vector3.Zero;
        _stride = 0;
    }
}
