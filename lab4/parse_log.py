from collections import namedtuple, defaultdict
import argparse
import sys
import os.path

parser = argparse.ArgumentParser(description='Grade lab')
parser.add_argument(
    '--grade_up_to',
    type=int,
    default=0,
    help='Will grade stages up to this one (included)',
)
parser.add_argument(
    '--grade_from', type=int, default=0, help='Will grade stages from this one'
)
parser.add_argument(
    '--output', type=str, default='', help='Will write the score to this file'
)
parser.add_argument(
    '--tmpfile', type=str, default='/tmp/log.txt', help='Will use this temporary file for execution log'
)
args = parser.parse_args()

PM_PREFIX = 'PM_DUMP'
VM_PREFIX = 'VM_DUMP'
PANIC_PREFIX = 'PANIC: Unexpected exception 52!'

# Per-page state for a page in physical and virtual memory.
PMEntry = namedtuple('PMEntry', ['owner', 'refcount'])
VMEntry = namedtuple('VMEntry', ['owner', 'refcount', 'ua', 'perm'])

# A bunch of constants.
PAGE_SIZE = 4096
KERNEL_LIMIT = 0x100000 // PAGE_SIZE
CGA_BLOCK = 0xB8000 // PAGE_SIZE
PTE_P = 0x1
PTE_W = 0x2
PTE_U = 0x4

# Offset into the start of kernel memory that should not be initially
# owned by processes.
PHYSICAL_UNALLOCATED_OFFSET = 17


class BColors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


class MemMapInstance:
    def __init__(self):
        self.pm_state = None
        self.vm_state = {}

    def set_pm_state(self, pm_state):
        assert self.pm_state == None
        self.pm_state = pm_state

    def add_vm_state(self, process_id, vm_state):
        assert not process_id in self.vm_state
        self.vm_state[process_id] = vm_state

    def IsKernelIsolated(self):
        # Returns true if all of kernel's addresses are not accessible
        # from any of the processes (except for the CGA block).
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i >= KERNEL_LIMIT:
                    break

                if i != CGA_BLOCK and entry.ua:
                    return False
        return True

    def AreProcessesIsolated(self):
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i < KERNEL_LIMIT:
                    # In kernel mem, checked by IsKernelIsolated
                    continue

                if entry.owner and entry.owner != process_id and entry.ua:
                    return False
        return True

    def IsVPAUsed(self):
        # Will check if virtual page allocation is used. Will simply
        # look at the owners of physical pages and make sure that at
        # least one physical page outside the address range of any
        # process is owned by a process.
        offset = PHYSICAL_UNALLOCATED_OFFSET
        for i, entry in enumerate(self.pm_state):
            if i < offset:
                continue

            if i >= KERNEL_LIMIT:
                return False
            if entry.owner and entry.owner <= 4:
                return True

        # Unreachable
        return False

    def AreProcessAddressesOverlapping(self):
        # Will check if there is at least one process that occupies
        # more than 64 pages of virtual memory.
        occupancy = defaultdict(int)
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i < KERNEL_LIMIT:
                    # In kernel mem.
                    continue

                if entry.owner and entry.owner == process_id and entry.ua:
                    occupancy[process_id] += 1
        return any(v > 64 for _, v in list(occupancy.items()))

    def CheckOwnershipInvariant(self):
        # Checks that all pages in the virtual address space of the
        # process are accessible by the process if they are owned by
        # it.
        for process_id, entries in list(self.vm_state.items()):
            for i, entry in enumerate(entries):
                if i < KERNEL_LIMIT:
                    # In kernel mem.
                    continue

                if entry.owner and entry.owner == process_id and not entry.ua:
                    return False
        return True

    def CheckCGAAccessInvariant(self):
        # The CGA block should be accessible by all processes.
        for process_id, entries in list(self.vm_state.items()):
            if not entries[CGA_BLOCK].ua:
                return False
        return True

    def IsForkOK(self):
        # Checks to see if all processes' virtual address space starts
        # at the same place and, if the page is writable, that it's
        # owned by the given process. (Handles shared, read-only pages.)
        for process_id, entries in list(self.vm_state.items()):
            entry = entries[KERNEL_LIMIT]
            if not entry.owner or \
                   (entry.owner != process_id and entry.perm & PTE_W):
                return False
        return True


