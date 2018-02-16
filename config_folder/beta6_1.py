{   "bin_fn"                    : "build/GluonicLQCD",
    "runName"                   : "prodRunBeta6_1",
    "N"                         : 28,
    "NT"                        : 56,
    "subDims"                   : [7, 7, 7, 7],
    "beta"                      : 6.1,
    "NCf"                       : 500,
    "NCor"                      : 600,
    "NTherm"                    : 20000,
    "NFlows"                    : 1000,
    "NUpdates"                  : 30,
    "storeCfgs"                 : True,
    "storeThermCfgs"            : False,
    "verboseRun"                : False,
    "hotStart"                  : False,
    "expFunc"                   : "morningstar", # options: luscher, taylor2, taylor4
    "observables"               : ["plaq"], # Optional: topologicalCharge, energyDensity
    "flowObservables"           : ["plaq","topc","energy"], # Optional: topologicalCharge, energyDensity
    "uTest"                     : False,
    "uTestVerbose"              : False,
    "SU3Eps"                    : 0.24,
    "flowEpsilon"               : 0.01,
    "metropolisSeed"            : 0,
    "randomMatrixSeed"          : 0,
    "threads"                   : 512,
    "cpu_approx_runtime_hr"     : 65, # FLOW TIME: 34.2 hours
    "cpu_approx_runtime_min"    : 0,
    "cpu_memory"                : 3800,
    "account_name"              : "nn2977k"}