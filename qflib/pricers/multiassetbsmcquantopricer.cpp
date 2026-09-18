/**
@file  multiassetbsmcquantopricer.cpp
@brief Implementation of the MultiAssetBsMcQuantoPricer class
*/

#include <qflib/pricers/multiassetbsmcquantopricer.hpp>

BEGIN_NAMESPACE(qf)

MultiAssetBsMcQuantoPricer::MultiAssetBsMcQuantoPricer(SPtrProduct prod,
                                                       SPtrYieldCurve discountYieldCurve,
                                                       std::vector<SPtrYieldCurve> const& growthYieldCurves,
                                                       Vector const& divYields,
                                                       Vector const& volatilities,
                                                       Vector const& fxVols,
                                                       Vector const& assetFXCorrels,
                                                       Vector const& spots,
                                                       Matrix const& correlMatrix,
                                                       McParams const& mcparams)
: MultiAssetBsMcPricer(prod, discountYieldCurve, divYields, volatilities, spots, correlMatrix, mcparams)
{
  size_t nassets = prod->nAssets();
  QF_ASSERT(growthYieldCurves.size() == nassets, "need as many growth yield curves as product assets!");
  QF_ASSERT(fxVols.size() == nassets, "need as many FX volatilities as product assets!");
  QF_ASSERT(assetFXCorrels.size() == nassets, "need as many asset-FX correlations as product assets!");

  // the base ctor sets the discount factors and stdevs;
  // the drifts are replaced by the quanto drifts on each asset currency curve
  Vector const& fixtimes = prod->fixTimes();
  for (size_t j = 0; j < nassets; ++j) {
    double t1 = 0.0;
    double quantoCoef = assetFXCorrels[j] * fxVols[j];
    for (size_t i = 0; i < fixtimes.size(); ++i) {
      double t2 = fixtimes[i];
      double var = vols_[j] * vols_[j] * (t2 - t1);
      double fwdrate = growthYieldCurves[j]->fwdRate(t1, t2);
      double quantoAdj = quantoCoef * vols_[j];
      drifts_(i, j) = (fwdrate - divylds_[j] + quantoAdj) * (t2 - t1) - 0.5 * var;
      t1 = t2;
    }
  }
}

END_NAMESPACE(qf)
