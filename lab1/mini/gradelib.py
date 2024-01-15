#!/usr/bin/env python2

# This script is originally from MIT JOS, and has been slightly modified
# for use in some Spring 2013 CS439 labs.

import sys, os, re, time, socket, select, subprocess, errno, shutil
from subprocess import check_call, Popen
from optparse import OptionParser

__all__ = []

##################################################################
# Test structure
#

__all__ += ["test", "end_part", "run_tests", "get_current_test"]

TESTS = []
TOTAL = POSSIBLE = 0
PART_TOTAL = PART_POSSIBLE = 0
CURRENT_TEST = None

def test(points, title=None, parent=None):
    """Decorator for declaring test functions.  If title is None, the
    title of the test will be derived from the function name by
    stripping the leading "test_" and replacing underscores with
    spaces."""

    def register_test(fn, title=title):
        if not title:
            assert fn.func_name.startswith("test_")
            title = fn.func_name[5:].replace("_", " ")
        if parent:
            title = "  " + title

        def run_test():
            global TOTAL, POSSIBLE, CURRENT_TEST

            # Handle test dependencies
            if run_test.complete:
                return
            run_test.complete = True
            if parent:
                parent()

            # Run the test
            fail = None
            start = time.time()
            CURRENT_TEST = run_test
            sys.stdout.write("%s: " % title)
            sys.stdout.flush()
            try:
                fn()
            except AssertionError, e:
                fail = str(e)

            # Display and handle test result
            POSSIBLE += points
            if points:
                print "%s" % \
                    (color("red", "FAIL") if fail else color("green", "OK")),
            if time.time() - start > 0.1:
                print "(%.1fs)" % (time.time() - start),
            print
            if fail:
                print "    %s" % fail.replace("\n", "\n    ")
            else:
                TOTAL += points
            for callback in run_test.on_finish:
                callback(fail)
            CURRENT_TEST = None

        # Record test metadata on the test wrapper function
        run_test.func_name = fn.func_name
        run_test.title = title
        run_test.complete = False
        run_test.parent = parent
        run_test.on_finish = []
        TESTS.append(run_test)
        return run_test
    return register_test

def end_part(name):
    def show_part():
        global PART_TOTAL, PART_POSSIBLE
        print "Part %s score: %d/%d" % \
            (name, TOTAL - PART_TOTAL, POSSIBLE - PART_POSSIBLE)
        print
        PART_TOTAL, PART_POSSIBLE = TOTAL, POSSIBLE
    show_part.title = ""
    TESTS.append(show_part)

def run_tests():
    """Set up for testing and run the registered test functions."""

    # Handle command line
    global options
    parser = OptionParser(usage="usage: %prog [-v] [filters...]")
    parser.add_option("-v", "--verbose", action="store_true",
                      help="print commands")
    parser.add_option("--color", choices=["never", "always", "auto"],
                      default="auto", help="never, always, or auto")
    parser.add_option("--no", action="store_true", default=False,
                      dest="limit_inverse",
                      help="only run tests not matching filter strings")
    (options, args) = parser.parse_args()

    # Start with a full build to catch build errors
    make()

    # Run tests
    limit = map(str.lower, args)
    def should_run(test):
        if not limit:
            return True
        else:
            matched = any(l in test.title.lower() for l in limit)
            return options.limit_inverse ^ matched
    try:
        for test in TESTS:
            if should_run(test):
                test()
        if not limit:
            print "Score: %d/%d" % (TOTAL, POSSIBLE)
    except KeyboardInterrupt:
        pass
    if TOTAL < POSSIBLE:
        sys.exit(1)

def get_current_test():
    if not CURRENT_TEST:
        raise RuntimeError("No test is running")
    return CURRENT_TEST

##################################################################
# Assertions
#

__all__ += ["assert_equal", "assert_lines_match"]

def assert_equal(got, expect, msg=""):
    if got == expect:
        return
    if msg:
        msg += "\n"
    raise AssertionError("%sgot:\n  %s\nexpected:\n  %s" %
                         (msg, str(got).replace("\n", "\n  "),
                          str(expect).replace("\n", "\n  ")))

