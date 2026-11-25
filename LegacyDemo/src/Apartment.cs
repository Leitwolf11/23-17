using Godot;
using System.Collections.Generic;

namespace TwentyThreeSeventeen;

internal sealed class Apartment
{
	public static readonly Vector3 Entrance = new(0, 0.04f, 5.15f);

	private const int Basement = -1;
	private const int TopFloor = 4;
	private const float Storey = 3f;
	private const float FlightLength = 5f;
	private const float FlightRise = Storey / 2f;
	private const float Lane = 0.97f;
	private const float LandingEdge = 2.5f;
	private const float MidLandingEnd = -3.65f;
	private const float BackWall = 5.65f;

	private readonly Node3D[] _floors = new Node3D[TopFloor - Basement + 1];
	private readonly List<Node3D> _details = [];
	private readonly List<OmniLight3D> _shadowLights = [];
	private readonly Dictionary<CaseItem, Label3D> _signs = [];
	private readonly Dictionary<Vector3, BoxMesh> _meshes = [];
	private readonly StandardMaterial3D _plaster = Material("98968f", "plaster");
	private readonly StandardMaterial3D _concrete = Material("495255");
	private readonly StandardMaterial3D _steps = Material("a29d90", "marble");
	private readonly StandardMaterial3D _rail = Material("252d32");
	private readonly StandardMaterial3D _wood = Material("4d3c35");
	private readonly StandardMaterial3D _paper = Material("c6a872");
	private readonly StandardMaterial3D _screen = Material("346d6a");
	private readonly StandardMaterial3D _lamp = new()
	{
		AlbedoColor = new Color("dfc99a"),
		ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded
	};

	private int _visibleFloor = int.MinValue;
	private readonly Dictionary<CaseItem, WitnessActor> _witnesses = [];

	public WitnessActor Witness(CaseItem item) => _witnesses[item];
	public static bool IsCheckpoint(int floor) => floor is >= Basement and <= TopFloor;
	public static Vector3 Checkpoint(int floor) => floor == 0 ? Entrance : new(0, floor * Storey + 0.04f, 4.65f);

	public static int LandingCheckpoint(Vector3 feet, int previous)
	{
		var floor = Mathf.RoundToInt(feet.Y / Storey);
		return IsCheckpoint(floor) && Mathf.Abs(feet.Y - floor * Storey) < 0.16f && feet.Z > 3.15f
			? floor : previous;
	}

	public void AnimateWitnesses(float delta, Vector3 observer)
	{
		foreach (var witness in _witnesses.Values)
			witness.Animate(delta, observer);
	}

	public Apartment(Node3D root)
	{
		var environment = new Godot.Environment
		{
			BackgroundMode = Godot.Environment.BGMode.Color,
			BackgroundColor = new Color("131a20"),
			AmbientLightSource = Godot.Environment.AmbientSource.Color,
			AmbientLightColor = new Color("9faeb8"),
			AmbientLightEnergy = 0.45f
		};
		root.AddChild(new WorldEnvironment { Environment = environment });

		for (var floor = Basement; floor <= TopFloor; floor++)
		{
			var level = new Node3D { Name = $"Floor{floor}", Position = Vector3.Up * (floor * Storey) };
			root.AddChild(level);
			_floors[floor - Basement] = level;
			BuildLanding(level, floor);
			if (floor < TopFloor)
				BuildStairs(level);
		}

		BuildServiceRoom(_floors[0]);
		var entrance = _floors[0 - Basement];
		BuildEntrance(entrance);
		BuildRainyEntrance(entrance);
		var thirdFloor = _floors[3 - Basement];
		Box(thirdFloor, new(-0.64f, 1.19f, 5.52f), new(0.15f, 0.24f, 0.08f), _rail);
		Box(thirdFloor, new(-0.64f, 1.24f, 5.47f), new(0.065f, 0.025f, 0.02f), _screen);
		_witnesses.Add(CaseItem.Electrician, BuildWitness(thirdFloor, CaseItem.Electrician, -1.16f));
		BuildFourthFloor(_floors[TopFloor - Basement]);
	}

