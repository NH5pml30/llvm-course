import itertools
import collections
from collections import Counter
import matplotlib.pyplot as plt

def sliding_window(iterable, n):
    "Collect data into overlapping fixed-length chunks or blocks."
    # sliding_window('ABCDEFG', 4) → ABCD BCDE CDEF DEFG
    iterator = iter(iterable)
    window = collections.deque(itertools.islice(iterator, n - 1), maxlen=n)
    for x in iterator:
        window.append(x)
        yield tuple(window)

def compute_stat_row(n_all, counter_entry):
    pat, n = counter_entry
    return (';'.join(x.strip() for x in pat), n, int(n / n_all * 100))

def stat_window(lines, out_filename, pat_len):
    counter = Counter()
    all = 0
    for pat in sliding_window(lines, pat_len):
        counter[tuple(pat)] += 1
        all += 1

    with open(f'{out_filename}_{pat_len}.csv', 'w') as outfile:
        outfile.write('pattern,N,%\n')
        for pat, n in counter.most_common()[:50]:
            r1, r2, r3 = compute_stat_row(all, (pat, n))
            outfile.write(f'{r1},{r2},{r3}\n')

    fig, ax = plt.subplots()
    barxs = []
    barys = []
    for pat, n in counter.most_common()[:30]:
        r1, r2, r3 = compute_stat_row(all, (pat, n))
        barxs.append(r1)
        barys.append(r2)
    ax.bar(barxs, barys)
    # ax.tick_params(axis='x', labelrotation=45)
    plt.setp(ax.get_xticklabels(), rotation=45, ha='right')
    ax.set_xlabel('pattern')
    plt.tight_layout()
    fig.savefig(f'{out_filename}_{pat_len}.png')
    plt.close(fig)

    return counter

with open('simple_trace.txt', 'r') as infile:
    for pat_len in range(1, 6):
        infile.seek(0)
        stat_window(infile, f'simple_trace_stats', pat_len)

with open('use_trace.txt', 'r') as infile:
    stat_window((x.rstrip(';\n') for x in infile), f'use_trace_stats', 1)