def assert_lines_match(text, *regexps, **kw):
    """Assert that all of regexps match some line in text.  If a 'no'
    keyword argument is given, it must be a list of regexps that must
    *not* match any line in text."""

    def assert_lines_match_kw(no=[]):
        return no
    no = assert_lines_match_kw(**kw)

    # Check text against regexps
    lines = text.splitlines()
    good = set()
    bad = set()
    for i, line in enumerate(lines):
        if any(re.match(r, line) for r in regexps):
            good.add(i)
            regexps = [r for r in regexps if not re.match(r, line)]
        if any(re.match(r, line) for r in no):
            bad.add(i)

    if not regexps and not bad:
        return

    # We failed; construct an informative failure message
    show = set()
    for lineno in good.union(bad):
        for offset in range(-2, 3):
            show.add(lineno + offset)
    if regexps:
        show.update(n for n in range(len(lines) - 5, len(lines)))

    msg = []
    last = -1
    for lineno in sorted(show):
        if 0 <= lineno < len(lines):
            if lineno != last + 1:
                msg.append("...")
            last = lineno
            msg.append("%s %s" % (color("red", "BAD ") if lineno in bad else
                                  color("green", "GOOD") if lineno in good
                                  else "    ",
                                  lines[lineno]))
    if last != len(lines) - 1:
        msg.append("...")
    if bad:
        msg.append("unexpected lines in output")
    for r in regexps:
        msg.append(color("red", "MISSING") + " '%s'" % r)
    raise AssertionError("\n".join(msg))

##################################################################
# Utilities
#

__all__ += ["make", "maybe_unlink", "color"]

MAKE_TIMESTAMP = 0

def pre_make():
    """Delay prior to running make to ensure file mtimes change."""
    while int(time.time()) == MAKE_TIMESTAMP:
        time.sleep(0.1)

def post_make():
    """Record the time after make completes so that the next run of
    make can be delayed if needed."""
    global MAKE_TIMESTAMP
    MAKE_TIMESTAMP = int(time.time())

def make(*target):
    pre_make()
    if Popen(("make", "--no-print-directory") + target).wait():
        sys.exit(1)
    post_make()

def show_command(cmd):
    from pipes import quote
    print "\n$", " ".join(map(quote, cmd))

def maybe_unlink(*paths):
    for path in paths:
        try:
            os.unlink(path)
        except EnvironmentError, e:
            if e.errno != errno.ENOENT:
                raise

COLORS = {"default": "\033[0m", "red": "\033[31m", "green": "\033[32m"}

def color(name, text):
    if options.color == "always" or (options.color == "auto" and os.isatty(1)):
        return COLORS[name] + text + COLORS["default"]
    return text

##################################################################
# Test runner
#

__all__ += ["TerminateTest", "Runner"]

class TerminateTest(Exception):
    pass

class Runner():
    def __init__(self, *default_monitors):
        self.__default_monitors = default_monitors

    def save_output(self, filename, data):
        with open(filename, "w") as out:
            print >> out, data,

    def run_test(self, binary):
        p = Popen(["stdbuf", "-o0", "-e0", binary], stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
        self.proc_output, _ = p.communicate()

    def match(self, *args, **kwargs):
        """Shortcut to call assert_lines_match on the most recent output."""

        def save_on_finish(fail):
            test = get_current_test()
            if test.parent:
                first_name = test.parent.func_name.replace("test_", "")
                second_name = test.title.strip()
                name = "%s.%s" % (first_name, second_name)
            else:
                name = test.func_name.replace("test_", "")
            filename = name + ".out"
            if fail:
                self.save_output(filename, self.proc_output)
                print "    Program output saved to %s" % filename
            elif os.path.exists(filename):
                os.remove(filename)
                print "    (Old %s failure log removed)" % filename

        get_current_test().on_finish.append(save_on_finish)
        assert_lines_match(self.proc_output, *args, **kwargs)