	public void LocalizeSigns()
	{
		foreach (var (item, sign) in _signs)
			sign.Text = (string)TranslationServer.Translate(item.ToString().ToUpperInvariant() + "_TITLE");
	}

	public void SetQuality(bool details, bool shadows)
	{
		foreach (var node in _details)
			node.Visible = details;
		foreach (var light in _shadowLights)
			light.ShadowEnabled = shadows;
	}

	public void UpdateVisibleFloors(float eyeHeight)
	{
		var current = Mathf.Clamp(Mathf.FloorToInt((eyeHeight - 0.65f) / Storey), Basement, TopFloor);
		if (current == _visibleFloor)
			return;

		_visibleFloor = current;
		for (var floor = 0; floor < _floors.Length; floor++)
			_floors[floor].Visible = Mathf.Abs(floor + Basement - current) <= 1;
	}

	private void BuildLanding(Node3D level, int floor)
	{
		Box(level, new(0, -0.13f, 4.075f), new(3.9f, 0.26f, BackWall - LandingEdge), _steps, solid: true);
		Box(level, new(0, 0.014f, 3.17f), new(3.72f, 0.018f, 0.032f), _rail, detail: true);
		Box(level, new(0, 0.014f, 4.79f), new(3.72f, 0.018f, 0.032f), _rail, detail: true);
		if (floor == Basement)
		{
			foreach (var side in new[] { -1, 1 })
			{
				Box(level, new(side * 1.42f, 1.47f, BackWall + 0.08f), new(1.26f, 3.22f, 0.18f), _plaster, solid: true);
				Box(level, new(side * 0.74f, 1.06f, BackWall - 0.025f), new(0.13f, 2.12f, 0.14f), _rail, solid: true);
			}
			Box(level, new(0, 2.61f, BackWall + 0.08f), new(1.62f, 0.96f, 0.18f), _plaster, solid: true);
		}
		else if (floor == 0)
		{
			Collider(level, new(0, 1.47f, BackWall + 0.03f), new(4.1f, 3.22f, 0.18f));
		}
		else
		{
			Box(level, new(0, 1.47f, BackWall + 0.08f), new(4.1f, 3.22f, 0.18f), _plaster, solid: true);
			Box(level, new(0, 1.08f, BackWall - 0.065f), new(1.02f, 2.16f, 0.12f), _wood);
			Box(level, new(0, 1.08f, BackWall - 0.128f), new(0.76f, 1.87f, 0.015f), _rail);
			Box(level, new(0.35f, 1.04f, BackWall - 0.15f), new(0.18f, 0.06f, 0.06f), _paper);
		}
		Box(level, new(-2.03f, 1.47f, 1f), new(0.18f, 3.22f, 9.5f), _plaster, solid: true);
		Box(level, new(2.03f, 1.47f, 1f), new(0.18f, 3.22f, 9.5f), _plaster, solid: true);
		Box(level, new(0, 1.47f, MidLandingEnd - 0.1f), new(4.1f, 3.22f, 0.18f), _plaster, solid: true);

		if (floor == Basement)
			Box(level, new(-Lane, 0.51f, 2.36f), new(1.80f, 1.02f, 0.11f), _rail, solid: true);

		if (floor == TopFloor)
		{
			Box(level, new(0, 3.06f, 1f), new(4.1f, 0.2f, 9.5f), _concrete, solid: true);
			Box(level, new(Lane, 0.49f, 2.36f), new(1.94f, 0.98f, 0.10f), _rail, solid: true);
		}

		Box(level, new(0, 2.76f, 4.15f), new(0.7f, 0.12f, 0.24f), _lamp);
		Box(level, new(-1.87f, 0.09f, 4.08f), new(0.12f, 0.18f, 3.12f), _rail, detail: true);
		Box(level, new(1.87f, 0.09f, 4.08f), new(0.12f, 0.18f, 3.12f), _rail, detail: true);

		var light = new OmniLight3D
		{
			Position = new(0, 2.64f, 4.12f),
			LightColor = new Color(floor == TopFloor ? "b4cbd8" : "ffc98e"),
			LightEnergy = 1.18f,
			OmniRange = 5.4f
		};
		level.AddChild(light);
		if (floor == 0 || floor == TopFloor)
			_shadowLights.Add(light);

		level.AddChild(new Label3D
		{
			Text = floor == Basement ? "−1" : floor.ToString(),
			Position = new Vector3(1.39f, 1.88f, BackWall - 0.12f),
			RotationDegrees = new Vector3(0, 180, 0),
			Font = GameTypography.Emphasis,
			FontSize = 48,
			PixelSize = 0.00675f,
			Modulate = new Color("e8d8b6"),
			OutlineSize = 2
		});
	}

