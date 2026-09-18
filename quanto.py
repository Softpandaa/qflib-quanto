"""Quanto option pricing with qflib 1.1.0.

Produces every table and figure of the report. Build the pyqflib package first (see README),
then run `python quanto.py` from the repository root. Figures are written to latex/figures/.

Convention: X is the exchange rate in units of asset currency (CCY1) per unit of payoff currency (CCY2),
and RHO is the correlation between asset returns and returns of X.
"""
import itertools
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

import qflib as qf

# ---------------------------------------------------------------- parameters
PAY_CCY, ASSET_CCY = "USD", "EUR"
CURVE_TENORS = [1 / 12, 1 / 4, 1 / 2, 1, 2, 3, 5, 10]
R_D, R_F = 0.05, 0.04                  # flat payoff and asset currency rates, continuously compounded
S0, K, T = 100.0, 100.0, 1.0
Q = 0.02                               # dividend yield
SIG_S, SIG_X = 0.20, 0.10              # asset and FX volatilities
RHO = 0.30

PDE_PARAMS = {"NTIMESTEPS": 200, "NSPOTNODES": 200, "NSTDDEVS": 5, "THETA": 0.5}
MC_PARAMS = {"URNGTYPE": "MT19937", "PATHGENTYPE": "EULER"}
N_PATHS = 200_000

ZERO_STRIKES = [50.0, 25.0, 10.0, 5.0, 1.0, 0.1, 0.01, 1e-4, 1e-6]
PARITY_EXPIRIES = [0.5, 1.0, 2.0]
PARITY_STRIKES = [80.0, 100.0, 120.0]
PDE_GRIDS = [25, 50, 100, 200, 400, 800]
MC_PATH_COUNTS = [5_000, 10_000, 50_000, 100_000, 500_000, 1_000_000]
RHO_GRID = np.linspace(-0.9, 0.9, 19)

# three-asset basket, every asset in CCY1
SPOTS = np.array([100.0, 100.0, 100.0])
WEIGHTS = np.array([1 / 3, 1 / 3, 1 / 3])
DIVS = np.array([0.02, 0.02, 0.02])
VOLS = np.array([0.20, 0.25, 0.30])
FX_VOLS = np.array([0.10, 0.10, 0.10])
FX_CORRELS = np.array([0.30, 0.30, 0.30])
ASSET_CORR = 0.5
ASSET_CORR_GRID = np.linspace(-0.5, 0.95, 20)    # -1/(N-1) = -0.5 is the lower bound for a valid matrix
WORST_OF_CORR_GRID = np.linspace(-0.5, 0.95, 16)
WORST_OF_STRIKE, WORST_OF_FIX = 1.0, 0.25
HEATMAP_FX_CORRS = np.linspace(-0.5, 0.5, 7)
HEATMAP_ASSET_CORRS = np.linspace(-0.4, 0.9, 7)

FIG_DIR = Path(__file__).resolve().parent / "latex" / "figures"
CALL, PUT = 1, -1

# Okabe-Ito hues in fixed order, black for reference lines
BLUE, VERMILLION, ORANGE, GREEN = "#0072B2", "#D55E00", "#E69F00", "#009E73"
STYLE = {
    "font.family": "serif",
    "font.serif": ["cmr10"],
    "mathtext.fontset": "cm",
    "axes.formatter.use_mathtext": True,
    "axes.unicode_minus": False,
    "axes.grid": True,
    "grid.alpha": 0.3,
    "legend.frameon": False,
    "savefig.dpi": 200,
    "savefig.bbox": "tight",
}


# ------------------------------------------------------------------- helpers
def setup_market():
    qf.ycCreate(PAY_CCY, CURVE_TENORS, [R_D] * len(CURVE_TENORS), 0)
    qf.ycCreate(ASSET_CCY, CURVE_TENORS, [R_F] * len(CURVE_TENORS), 0)


def price_bs(payoff, strike, expiry, rho=RHO):
    return qf.qEuroBS(payoff, S0, strike, expiry, R_D, R_F, Q, SIG_S, SIG_X, rho)


def price_pde(payoff, strike, expiry, rho=RHO, params=PDE_PARAMS):
    return qf.qEuroBSPDE(payoff, strike, expiry, S0, PAY_CCY, Q, SIG_S, ASSET_CCY, SIG_X, rho, params)["Price"]


