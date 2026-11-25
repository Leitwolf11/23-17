using System;
using System.Collections.Generic;
using System.Linq;

namespace TwentyThreeSeventeen;

internal enum CaseItem { Dispatch, Camera, Breaker, Witness, AccessLog, Electrician }
internal enum Comparison { CameraClock, DarkLanding, ElectricianRoute }
internal enum ReportFinding { Supported, Incomplete, Unsupported }
internal enum TimelineEvent { Outage, Bell, ServiceEntry, WitnessClaim, Arrival, PowerRestored }

internal sealed class Investigation
{
    public static readonly IReadOnlyList<CaseItem> EvidenceOrder =
        [CaseItem.Dispatch, CaseItem.Camera, CaseItem.Breaker, CaseItem.AccessLog];
    public static readonly IReadOnlyList<CaseItem> SourceOrder =
        [.. EvidenceOrder, CaseItem.Witness, CaseItem.Electrician];

    private static readonly TimelineEvent[] UnsortedTimeline =
        [TimelineEvent.Arrival, TimelineEvent.PowerRestored, TimelineEvent.ServiceEntry,
         TimelineEvent.WitnessClaim, TimelineEvent.Outage, TimelineEvent.Bell];

    private readonly HashSet<CaseItem> _sources = [];
    private readonly HashSet<Comparison> _comparisons = [];
    private readonly TimelineEvent[] _timeline = (TimelineEvent[])UnsortedTimeline.Clone();
    private readonly int[] _reportAnswers = [-1, -1, -1];

    public IReadOnlyList<TimelineEvent> VisibleTimeline => _timeline.Where(KnowsEvent).ToArray();
    public IReadOnlyList<int> ReportAnswers => _reportAnswers;
    public bool Challenged { get; private set; }
    public bool CanChallenge => Has(Comparison.DarkLanding);
    public int EvidenceCount => EvidenceOrder.Count(Has);
    public bool ReadyToReport => _sources.Count == SourceOrder.Count &&
        _comparisons.Count == Enum.GetValues<Comparison>().Length && Challenged;

    public static bool IsWitness(CaseItem item) => item is CaseItem.Witness or CaseItem.Electrician;
    public bool Has(CaseItem item) => _sources.Contains(item);
    public bool Has(Comparison comparison) => _comparisons.Contains(comparison);
    public bool Inspect(CaseItem item) => _sources.Add(item);

    public Comparison? Compare(CaseItem first, CaseItem second)
    {
        if (!Has(first) || !Has(second))
            return null;

        var match = (first, second) switch
        {
            (CaseItem.Dispatch, CaseItem.Camera) or (CaseItem.Camera, CaseItem.Dispatch) => Comparison.CameraClock,
            (CaseItem.Witness, CaseItem.Breaker) or (CaseItem.Breaker, CaseItem.Witness) => Comparison.DarkLanding,
            (CaseItem.Electrician, CaseItem.AccessLog) or (CaseItem.AccessLog, CaseItem.Electrician) => Comparison.ElectricianRoute,
            _ => (Comparison?)null
        };
        if (match.HasValue)
            _comparisons.Add(match.Value);
        return match;
    }

    public bool Challenge()
    {
        if (!CanChallenge || Challenged)
            return false;

        Challenged = true;
        return true;
    }

    public void MoveTimelineCard(int index, int direction)
    {
        var timeline = VisibleTimeline;
        var neighbor = index + direction;
        if (direction is not (-1 or 1) || (uint)index >= timeline.Count || (uint)neighbor >= timeline.Count)
            return;

        var first = Array.IndexOf(_timeline, timeline[index]);
        var second = Array.IndexOf(_timeline, timeline[neighbor]);
        (_timeline[first], _timeline[second]) = (_timeline[second], _timeline[first]);
    }

    public void SetReportAnswer(int index, int value) => _reportAnswers[index] = value;

    public ReportFinding Evaluate()
    {
        var timeline = VisibleTimeline;
        if (!_reportAnswers.SequenceEqual([1, 1, 2]) || !timeline.SequenceEqual(timeline.Order()))
            return ReportFinding.Unsupported;

        return ReadyToReport ? ReportFinding.Supported : ReportFinding.Incomplete;
    }

    public CaseSnapshot Snapshot(int checkpoint) =>
        new(CaseSave.Version, checkpoint, _sources.Order().ToArray(), _comparisons.Order().ToArray(),
            Challenged, (TimelineEvent[])_timeline.Clone(), (int[])_reportAnswers.Clone());

    public bool Restore(CaseSnapshot snapshot)
    {
        if (snapshot.Version != CaseSave.Version || !Apartment.IsCheckpoint(snapshot.Checkpoint) ||
            snapshot.Sources is not { } sources || sources.Any(item => !Enum.IsDefined(item)) ||
            snapshot.Comparisons is not { } comparisons || comparisons.Any(match => !Enum.IsDefined(match)) ||
            snapshot.Timeline is not { } timeline || !timeline.Order().SequenceEqual(Enum.GetValues<TimelineEvent>()) ||
            snapshot.ReportAnswers is not { Length: 3 } answers || answers.Any(answer => answer is < -1 or > 2))
            return false;

        var discovered = sources.ToHashSet();
        if (comparisons.Any(match => match switch
            {
                Comparison.CameraClock => !discovered.Contains(CaseItem.Dispatch) || !discovered.Contains(CaseItem.Camera),
                Comparison.DarkLanding => !discovered.Contains(CaseItem.Witness) || !discovered.Contains(CaseItem.Breaker),
                _ => !discovered.Contains(CaseItem.Electrician) || !discovered.Contains(CaseItem.AccessLog)
            }) || (snapshot.Challenged && !comparisons.Contains(Comparison.DarkLanding)))
            return false;

        _sources.UnionWith(discovered);
        _comparisons.UnionWith(comparisons);
        Challenged = snapshot.Challenged;
        timeline.CopyTo(_timeline, 0);
        answers.CopyTo(_reportAnswers, 0);
        return true;
    }

    public void Reset()
    {
        _sources.Clear();
        _comparisons.Clear();
        UnsortedTimeline.CopyTo(_timeline, 0);
        Array.Fill(_reportAnswers, -1);
        Challenged = false;
    }

    private bool KnowsEvent(TimelineEvent item) => item switch
    {
        TimelineEvent.Outage or TimelineEvent.PowerRestored => Has(CaseItem.Breaker),
        TimelineEvent.Bell => Has(CaseItem.Dispatch),
        TimelineEvent.ServiceEntry => Has(CaseItem.AccessLog),
        TimelineEvent.WitnessClaim => Has(CaseItem.Witness),
        TimelineEvent.Arrival => Has(CaseItem.Camera),
        _ => false
    };
}
