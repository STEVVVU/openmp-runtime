import importlib.util
import json
import pathlib
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('metrics', pathlib.Path(__file__).with_name('summarize-metrics.py'))
metrics = importlib.util.module_from_spec(spec)
spec.loader.exec_module(metrics)


class SummaryTests(unittest.TestCase):
    def fixture(self, root):
        base = dict(kernel='balanced', iterations=8, rounds=32, errors=0, checksum=123,
                    elapsed_ns=100, threads=1)
        rows = [dict(base, schedule='serial', trial=0)]
        rows += [dict(base, schedule=schedule, trial=trial, threads=2,
                      worker_iterations=[4, 4], worker_work=[128, 128])
                 for trial in (-1, 0) for schedule in ('static', 'dynamic', 'guided', 'fac2')]
        for repeat in range(1, 6):
            (root / f'metrics-balanced-8-32-2-{repeat}-lomp.log').write_text(
                '\n'.join(json.dumps(row) for row in rows))

    def test_validation_log_is_not_a_sample(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            self.fixture(root)
            (root / 'metrics-validation.log').write_text('Traceback: prior failed run\n')
            result = metrics.summarize(root)
            self.assertEqual(len(result), 5)
            self.assertTrue(all(row['repeats'] == 5 for row in result))

    def test_missing_repeat_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            self.fixture(root)
            (root / 'metrics-balanced-8-32-2-5-lomp.log').unlink()
            with self.assertRaises(AssertionError):
                metrics.summarize(root)

    def test_missing_schedule_row_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            self.fixture(root)
            path = root / 'metrics-balanced-8-32-2-5-lomp.log'
            path.write_text('\n'.join(path.read_text().splitlines()[:-1]))
            with self.assertRaises(AssertionError):
                metrics.summarize(root)


if __name__ == '__main__':
    unittest.main()
