using Godot;

namespace TwentyThreeSeventeen;

internal static class WitnessFace
{
    public static ArrayMesh Build(bool electrician)
    {
        const int around = 72;
        const int rows = 48;
        var surface = new SurfaceTool();
        surface.Begin(Mesh.PrimitiveType.Triangles);
        surface.SetSmoothGroup(0);
        for (var row = 0; row <= rows; row++)
        {
            var latitude = Mathf.Lerp(-1.568f, 1.568f, row / (float)rows);
            var y = 0.133f + 0.130f * Mathf.Sin(latitude);
            for (var column = 0; column < around; column++)
            {
                var heading = Mathf.Tau * column / around;
                var across = Mathf.Sin(heading);
                var front = Mathf.Cos(heading);
                var width = electrician ? 0.076f : 0.083f;
                var jaw = Mathf.Lerp(electrician ? 0.72f : 0.81f, 1f, Mathf.SmoothStep(0.054f, 0.132f, y));
                var x = width * jaw * Mathf.Cos(latitude) * across;
                var z = -0.096f * Mathf.Cos(latitude) * front;
                var face = Mathf.SmoothStep(0.38f, 0.94f, front);
                var nose = 0.029f * Hill(x, y, 0f, 0.115f, 0.017f, 0.021f)
                    + 0.013f * Hill(x, y, 0f, 0.146f, 0.012f, 0.032f);
                var socket = 0.011f * Hill(Mathf.Abs(x), y, 0.034f, 0.153f, 0.016f, 0.009f);
                var cheeks = 0.009f * Hill(Mathf.Abs(x), y, 0.043f, 0.121f, 0.022f, 0.023f);
                var chin = 0.020f * Hill(x, y, 0f, 0.033f, 0.039f, 0.031f);
                z -= face * (nose + cheeks + chin - socket);
                var tone = 0.96f - face * (socket * 10f + (electrician ? 0.055f : 0.18f)
                    * Hill(x, y, 0f, 0.043f, 0.082f, 0.046f));
                var flush = face * cheeks * 3.2f;
                surface.SetColor(new Color(tone + flush, tone - flush, tone - 1.2f * flush));
                surface.AddVertex(new Vector3(x, y, z));
            }
        }
        Connect(surface, around, rows);
        return surface.Commit();
    }

    public static ArrayMesh Hair()
    {
        const int around = 44;
        const int rows = 12;
        var surface = new SurfaceTool();
        surface.Begin(Mesh.PrimitiveType.Triangles);
        for (var row = 0; row <= rows; row++)
        {
            for (var column = 0; column < around; column++)
            {
                var heading = Mathf.Tau * column / around;
                var front = Mathf.Cos(heading);
                var hairline = Mathf.Lerp(-0.29f, 0.69f, Mathf.SmoothStep(-0.5f, 0.92f, front));
                hairline += 0.045f * Mathf.Sin(heading * 6f) + 0.038f * Mathf.Cos(heading * 13f);
                var latitude = Mathf.Lerp(hairline, 1.568f, row / (float)rows);
                var radius = Mathf.Cos(latitude);
                surface.AddVertex(new Vector3(0.086f * radius * Mathf.Sin(heading),
                    0.133f + 0.136f * Mathf.Sin(latitude), -0.100f * radius * front));
            }
        }
        Connect(surface, around, rows);
        return surface.Commit();
    }

    private static float Hill(float x, float y, float centerX, float centerY, float width, float height)
    {
        var u = (x - centerX) / width;
        var v = (y - centerY) / height;
        return Mathf.Exp(-(u * u + v * v));
    }

    private static void Connect(SurfaceTool surface, int around, int rows)
    {
        for (var row = 0; row < rows; row++)
        {
            for (var column = 0; column < around; column++)
            {
                var lower = row * around + column;
                var next = row * around + (column + 1) % around;
                surface.AddIndex(lower);
                surface.AddIndex(next);
                surface.AddIndex(lower + around);
                surface.AddIndex(next);
                surface.AddIndex(next + around);
                surface.AddIndex(lower + around);
            }
        }
        surface.GenerateNormals();
    }
}
