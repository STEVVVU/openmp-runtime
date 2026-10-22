"""Validate retained native rows and summarize five process repeats."""
import json
import pathlib
import statistics
import sys


def summarize(directory):
    groups = {}
    for path in sorted(directory.glob('metrics-*.log')):
        runtime = path.stem.rsplit('-', 1)[1]
        rows = [json.loads(line) for line in path.read_text().splitlines()
                if line.startswith('{')]
        assert rows and rows[0]['schedule'] == 'serial', path
        expected = rows[0]['checksum']
        for row in rows:
            assert row['errors'] == 0 and row['checksum'] == expected, (path, row)
            assert row['elapsed_ns'] > 0, (path, row)
            if 'worker_iterations' in row:
                assert len(row['worker_iterations']) == row['threads'], (path, row)
                assert sum(row['worker_iterations']) == row['iterations'], (path, row)
            if row['trial'] < 0:
                continue
            key = (runtime, row['kernel'], row['iterations'], row['rounds'],
                   path.stem.split('-')[-3], row['schedule'])
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