	private void BuildStairs(Node3D level)
	{
		Ramp(level, Lane, LandingEdge, 0, -LandingEdge, FlightRise);
		Ramp(level, -Lane, -LandingEdge, FlightRise, LandingEdge, Storey);
		Box(level, new(0, FlightRise - 0.12f, -3.075f), new(3.9f, 0.24f, 1.15f), _concrete, solid: true);
		Box(level, new(0, 2.54f, -3.48f), new(0.5f, 0.1f, 0.13f), _lamp);
	}

	private void Ramp(Node3D level, float x, float startZ, float startY, float endZ, float endY)
	{
		var rise = endY - startY;
		var angle = -Mathf.Atan2(rise, FlightLength) * Mathf.Sign(endZ - startZ);
		var center = new Vector3(x, (startY + endY) / 2, (startZ + endZ) / 2);
		var basis = new Basis(Vector3.Right, angle);
		var length = Mathf.Sqrt(FlightLength * FlightLength + rise * rise);
		var size = new Vector3(1.82f, 0.22f, length);
		var ramp = Collider(level, center - basis.Y * (size.Y / 2), size);
		ramp.Rotation = new Vector3(angle, 0, 0);

		const int count = 10;
		for (var step = 0; step < count; step++)
		{
			var part = (step + 0.5f) / count;
			var top = Mathf.Lerp(startY, endY, part);
			var z = Mathf.Lerp(startZ, endZ, part);
			Box(level, new(x, top - 0.13f, z), new(1.83f, 0.26f, FlightLength / count), _steps);
		}

		var innerEdge = Mathf.Sign(x) * 0.065f;
		var banister = Box(level, new(innerEdge, center.Y + 0.53f, center.Z),
			new(0.13f, 1.05f, length), _rail, solid: true);
		banister.Rotation = new Vector3(angle, 0, 0);
	}

	private void BuildEntrance(Node3D level)
	{
		Desk(level, -1.59f, 3.8f);
		Desk(level, 1.59f, 3.8f);

		var dispatch = Evidence(level, new(-1.56f, 1.04f, 3.8f), new(0.46f, 0.32f, 0.38f), CaseItem.Dispatch);
		Box(dispatch, new(0, -0.07f, 0), new(0.44f, 0.06f, 0.36f), _wood);
		Box(dispatch, new(0, -0.029f, 0), new(0.39f, 0.035f, 0.30f), _paper);
		for (var row = 0; row < 5; row++)
			Box(dispatch, new(-0.025f, -0.009f, -0.09f + row * 0.04f), new(0.27f, 0.003f, 0.006f), _rail, detail: true);

		var camera = Evidence(level, new(1.56f, 1.16f, 3.8f), new(0.47f, 0.52f, 0.36f), CaseItem.Camera);
		Box(camera, new(0, -0.18f, 0), new(0.28f, 0.16f, 0.26f), _rail);
		Box(camera, new(0, 0.04f, 0), new(0.47f, 0.33f, 0.33f), _rail);
		Box(camera, new(0, 0.055f, 0.17f), new(0.39f, 0.24f, 0.014f), _screen);
		Box(camera, new(0, 0.055f, -0.17f), new(0.39f, 0.24f, 0.014f), _screen);
		Box(camera, new(0.16f, -0.095f, 0.17f), new(0.023f, 0.012f, 0.016f), _lamp, detail: true);

		var mailboxes = Box(level, new(-1.28f, 1.46f, 5.53f), new(0.94f, 0.57f, 0.21f), _wood, detail: true);
		for (var slot = 0; slot < 4; slot++)
		{
			var x = -0.3375f + 0.225f * slot;
			Box(mailboxes, new(x, 0, -0.12f), new(0.21f, 0.46f, 0.04f), _rail);
			Box(mailboxes, new(x, 0.12f, -0.145f), new(0.14f, 0.026f, 0.02f), _concrete);
			Box(mailboxes, new(x, -0.10f, -0.15f), new(0.064f, 0.027f, 0.017f), _paper);
		}

		Box(level, new(0, 0.003f, 4.075f), new(3.87f, 0.004f, 3.13f), Terrazzo("535556", "857b6b"), detail: true);
		Box(level, new(0, 0.013f, 5.12f), new(1.28f, 0.012f, 0.58f), Material("343131"));
		var trim = Material("524638", "plaster");
		foreach (var side in new[] { -1, 1 })
		{
			Box(level, new(side * 1.944f, 0.45f, 4.05f), new(0.047f, 0.9f, 3.06f), trim);
			Box(level, new(side * 1.917f, 0.91f, 4.05f), new(0.064f, 0.047f, 3.06f), _wood);
		}
	}

