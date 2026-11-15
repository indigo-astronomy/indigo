"""Standard-library unittest adapter for the Factory test-count contract."""
import argparse
import json
from pathlib import Path
import unittest


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--start-directory',default='.')
    parser.add_argument('--pattern',default='test*.py')
    parser.add_argument('--report',help='Write the counts to this JSON file; nothing is written without it.')
    args=parser.parse_args()
    suite=unittest.defaultTestLoader.discover(args.start_directory,pattern=args.pattern)
    result=unittest.TextTestRunner(verbosity=2).run(suite)
    if args.report:Path(args.report).write_text(json.dumps({'executed':result.testsRun,
        'failed':len(result.failures)+len(result.errors)+len(result.unexpectedSuccesses),
        'skipped':len(result.skipped)+len(result.expectedFailures)})+'\n')
    return 0 if result.wasSuccessful() else 1


if __name__=='__main__':raise SystemExit(main())