def price_mc(payoff, strike, expiry, rho=RHO, npaths=N_PATHS, spot=S0, div=Q, vol=SIG_S, fx_vol=SIG_X):
    return qf.qEuroBSMC(payoff, strike, expiry, spot, PAY_CCY, div, vol, ASSET_CCY, fx_vol, rho, MC_PARAMS, npaths)


def discounted_quanto_forward(expiry):
    """exp(-r_d T) F^Q with F^Q = S0 exp((r_f - q + rho sigma_S sigma_X) T)"""
    return S0 * np.exp((R_F - Q + RHO * SIG_S * SIG_X - R_D) * expiry)


def uniform_corr(n, rho):
    m = np.full((n, n), float(rho))
    np.fill_diagonal(m, 1.0)
    return m


def basket_market(n):
    return dict(discountcrv=PAY_CCY, growthcrvs=[ASSET_CCY] * n, mcparams=MC_PARAMS, npaths=N_PATHS)


def basket_call(asset_corr, fx_correls=FX_CORRELS):
    return qf.qBasketEuroBSMC(CALL, K, T, spots=SPOTS.tolist(), weights=WEIGHTS.tolist(),
                              divyields=DIVS, volatilities=VOLS, fxvols=FX_VOLS, correls=fx_correls,
                              correlmatrix=uniform_corr(len(SPOTS), asset_corr), **basket_market(len(SPOTS)))


def save(fig, name):
    fig.savefig(FIG_DIR / name)
    plt.close(fig)


# ------------------------------------------------------------------ sections
def zero_strike():
    target = discounted_quanto_forward(T)
    rows = [[k, price_bs(CALL, k, T), price_pde(CALL, k, T), price_mc(CALL, k, T)["Mean"]] for k in ZERO_STRIKES]
    df = pd.DataFrame(rows, columns=["K", "Closed form", "PDE", "Monte Carlo"])
    print(f"\nZero-strike limit, target exp(-r_d T) F^Q = {target:.6f}")
    print(df.to_string(index=False, float_format="%.4f", formatters={"K": "{:g}".format}))

    fig, ax = plt.subplots(figsize=(6.4, 4.0))
    ax.semilogx(df["K"], df["Closed form"], color=BLUE, ls="-", marker="o", label="Closed form")
    ax.semilogx(df["K"], df["PDE"], color=VERMILLION, ls="--", marker="s", mfc="none", label="PDE")
    ax.semilogx(df["K"], df["Monte Carlo"], color=ORANGE, ls=":", marker="x", label="Monte Carlo")
    ax.axhline(target, color="black", ls="-.", lw=1, label=r"$e^{-r_d T}F^Q$")
    ax.invert_xaxis()
    ax.set_xlabel("Strike $K$")
    ax.set_ylabel("Call price")
    ax.legend()
    save(fig, "zerostrike.png")
    return df, target


def parity():
    rows = []
    for expiry, strike in itertools.product(PARITY_EXPIRIES, PARITY_STRIKES):
        call, put = price_pde(CALL, strike, expiry), price_pde(PUT, strike, expiry)
        rhs = discounted_quanto_forward(expiry) - strike * np.exp(-R_D * expiry)
        rows.append([expiry, strike, call, put, abs(call - put - rhs)])
    df = pd.DataFrame(rows, columns=["T", "K", "Call", "Put", "|Residual|"])
    print("\nPut-call parity, PDE prices")
    print(df.to_string(index=False, formatters={"Call": "{:.6f}".format, "Put": "{:.6f}".format,
                                                 "|Residual|": "{:.2e}".format}))
    return df


