"""Validate retained native rows and summarize five process repeats."""
import json
import pathlib
import re
import statistics
import sys

SAMPLE_NAME = re.compile(r'metrics-(balanced|irregular|clustered)-(\d+)-(\d+)-(1|2|4)-([1-5])-(lomp|system)\.log\Z')


def summarize(directory):
    groups = {}
    for path in sorted(directory.glob('metrics-*.log')):
        match = SAMPLE_NAME.fullmatch(path.name)
        if match is None:
            continue
        kernel, iterations, rounds, workers, repeat, runtime = match.groups()
        rows = [json.loads(line) for line in path.read_text().splitlines()
                if line.startswith('{')]
        assert rows and rows[0]['schedule'] == 'serial', path
        schedules = ('static', 'dynamic', 'guided', 'fac2') if runtime == 'lomp' else ('static', 'dynamic', 'guided')
        assert [(row['schedule'], row['trial']) for row in rows] == [('serial', 0)] + [
            (schedule, trial) for trial in (-1, 0) for schedule in schedules], path
        expected = rows[0]['checksum']
        for row in rows:
            assert (row['kernel'], row['iterations'], row['rounds']) == (kernel, int(iterations), int(rounds)), (path, row)
            assert row['threads'] == (1 if row['schedule'] == 'serial' else int(workers)), (path, row)
            assert row['errors'] == 0 and row['checksum'] == expected, (path, row)
            assert row['elapsed_ns'] > 0, (path, row)
            if 'worker_iterations' in row:
                assert len(row['worker_iterations']) == row['threads'], (path, row)
                assert sum(row['worker_iterations']) == row['iterations'], (path, row)
            if row['trial'] < 0:
                continue
            key = (runtime, row['kernel'], row['iterations'], row['rounds'],
                   workers, row['schedule'])
            groups.setdefault(key, []).append(row['elapsed_ns'])
    assert groups, 'no measurements'
    output = []
    for key, values in sorted(groups.items()):
        assert len(values) == 5, (key, len(values))
        output.append(dict(zip(('runtime', 'kernel', 'iterations', 'rounds',
                                'requested_threads', 'schedule'), key),
                           repeats=len(values), median_ns=statistics.median(values),
                           min_ns=min(values), max_ns=max(values),
                           short_sample=min(values) < 10_000_000))
    return output


if __name__ == '__main__':
    root = pathlib.Path(sys.argv[1])
    (root / 'metrics-summary.json').write_text(json.dumps(summarize(root), indent=2) + '\n')
