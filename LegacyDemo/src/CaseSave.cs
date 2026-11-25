using Godot;
using System;
using System.IO;
using System.Text.Json;

namespace TwentyThreeSeventeen;

internal sealed record CaseSnapshot(int Version, int Checkpoint, CaseItem[] Sources,
    Comparison[] Comparisons, bool Challenged, TimelineEvent[] Timeline, int[] ReportAnswers);

internal enum SaveStatus { Missing, Ready, Invalid }

internal static class CaseSave
{
    public const int Version = 1;
    private static string Path => ProjectSettings.GlobalizePath("user://case.json");

    public static SaveStatus Load(Investigation investigation, out int checkpoint)
    {
        checkpoint = 0;
        try
        {
            using var stream = System.IO.File.OpenRead(Path);
            var snapshot = JsonSerializer.Deserialize<CaseSnapshot>(stream);
            if (snapshot == null || !investigation.Restore(snapshot))
                return SaveStatus.Invalid;

            checkpoint = snapshot.Checkpoint;
            return SaveStatus.Ready;
        }
        catch (Exception error) when (error is FileNotFoundException or DirectoryNotFoundException)
        {
            return SaveStatus.Missing;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or JsonException)
        {
            return SaveStatus.Invalid;
        }
    }

    public static bool Write(CaseSnapshot snapshot)
    {
        var path = Path;
        var temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            using (var stream = System.IO.File.Create(temporary))
            {
                JsonSerializer.Serialize(stream, snapshot);
                stream.Flush(flushToDisk: true);
            }

            System.IO.File.Move(temporary, path, overwrite: true);
            return true;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            GD.PushWarning("Could not save investigation progress: " + error.Message);
            return false;
        }
        finally
        {
            try
            {
                System.IO.File.Delete(temporary);
            }
            catch (Exception cleanupError) when (cleanupError is IOException or UnauthorizedAccessException)
            {
                GD.PushWarning("Could not remove an autosave temporary file: " + cleanupError.Message);
            }
        }
    }

    public static bool Clear()
    {
        try
        {
            System.IO.File.Delete(Path);
            return true;
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException)
        {
            GD.PushWarning("Could not clear investigation progress: " + error.Message);
            return false;
        }
    }
}
