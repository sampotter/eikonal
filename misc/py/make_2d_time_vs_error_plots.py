#!/usr/bin/env python3

BUILD_TYPE='Release'

import os
import sys
if '../../build/%s' % BUILD_TYPE not in sys.path:
    sys.path.insert(0, os.path.abspath('../../build/%s' % BUILD_TYPE))

import common
import itertools
import matplotlib.pyplot as plt
import numpy as np
import pyeikonal as eik
import speedfuncs
import time

from matplotlib import rc

rc('text', usetex=True)
rc('font', **{'family': 'serif', 'serif': ['Computer Modern']})

norm = np.linalg.norm

plt.ion()
plt.style.use('bmh')

Npows = np.arange(3, 15)
N = 2**Npows + 1

use_local_factoring = True
r_fac = 0.1

Slows = [speedfuncs.s1, speedfuncs.s2, speedfuncs.s3, speedfuncs.s4]
Solns = {
    speedfuncs.s1: speedfuncs.f1,
    speedfuncs.s2: speedfuncs.f2,
    speedfuncs.s3: speedfuncs.f3,
    speedfuncs.s4: speedfuncs.f4
}
Olims = [eik.Olim4Mid0, eik.Olim4Mid1, eik.Olim4Rect,
         eik.Olim8Mid0, eik.Olim8Mid1, eik.Olim8Rect]

Slows_by_Olims = list(itertools.product(Slows, Olims))

T = {(slow, Olim): np.empty(N.shape) for slow, Olim in Slows_by_Olims}
E2 = {(slow, Olim): np.empty(N.shape) for slow, Olim in Slows_by_Olims}
EI = {(slow, Olim): np.empty(N.shape) for slow, Olim in Slows_by_Olims}

ntrials = 2

current_slow, current_Olim, current_n = None, None, None
for (slow, Olim), (ind, n) in itertools.product(Slows_by_Olims, enumerate(N)):
    if slow != current_slow:
        print(speedfuncs.get_slowness_func_name(slow))
        current_slow = slow
    if Olim != current_Olim:
        print('* %s' % str(Olim))
        current_Olim = Olim
    if n != current_n:
        print('  - %d' % n)
        current_n = n

    # get timings

    h = 2/(n-1)
    i0, j0 = n//2, n//2
    L = np.linspace(-1, 1, n)
    x, y = np.meshgrid(L, L)
    R = np.sqrt(x**2 + y**2)
    I, J = np.where(R < r_fac)
    u, S = speedfuncs.get_fields(Solns[slow], slow, x, y)

    t = np.inf

    for _ in range(ntrials):

        o = Olim(S, h)
        if use_local_factoring:
            for i, j in zip(I, J):
                o.set_node_fac_parent(i, j, i0, j0)
        o.addBoundaryNode(i0, j0)

        t0 = time.perf_counter()
        o.run()
        t = min(t, time.perf_counter() - t0)

    T[slow, Olim][ind] = t

    # get errors

    U = np.array([[o.getValue(i, j) for j in range(n)] for i in range(n)])
    E2[slow, Olim][ind] = norm(u - U, 'fro')/norm(u, 'fro')
    EI[slow, Olim][ind] = norm(u - U, np.inf)/norm(u, np.inf)

# make plots
 
marker = '*'
linestyles = ['solid', 'dashed', 'dotted']
colors = ['#1f77b4', '#d62728']

# time vs error

fig, axes = plt.subplots(4, 2, sharex=True, sharey='row', figsize=(8, 7))

axes[0, 0].set_title('Relative $\ell_2$ Error')
axes[0, 1].set_title('Relative $\ell_\infty$ Error')

for row, slow in enumerate(Slows):
    for ind, Olim in enumerate(Olims):
        name = common.get_marcher_name(Olim)
        axes[row, 0].loglog(
            T[slow, Olim], E2[slow, Olim], marker=marker, color=colors[ind//3],
            linestyle=linestyles[ind % 3], linewidth=1, label=name)
        axes[row, 0].text(0.95, 0.9, '$\\texttt{s%d}$' % (row + 1),
                          transform=axes[row, 0].transAxes,
                          horizontalalignment='center',
                          verticalalignment='center')
        axes[row, 1].loglog(
            T[slow, Olim], EI[slow, Olim], marker=marker, color=colors[ind//3],
            linestyle=linestyles[ind % 3], linewidth=1, label=name)
        axes[row, 1].text(0.95, 0.9, '$\\texttt{s%d}$' % (row + 1),
                          transform=axes[row, 1].transAxes,
                          horizontalalignment='center',
                          verticalalignment='center')

axes[-1, 0].set_xlabel('Time (s.)')    
axes[-1, 1].set_xlabel('Time (s.)')    

handles, labels = axes[-1, -1].get_legend_handles_labels()
    
fig.legend(handles, labels, loc='upper center', ncol=3)
fig.tight_layout()
fig.subplots_adjust(0.05, 0.075, 0.995, 0.8625)
fig.savefig('time_vs_error_2d.eps')
fig.show()

# size vs error

fig, axes = plt.subplots(4, 2, sharex=True, sharey='row', figsize=(8, 7))

axes[0, 0].set_title('Relative $\ell_2$ Error')
axes[0, 1].set_title('Relative $\ell_\infty$ Error')

for row, slow in enumerate(Slows):
    for ind, Olim in enumerate(Olims):
        name = common.get_marcher_name(Olim)
        axes[row, 0].loglog(
            N, E2[slow, Olim], marker=marker, color=colors[ind//3],
            linestyle=linestyles[ind % 3], linewidth=1, label=name)
        axes[row, 0].text(0.95, 0.9, '$\\texttt{s%d}$' % (row + 1),
                          transform=axes[row, 0].transAxes,
                          horizontalalignment='center',
                          verticalalignment='center')
        axes[row, 0].minorticks_off()
        axes[row, 1].loglog(
            N, EI[slow, Olim], marker=marker, color=colors[ind//3],
            linestyle=linestyles[ind % 3], linewidth=1, label=name)
        axes[row, 1].text(0.95, 0.9, '$\\texttt{s%d}$' % (row + 1),
                          transform=axes[row, 1].transAxes,
                          horizontalalignment='center',
                          verticalalignment='center')
        axes[row, 1].minorticks_off()

axes[-1, 0].set_xlabel('$N$')

xticklabels = ['$2^%d + 1$' % p for p in Npows]
axes[-1, 1].set_xlabel('$N$')    
axes[-1, 1].set_xticks(N[::2])
axes[-1, 1].set_xticklabels(xticklabels[::2])

handles, labels = axes[-1, -1].get_legend_handles_labels()
    
fig.legend(handles, labels, loc='upper center', ncol=3)
fig.tight_layout()
fig.subplots_adjust(0.05, 0.075, 0.995, 0.8625)
fig.savefig('size_vs_error_2d.eps')
fig.show()
