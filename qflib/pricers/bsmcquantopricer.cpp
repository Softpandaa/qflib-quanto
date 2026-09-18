/**
@file  bsmcquantopricer.cpp
@brief Implementation of the BsMcQuantoPricer class
*/

#include <qflib/pricers/bsmcquantopricer.hpp>

BEGIN_NAMESPACE(qf)

BsMcQuantoPricer::BsMcQuantoPricer(SPtrProduct prod,
                                   SPtrYieldCurve discountYieldCurve,
                                   SPtrYieldCurve growthYieldCurve,
                                   double divYield,
                                   SPtrVolatilityTermStructure assetVol,
                                   SPtrVolatilityTermStructure fxVol,
                                   double correl,
                                   double spot,
                                   McParams mcparams)
: BsMcPricer(prod, discountYieldCurve, divYield, assetVol, spot, mcparams)
{
  QF_ASSERT(correl >= -1.0 && correl <= 1.0, "the asset-FX correlation must lie in [-1, 1]!");

  // the base ctor sets the discount factors and stdevs;
  // the drifts are replaced by the quanto drifts on the asset currency curve
  Vector const& fixtimes = prod->fixTimes();
  double t1 = 0.0;
  for (size_t i = 0; i < fixtimes.size(); ++i) {
    double t2 = fixtimes[i];
    double dt = t2 - t1;
    double fwdAssetVol = assetVol->fwdVol(t1, t2);
    double fwdFxVol = fxVol->fwdVol(t1, t2);
    double var = fwdAssetVol * fwdAssetVol * dt;
    drifts_[i] = (growthYieldCurve->fwdRate(t1, t2) - divYield + correl * fwdAssetVol * fwdFxVol) * dt - 0.5 * var;
    t1 = t2;
  }
}

END_NAMESPACE(qf)
