/**
@file  multiassetbsmcquantopricer.hpp
@brief Multiasset Monte Carlo quanto pricer in the Black-Scholes model
*/

#pragma once

#include <qflib/pricers/multiassetbsmcpricer.hpp>
#include <vector>

BEGIN_NAMESPACE(qf)

/** Multiasset Monte Carlo pricer with every asset quanto-ed into one payoff currency.
    Asset j drifts at its own currency rate less its dividend yield plus assetFXCorrels[j] * volatilities[j] * fxVols[j],
    and cash flows are discounted on the payoff currency curve.
    fxVols[j] and assetFXCorrels[j] refer to the exchange rate X, in units of asset currency per unit of payoff currency of asset j.
    No other FX correlation enters the price of a payoff written on the asset prices alone.
*/
class MultiAssetBsMcQuantoPricer : public MultiAssetBsMcPricer
{
public:
  /** Initializing ctor */
  MultiAssetBsMcQuantoPricer(SPtrProduct prod,
                             SPtrYieldCurve discountYieldCurve,
                             std::vector<SPtrYieldCurve> const& growthYieldCurves,
                             Vector const& divYields,
                             Vector const& volatilities,
                             Vector const& fxVols,
                             Vector const& assetFXCorrels,
                             Vector const& spots,
                             Matrix const& correlMatrix,
                             McParams const& mcparams);
};

END_NAMESPACE(qf)
