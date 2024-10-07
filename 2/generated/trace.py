import itertools
import collections
from collections import Counter

def sliding_window(iterable, n):
    "Collect data into overlapping fixed-length chunks or blocks."
    # sliding_window('ABCDEFG', 4) → ABCD BCDE CDEF DEFG
    iterator = iter(iterable)
    window = collections.deque(itertools.islice(iterator, n - 1), maxlen=n)
    for x in iterator:
        window.append(x)
        yield tuple(window)

def stat_window(lines, out_filename, pat_len):
    counter = Counter()
    all = 0
    for pat in sliding_window(lines, pat_len):
        counter[tuple(pat)] += 1
        all += 1

    with open(out_filename, 'w') as outfile:
        outfile.write('pattern,N,%\n')
        for pat, n in counter.most_common()[:50]:
            outfile.write(f'{';'.join(x.strip() for x in pat)},{n},{int(n/all*100)}\n')
    return counter

with open('simple_trace.txt', 'r') as infile:
    for pat_len in range(1, 6):
        infile.seek(0)
        stat_window(infile, f'simple_trace_stats_{pat_len}.csv', pat_len)

with open('use_trace.txt', 'r') as infile:
    stat_window((x.rstrip(';\n') for x in infile), f'use_trace_stats.csv', 1)