	private void BuildRainyEntrance(Node3D level)
	{
		var frame = Material("393c37");
		frame.Roughness = 0.52f;
		frame.Metallic = 0.42f;
		for (var index = -2; index <= 2; index++)
		{
			var x = index * 0.93f;
			Box(level, new(x, 1.36f, BackWall - 0.02f), new(0.063f, 2.73f, 0.15f), frame);
		}

		Box(level, new(0, 2.71f, BackWall - 0.04f), new(3.94f, 0.086f, 0.18f), frame);
		Box(level, new(0, 2.94f, BackWall + 0.03f), new(4.1f, 0.44f, 0.19f), _plaster);
		Box(level, new(0, 0.27f, BackWall - 0.04f), new(3.88f, 0.53f, 0.16f), _wood);
		Box(level, new(0, 1.02f, BackWall - 0.072f), new(3.88f, 0.045f, 0.19f), frame);
		Box(level, new(-0.14f, 1.01f, BackWall - 0.17f), new(0.035f, 0.26f, 0.032f), _paper);
		var glass = new ShaderMaterial { Shader = GD.Load<Shader>("res://src/shaders/rain_glass.gdshader") };
		for (var pane = 0; pane < 4; pane++)
		{
			level.AddChild(new MeshInstance3D
			{
				Mesh = new QuadMesh { Size = new Vector2(0.88f, 2.15f) },
				MaterialOverride = glass,
				Position = new Vector3((pane - 1.5f) * 0.93f, 1.62f, BackWall + 0.01f),
				CastShadow = GeometryInstance3D.ShadowCastingSetting.Off
			});
		}

		var street = new Node3D { Name = "StreetBackdrop", Position = new Vector3(0, 0, BackWall) };
		level.AddChild(street);
		var pavement = Material("515659", "plaster");
		pavement.Roughness = 0.69f;
		var road = Material("30393d");
		road.Roughness = 0.30f;
		var walls = Material("4c565c", "plaster");
		var facadeTrim = Material("505f62");
		var glow = new StandardMaterial3D { AlbedoColor = new Color("afa994"), ShadingMode = BaseMaterial3D.ShadingModeEnum.Unshaded };
		Box(street, new(0, -0.18f, 1.10f), new(16f, 0.33f, 2.6f), pavement);
		Box(street, new(0, -0.24f, 2.46f), new(16f, 0.34f, 0.24f), facadeTrim);
		Box(street, new(0, -0.35f, 5.32f), new(16f, 0.35f, 6.0f), road);
		Box(street, new(0, -0.18f, 9.28f), new(16f, 0.33f, 2.8f), pavement);
		Box(street, new(0, 3.0f, 10.05f), new(16f, 6.30f, 0.47f), walls);
		Box(street, new(0, 2.25f, 9.79f), new(16f, 0.16f, 0.36f), facadeTrim);
		for (var bay = -2; bay <= 2; bay++)
		{
			var x = bay * 2.55f + 0.43f;
			Box(street, new(x, 3.53f, 9.79f), new(1.10f, 1.22f, 0.18f), frame);
			if (bay is -1 or 1)
				Box(street, new(x, 3.54f, 9.68f), new(0.89f, 1.02f, 0.08f), glow);
			Box(street, new(x, 1.04f, 9.78f), new(1.38f, 2.15f, 0.14f), _rail);
		}

		Box(street, new(-1.88f, 1.96f, 7.61f), new(0.074f, 3.92f, 0.074f), frame);
		Box(street, new(-1.56f, 3.85f, 7.61f), new(0.66f, 0.075f, 0.075f), frame);
		Box(street, new(-1.25f, 3.81f, 7.61f), new(0.25f, 0.10f, 0.18f), glow);
		street.AddChild(new OmniLight3D
		{
			Position = new Vector3(-1.21f, 3.61f, 7.50f),
			LightColor = new Color("fbcc91"),
			LightEnergy = 2.6f,
			OmniRange = 7.5f
		});
	}

