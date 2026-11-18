"""Standard-library unittest adapter for the Factory test-count contract."""
import argparse
import json
from pathlib import Path
import unittest


class CountingResult(unittest.TextTestResult):
    """Count skipped parent tests separately from fixtures and subtests."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.current_test = None
        self.skipped_tests = 0

    def startTest(self, test):
        self.current_test = test
        super().startTest(test)

    def stopTest(self, test):
        super().stopTest(test)
        self.current_test = None

    def addSkip(self, test, reason):
        if test is self.current_test:
            self.skipped_tests += 1
            super().addSkip(test, reason)
        else:
            # Older TextTestResult omits fixture/subtest identities. Keep their
            # verbose lines explicit so the flow parser can distinguish them.
            unittest.TestResult.addSkip(self, test, reason)
            prefix = "\n  " if self.current_test is not None else ""
            # Descriptions include docstrings; parameter reprs can also span lines.
            identity = str(test).replace("\r", "\\r").replace("\n", "\\n")
            self.stream.writeln(prefix + identity + " ... skipped " + repr(reason))
            self._newline = True


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--start-directory',default='.')
    parser.add_argument('--pattern',default='test*.py')
    parser.add_argument('--report',help='Write the counts to this JSON file; nothing is written without it.')
    args=parser.parse_args()
    suite=unittest.defaultTestLoader.discover(args.start_directory,pattern=args.pattern)
    result=unittest.TextTestRunner(verbosity=2,resultclass=CountingResult).run(suite)
    if args.report:Path(args.report).write_text(json.dumps({'executed':result.testsRun-result.skipped_tests,
        'failed':len(result.failures)+len(result.errors)+len(result.unexpectedSuccesses),
        'skipped':len(result.skipped)})+'\n')
    return 0 if result.wasSuccessful() else 1


if __name__=='__main__':raise SystemExit(main())