class MemMapTimeline:
    def __init__(self, log_file):
        # Parsed data. For each tick will store an instance of
        # MemMapInstance.
        self.mem_data = defaultdict(MemMapInstance)
        self.panic = False

        with open(log_file) as f:
            lines = f.readlines()

        for line in lines:
            if line.startswith(PM_PREFIX):
                line_split = list(filter(bool, line.split(' ')))

                # The label, the tick 'timestamp' and the newline at
                # the end, 512 pages in physical memory.
                assert len(line_split) - 3 == 512 * 2
                tick = int(line_split[1])

                data = []
                for i in range(512):
                    owner, refcount = line_split[2 * i + 2 : 2 * i + 4]
                    data.append(PMEntry(int(owner), int(refcount)))
                self.mem_data[tick].set_pm_state(data)
            elif line.startswith(VM_PREFIX):
                line_split = list(filter(bool, line.split(' ')))

                # The label, the id, the tick 'timestamp' and the
                # newline at the end, 768 pages in virtual memory.
                assert len(line_split) - 4 == 768 * 3
                process_id = int(line_split[1])
                tick = int(line_split[2])

                data = []
                for i in range(768):
                    owner, refcount, perm = line_split[3 * i + 3 : 3 * i + 6]
                    data.append(VMEntry(int(owner), int(refcount),
                                        int(perm) & PTE_U,
                                        int(perm)))
                self.mem_data[tick].add_vm_state(process_id, data)
            elif line.startswith(PANIC_PREFIX):
                self.panic = True

    def PerformCheck(self, check_function, message, start_at_tick):
        res = 0
        tick = 0
        for tick, instance in sorted(self.mem_data.items()):
            if tick < start_at_tick:
                continue

            func = getattr(instance, check_function)
            if func():
                res = 1
            else:
                return (0, tick)

        return (res, tick)

    def PerformChecks(self, grade_from, up_to):
        all_functions = [
            ('IsKernelIsolated', 'Kernel isolation', 0),
            ('AreProcessesIsolated', 'Process isolation', 0),
            ('IsVPAUsed', 'Virtual page allocation', 500),
            ('AreProcessAddressesOverlapping', 'Overlapping address spaces', 800),
            ('IsForkOK', 'Fork', 800),
        ]
        to_perform = all_functions[grade_from:up_to]
        total = 0
        for function_name, message, start_at_tick in to_perform:
            res, tick = self.PerformCheck(function_name, message, start_at_tick)
            if res:
                print('{}{} passed{}'.format(BColors.OKGREEN, message, BColors.ENDC))
            else:
                print(
                    '{}{} failed at tick {}{}'.format(
                        BColors.FAIL, message, tick, BColors.ENDC
                    )
                )
            total += res
        if args.output != '':
            with open(args.output, 'w') as f:
                f.write(str(total))
        else:
            print('Total score: {}/5'.format(total))

    def CheckInvariants(self, expected_max_tick):
        if self.mem_data:
            max_tick = max(self.mem_data.keys())
            if not self.panic and expected_max_tick != max_tick:
                print(
                    'Kernel prematurely sys.exited, expected to last until tick',
                    expected_max_tick,
                    'but last recorded tick is',
                    max_tick,
                )
                sys.exit(1)

        for tick, instance in sorted(self.mem_data.items()):
            if not instance.CheckOwnershipInvariant():
                print(
                    'At least one non-kernel page owned by a process is not accessible by it at tick ',
                    tick,
                )
                sys.exit(1)

            if not instance.CheckCGAAccessInvariant():
                print(
                    'At least one process does not have access to the CGA console at tick',
                    tick,
                )
                sys.exit(1)

if not os.path.isfile(args.tmpfile):
    print('No output from run, did code compile?')
    sys.exit(1)

timeline = MemMapTimeline(args.tmpfile)
timeline.CheckInvariants(900)
timeline.PerformChecks(args.grade_from, args.grade_up_to)