	private static ShaderMaterial Terrazzo(string stone, string aggregate)
	{
		var material = new ShaderMaterial { Shader = GD.Load<Shader>("res://src/shaders/terrazzo.gdshader") };
		material.SetShaderParameter("stone", new Color(stone));
		material.SetShaderParameter("aggregate", new Color(aggregate));
		return material;
	}

	private void BuildFourthFloor(Node3D level)
	{
		var breaker = Evidence(level, new(-1.71f, 1.42f, 4.08f), new(0.42f, 0.58f, 0.5f), CaseItem.Breaker);
		Box(breaker, new(-0.11f, 0, 0), new(0.20f, 0.58f, 0.50f), _rail);
		Box(breaker, new(0.01f, 0.02f, 0), new(0.035f, 0.45f, 0.39f), _concrete);
		for (var slot = 0; slot < 4; slot++)
		{
			var z = -0.135f + 0.09f * slot;
			Box(breaker, new(0.041f, 0.02f, z), new(0.055f, 0.18f, 0.066f), _paper);
			Box(breaker, new(0.076f, 0.05f, z), new(0.033f, 0.06f, 0.042f), _rail);
		}

		_witnesses.Add(CaseItem.Witness, BuildWitness(level, CaseItem.Witness, 1.1f));
	}

	private WitnessActor BuildWitness(Node3D level, CaseItem item, float x)
	{
		var counter = Collider(level, new(x, 0.87f, 4.9f), new(0.68f, 1.74f, 0.55f));
		counter.SetMeta("case_item", (int)item);
		return new WitnessActor(counter, item == CaseItem.Electrician);
	}

