
qht_bench = executable('qht-bench',
                       sources: 'qht-bench.c',
                       dependencies: [qemuutil, print_stub])

qtree_bench = executable('qtree-bench',
                         sources: 'qtree-bench.c',
                         dependencies: [qemuutil, print_stub])

executable('atomic_add-bench',
           sources: files('atomic_add-bench.c'),
           dependencies: [qemuutil, print_stub],
           build_by_default: false)

executable('atomic64-bench',
           sources: files('atomic64-bench.c'),
           dependencies: [qemuutil, print_stub],
           build_by_default: false)

benchs = {}

if have_block
  benchs += {
     'benchmark-crypto-hash': [crypto],
     'benchmark-crypto-hmac': [crypto],
     'benchmark-crypto-cipher': [crypto],
     'benchmark-crypto-akcipher': [crypto],
  }
endif

foreach bench_name, deps: benchs
  exe = executable(bench_name, bench_name + '.c',
                   dependencies: [qemuutil, print_stub] + deps)
  benchmark(bench_name, exe,
            args: ['--tap', '-k'],
            protocol: 'tap',
            timeout: 0,
            suite: ['speed'])
endforeach
