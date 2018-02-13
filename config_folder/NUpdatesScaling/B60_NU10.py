{   "bin_fn"                    : "build/GluonicLQCD",
    "runName"                   : "B60_NU10",
    "N"                         : 24,
    "NT"                        : 48,
    "subDims"                   : [6, 6, 6, 6],
    "beta"                      : 6.0,
    "NCf"                       : 10,
    "NCor"                      : 600,
    "NTherm"                    : 0,
    "NFlows"                    : 0,
    "NUpdates"                  : 10,
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
    "cpu_approx_runtime_hr"     : 0,
    "cpu_approx_runtime_min"    : 45,
    "cpu_memory"                : 3800,
    "account_name"              : "nn2977k"}