	private void BuildServiceRoom(Node3D basement)
	{
		var room = new Node3D { Name = "ServiceRoom", Position = new Vector3(0, 0, BackWall) };
		basement.AddChild(room);
		var cabinet = Material("45595e");
		var conduit = Material("a88048");
		Box(room, new(0, -0.13f, 2.12f), new(4.12f, 0.26f, 4.24f), _concrete, solid: true);
		Box(room, new(0, 0.003f, 2.13f), new(4.10f, 0.005f, 4.19f), Terrazzo("444a48", "5c6257"), detail: true);
		Box(room, new(0, 2.92f, 2.12f), new(4.12f, 0.18f, 4.24f), _plaster, solid: true);
		Box(room, new(0, 1.43f, 4.2f), new(4.12f, 2.88f, 0.18f), _plaster, solid: true);
		foreach (var side in new[] { -1, 1 })
		{
			Box(room, new(side * 2.02f, 1.43f, 2.12f), new(0.18f, 2.88f, 4.24f), _plaster, solid: true);
			Box(room, new(side * 1.89f, 0.12f, 2.12f), new(0.09f, 0.20f, 4.12f), _rail, detail: true);
		}
		Box(room, new(-1.35f, 1.16f, 3.87f), new(0.96f, 1.78f, 0.40f), cabinet, solid: true);
		Box(room, new(-1.35f, 1.66f, 3.655f), new(0.72f, 0.39f, 0.04f), _rail);
		for (var bank = 0; bank < 4; bank++)
		{
			var x = -1.63f + bank * 0.18f;
			Box(room, new(x, 1.66f, 3.617f), new(0.10f, 0.21f, 0.05f), _paper, detail: true);
			Box(room, new(x, 2.36f, 3.96f), new(0.037f, 0.70f, 0.04f), conduit, detail: true);
			Box(room, new(x, 2.696f - bank * 0.054f, 2.32f), new(0.037f, 0.040f, 3.30f), conduit, detail: true);
		}

		Box(room, new(1.20f, 0.82f, 3.35f), new(1.28f, 0.12f, 0.75f), _wood, solid: true);
		Box(room, new(1.20f, 0.38f, 3.48f), new(1.06f, 0.76f, 0.48f), cabinet, solid: true);
		var terminal = Evidence(room, new(1.23f, 1.16f, 3.37f), new(0.58f, 0.54f, 0.44f), CaseItem.AccessLog);
		Box(terminal, Vector3.Zero, new(0.58f, 0.49f, 0.39f), _rail);
		Box(terminal, new(0, 0.04f, -0.20f), new(0.49f, 0.28f, 0.04f), _screen);
		for (var line = 0; line < 4; line++)
			Box(terminal, new(-0.025f, 0.12f - line * 0.06f, -0.228f), new(0.30f, 0.014f, 0.012f), _lamp);
		Box(room, new(0, 2.82f, 2.1f), new(1.12f, 0.089f, 0.29f), _rail);
		Box(room, new(0, 2.770f, 2.1f), new(1.03f, 0.030f, 0.18f), _lamp);
		var grille = Box(room, new(0, 2.13f, 4.08f), new(0.73f, 0.38f, 0.09f), _rail, detail: true);
		for (var slat = -3; slat <= 3; slat++)
			Box(grille, new(0, slat * 0.041f, -0.053f), new(0.60f, 0.013f, 0.022f), cabinet);
		room.AddChild(new OmniLight3D
		{
			Position = new(0, 2.66f, 2.1f),
			LightColor = new Color("b6d9cc"),
			LightEnergy = 1.2f,
			OmniRange = 5.2f
		});
	}

	private void Desk(Node3D level, float x, float z)
	{
		Box(level, new(x, 0.86f, z), new(0.66f, 0.12f, 0.68f), _wood, solid: true);
		Box(level, new(x, 0.42f, z), new(0.48f, 0.84f, 0.53f), _rail, solid: true);
	}

	private StaticBody3D Evidence(Node3D level, Vector3 center, Vector3 size, CaseItem item)
	{
		var body = Collider(level, center, size);
		body.SetMeta("case_item", (int)item);
		var sign = new Label3D
		{
			Position = new Vector3(-Mathf.Sign(center.X) * 0.24f, size.Y / 2 + 0.17f, -0.22f),
			Billboard = BaseMaterial3D.BillboardModeEnum.Enabled,
			Font = GameTypography.Emphasis,
			FontSize = 46,
			PixelSize = 0.002f,
			Modulate = new Color("f6ead1"),
			OutlineSize = 2
		};
		body.AddChild(sign);
		_signs.Add(item, sign);
		return body;
	}

	private Node3D Box(Node3D owner, Vector3 center, Vector3 size, Godot.Material material, bool solid = false,
		bool detail = false)
	{
		var node = solid ? Collider(owner, center, size) : new Node3D { Position = center };
		if (!solid)
			owner.AddChild(node);
		if (!_meshes.TryGetValue(size, out var mesh))
		{
			mesh = new BoxMesh { Size = size };
			_meshes.Add(size, mesh);
		}
		node.AddChild(new MeshInstance3D { Mesh = mesh, MaterialOverride = material });
		if (detail)
			_details.Add(node);
		return node;
	}

	private static StaticBody3D Collider(Node3D owner, Vector3 center, Vector3 size)
	{
		var body = new StaticBody3D { Position = center };
		body.AddChild(new CollisionShape3D { Shape = new BoxShape3D { Size = size } });
		owner.AddChild(body);
		return body;
	}

	private static StandardMaterial3D Material(string color, string? texture = null) => new()
	{
		AlbedoColor = new Color(color),
		AlbedoTexture = texture == null ? null : GD.Load<Texture2D>($"res://assets/textures/{texture}.png"),
		Uv1Scale = texture == "plaster" ? new Vector3(2.8f, 2.8f, 1) : Vector3.One,
		Roughness = 0.88f
	};
}