def convergence():
    reference = price_bs(CALL, K, T)
    pde = np.array([price_pde(CALL, K, T, params={**PDE_PARAMS, "NTIMESTEPS": n, "NSPOTNODES": n})
                    for n in PDE_GRIDS])
    mc = [price_mc(CALL, K, T, npaths=n) for n in MC_PATH_COUNTS]
    mc_mean = np.array([r["Mean"] for r in mc])
    mc_se = np.array([r["StdErr"] for r in mc])
    pde_rate = np.polyfit(np.log(PDE_GRIDS), np.log(np.abs(pde - reference)), 1)[0]

    print(f"\nConvergence, closed form {reference:.6f}")
    for n, p in zip(PDE_GRIDS, pde):
        print(f"  PDE N = {n:4d}: {p:.6f}  |error| {abs(p - reference):.3e}")
    for n, m, s in zip(MC_PATH_COUNTS, mc_mean, mc_se):
        print(f"  MC  N = {n:7d}: {m:.5f}  SE {s:.5f}  |error| {abs(m - reference):.5f}")
    print(f"  PDE |error| ~ N^{pde_rate:.2f}")

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 3.8))
    ax1.semilogx(PDE_GRIDS, pde, color=VERMILLION, ls="--", marker="s", mfc="none", label="PDE")
    ax1.axhline(reference, color="black", ls="-.", lw=1, label="Closed form")
    ax1.set_xticks(PDE_GRIDS, [str(n) for n in PDE_GRIDS])
    ax1.minorticks_off()
    ax1.set_xlabel("Grid size $N$")
    ax1.set_ylabel("Call price")
    ax1.legend()
    ax2.errorbar(MC_PATH_COUNTS, mc_mean, yerr=1.96 * mc_se, color=ORANGE, ls=":", marker="x",
                 capsize=3, label="Monte Carlo, 95% interval")
    ax2.axhline(reference, color="black", ls="-.", lw=1, label="Closed form")
    ax2.set_xscale("log")
    ax2.set_xlabel("Number of paths")
    ax2.set_ylabel("Call price")
    ax2.legend()
    save(fig, "convergence.png")
    return reference, pde, mc_mean, mc_se, pde_rate


def asset_fx_correlation():
    out = {}
    for name, payoff in (("Call", CALL), ("Put", PUT)):
        mc = [price_mc(payoff, K, T, rho=r) for r in RHO_GRID]
        out[name] = dict(bs=np.array([price_bs(payoff, K, T, rho=r) for r in RHO_GRID]),
                         pde=np.array([price_pde(payoff, K, T, rho=r) for r in RHO_GRID]),
                         mc=np.array([m["Mean"] for m in mc]), se=np.array([m["StdErr"] for m in mc]))

    fig, ax = plt.subplots(figsize=(6.4, 4.0))
    for name in ("Call", "Put"):
        d = out[name]
        ax.plot(RHO_GRID, d["bs"], color=BLUE, ls="-", label="Closed form" if name == "Call" else None)
        ax.plot(RHO_GRID, d["pde"], color=VERMILLION, ls="none", marker="s", mfc="none",
                label="PDE" if name == "Call" else None)
        ax.errorbar(RHO_GRID, d["mc"], yerr=1.96 * d["se"], color=ORANGE, ls="none", marker="x", ms=5,
                    capsize=2, label="Monte Carlo, 95% interval" if name == "Call" else None)
        ax.annotate(name, (RHO_GRID[-1], d["bs"][-1]), xytext=(6, 0), textcoords="offset points", va="center")
    ax.set_xlabel(r"Asset-FX correlation $\rho$")
    ax.set_ylabel("Option price")
    ax.legend()
    save(fig, "quanto_asset_FX_corr.png")
    print(f"\nMaximum |PDE - closed form| over the rho grid: "
          f"{max(np.max(np.abs(out[n]['pde'] - out[n]['bs'])) for n in out):.2e}")
    return out


