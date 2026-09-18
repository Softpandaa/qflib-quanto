# Multi-Asset Quanto Pricing via C++ and Python Interface

This report documents qflib, a C++ library with a Python interface. It includes pricing European quanto options by three methods (a closed-form formula, Monte Carlo simulation and a finite-difference PDE) and pricing quanto basket calls and quanto worst-of digitals by Monte Carlo.

| Python function | Method | Pricer | Payoff |
|---|---|---|---|
| `qFwdPrice` | closed form | `quantoFwdPrice` | |
| `qEuroBS` | closed form | `quantoEuropeanOptionBS` | |
| `qEuroBSMC` | Monte Carlo | `BsMcQuantoPricer` | `EuropeanCallPut` |
| `qEuroBSPDE` | PDE | `Pde1DSolver` | `EuropeanCallPut` |
| `qBasketEuroBSMC` | Monte Carlo | `MultiAssetBsMcQuantoPricer` | `BasketEuropeanCallPut` |
| `qWorstofDigiBSMC` | Monte Carlo | `MultiAssetBsMcQuantoPricer` | `WorstOfDigitalCallPut` |

## Layout

```
qflib/            C++ library
pyqflib/          Python extension module and the qflib package
quanto.py         numbers and figures of the report
report.pdf
CMakeLists.txt
```

## Requirements

- Visual C++ 2026 on Windows, or gcc 14 on Linux
- CMake 3.25 or later and Ninja
- Python 3.12 with NumPy, pandas and matplotlib, in a Miniconda environment named `qfgb`
- Armadillo 15.2.3 in a folder named `armadillo-15.2.3` next to this repository, with its headers in `include` and its bundled LAPACK, BLAS and f2c libraries in `lib`

```
parent/
├── armadillo-15.2.3/
└── qflib-quanto/         this repository
```

`pyqflib/CMakeLists.txt` reads the Python headers and library from `%USERPROFILE%\Miniconda3\envs\qfgb` on Windows and `~/miniconda3/envs/qfgb` on Linux. 

## Reproducing

Windows.

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl
cmake --build build
pip install .\pyqflib
python quanto.py
```

Linux.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
pip install ./pyqflib
python quanto.py
```

The build writes the extension module into `pyqflib/qflib/`, and `pip install` copies it into the environment, so rerun `pip install` after every rebuild. `quanto.py` then prints the numbers quoted in the report and writes its seven figures to `latex/figures/`, which is created on first run and is not tracked.

### Credits

qflib 1.0.0 teaching library was written by Michael G. Sotiropoulos for QFGB 8960 Advanced C++ for Finance. 