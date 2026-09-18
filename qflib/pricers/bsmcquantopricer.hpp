/**
@file  bsmcquantopricer.hpp
@brief Monte Carlo quanto pricer in the Black-Scholes model
*/

#pragma once

#include <qflib/pricers/bsmcpricer.hpp>

BEGIN_NAMESPACE(qf)

/** Monte Carlo pricer for a single asset quanto-ed into the payoff currency.
    The asset drifts at the asset currency rate less the dividend yield plus correl * assetVol * fxVol,
    and cash flows are discounted on the payoff currency curve.
    fxVol and correl refer to the exchange rate X, in units of asset currency per unit of payoff currency.
    The quanto term uses the product of the forward volatilities over each fixing interval,
    which is exact when both volatilities are flat over that interval.
*/
class BsMcQuantoPricer : public BsMcPricer
{
public:
  /** Initializing ctor */
  BsMcQuantoPricer(SPtrProduct prod,
                   SPtrYieldCurve discountYieldCurve,
                   SPtrYieldCurve growthYieldCurve,
                   double divYield,
                   SPtrVolatilityTermStructure assetVol,
                   SPtrVolatilityTermStructure fxVol,
                   double correl,
                   double spot,
                   McParams mcparams);
};

END_NAMESPACE(qf)