def basket():
    # one-asset basket reduces to the single-asset pricer
    one = qf.qBasketEuroBSMC(CALL, K, T, spots=[S0], weights=[1.0], divyields=np.array([Q]),
                             volatilities=np.array([SIG_S]), fxvols=np.array([SIG_X]), correls=np.array([RHO]),
                             correlmatrix=np.array([[1.0]]), **basket_market(1))
    single = price_mc(CALL, K, T)
    print(f"\nOne-asset basket {one['Mean']:.4f} ({one['StdErr']:.4f}), "
          f"single-asset MC {single['Mean']:.4f} ({single['StdErr']:.4f}), closed form {price_bs(CALL, K, T):.4f}")

    singles = [price_mc(CALL, K, T, spot=s, div=d, vol=v, fx_vol=fv, rho=c)
               for s, d, v, fv, c in zip(SPOTS, DIVS, VOLS, FX_VOLS, FX_CORRELS)]
    prices = np.array([r["Mean"] for r in singles])
    ses = np.array([r["StdErr"] for r in singles])
    average, average_se = float(WEIGHTS @ prices), float(np.sqrt(WEIGHTS ** 2 @ ses ** 2))
    b = basket_call(ASSET_CORR)
    for j, (p, s) in enumerate(zip(prices, ses)):
        print(f"  Asset {j + 1} (sigma_S = {VOLS[j]:.0%}): {p:.4f} ({s:.4f})")
    print(f"  Weighted average: {average:.4f} ({average_se:.4f})")
    print(f"  Basket (rho_jk = {ASSET_CORR}): {b['Mean']:.4f} ({b['StdErr']:.4f})")

    labels = [f"Asset {j + 1}\n$\\sigma_S={v:.0%}$".replace("%", r"\%") for j, v in enumerate(VOLS)]
    labels += ["Weighted\naverage", f"Basket\n$\\rho_{{jk}}={ASSET_CORR}$"]
    values = np.r_[prices, average, b["Mean"]]
    errors = np.r_[ses, average_se, b["StdErr"]]
    fig, ax = plt.subplots(figsize=(6.4, 4.0))
    bars = ax.bar(labels, values, yerr=1.96 * errors, capsize=3,
                  color=[BLUE] * len(VOLS) + [ORANGE, VERMILLION], edgecolor="black", linewidth=0.6)
    for bar, hatch in zip(bars, [""] * len(VOLS) + ["//", "xx"]):
        bar.set_hatch(hatch)
    ax.set_ylabel("Call price")
    ax.grid(axis="x", visible=False)
    save(fig, "diversification.png")
    return prices, ses, average, b


def asset_asset_correlation():
    basket_res = [basket_call(c) for c in ASSET_CORR_GRID]
    worst_res = [qf.qWorstofDigiBSMC(CALL, strike=WORST_OF_STRIKE, timetofix=WORST_OF_FIX, timetoexp=T,
                                     spots=SPOTS.tolist(), divyields=DIVS, volatilities=VOLS, fxvols=FX_VOLS,
                                     correls=FX_CORRELS, correlmatrix=uniform_corr(len(SPOTS), c),
                                     **basket_market(len(SPOTS)))
                 for c in WORST_OF_CORR_GRID]
    for res, grid, colour, marker, name, fname in (
            (basket_res, ASSET_CORR_GRID, BLUE, "s", "Basket call price", "basket_asset_asset.png"),
            (worst_res, WORST_OF_CORR_GRID, GREEN, "o", "Worst-of digital call price", "digital_asset_asset.png")):
        fig, ax = plt.subplots(figsize=(6.4, 4.0))
        ax.errorbar(grid, [r["Mean"] for r in res], yerr=[1.96 * r["StdErr"] for r in res],
                    color=colour, marker=marker, ms=4, capsize=2)
        ax.set_xlabel(r"Asset-asset correlation $\rho_{jk}$")
        ax.set_ylabel(name)
        save(fig, fname)
    return np.array([r["Mean"] for r in basket_res]), np.array([r["Mean"] for r in worst_res])


def joint_correlation():
    grid = np.array([[basket_call(a, np.full(len(SPOTS), f))["Mean"] for f in HEATMAP_FX_CORRS]
                     for a in HEATMAP_ASSET_CORRS])
    fig, ax = plt.subplots(figsize=(5.2, 4.0))
    im = ax.imshow(grid, origin="lower", aspect="auto", cmap="cividis",
                   extent=[HEATMAP_FX_CORRS[0], HEATMAP_FX_CORRS[-1], HEATMAP_ASSET_CORRS[0], HEATMAP_ASSET_CORRS[-1]])
    ax.grid(False)
    ax.set_xlabel(r"Asset-FX correlation $\rho_j$")
    ax.set_ylabel(r"Asset-asset correlation $\rho_{jk}$")
    fig.colorbar(im, ax=ax, label="Basket call price")
    save(fig, "basket_heatmap.png")
    return grid


def main():
    plt.rcParams.update(STYLE)
    FIG_DIR.mkdir(parents=True, exist_ok=True)
    setup_market()
    print(f"qflib version {qf.version()}")
    zero_strike()
    parity()
    convergence()
    asset_fx_correlation()
    basket()
    asset_asset_correlation()
    joint_correlation()


if __name__ == "__main__":
    main()
