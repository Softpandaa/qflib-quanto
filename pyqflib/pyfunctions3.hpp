/**
@file  pyfunctions3.hpp
@brief Implementation of Python callable functions
*/
#include <pyqflib/pyutils.hpp>

#include <qflib/defines.hpp>
#include <qflib/products/europeancallput.hpp>
#include <qflib/products/digitalcallput.hpp>
#include <qflib/products/asianbasketcallput.hpp>
#include <qflib/products/worstofdigitalcallput.hpp>
#include <qflib/products/basketeuropeancallput.hpp>
#include <qflib/pricers/bsmcpricer.hpp>
#include <qflib/pricers/bsmcquantopricer.hpp>
#include <qflib/pricers/multiassetbsmcpricer.hpp>
#include <qflib/pricers/multiassetbsmcquantopricer.hpp>
#include <qflib/math/stats/meanvarcalculator.hpp>
#include <qflib/math/random/rng.hpp>

using namespace std;

static
PyObject*  pyQfEuroBSMC(PyObject* pyDummy, PyObject* pyArgs)
{
PY_BEGIN;

  PyObject* pyPayoffType(NULL);
  PyObject* pySpot(NULL);
  PyObject* pyStrike(NULL);
  PyObject* pyTimeToExp(NULL);
  PyObject* pyDiscountCrv(NULL);
  PyObject* pyDivYield(NULL);
  PyObject* pyVolatility(NULL);
  PyObject* pyMcParams(NULL);
  PyObject* pyNPaths(NULL);

  if (!PyArg_ParseTuple(pyArgs, "OOOOOOOOO", &pyPayoffType, &pyStrike, &pyTimeToExp, 
    &pySpot, &pyDiscountCrv, &pyDivYield, &pyVolatility, &pyMcParams, &pyNPaths))
    return NULL;

  int payoffType = asInt(pyPayoffType);
  double spot = asDouble(pySpot);
  double strike = asDouble(pyStrike);
  double timeToExp = asDouble(pyTimeToExp);

  std::string name = asString(pyDiscountCrv);
  qf::SPtrYieldCurve spyc = qf::market().yieldCurves().get(name);
  QF_ASSERT(spyc, "error: yield curve " + name + " not found");

  double divYield = asDouble(pyDivYield);
    // read volatility, either number or term structure
  qf::SPtrVolatilityTermStructure spvol;
  if (isString(pyVolatility)) { // check if input is an object name
    std::string volname = asString(pyVolatility);
    spvol = qf::market().volatilities().get(volname);
  }
  else { // assume real number
  double vol = asDouble(pyVolatility);
    spvol.reset(new qf::VolatilityTermStructure(&timeToExp, &timeToExp + 1,
      &vol, &vol + 1));
  }
 
  // read the MC parameters
  qf::McParams mcparams = asMcParams(pyMcParams);
  // read the number of paths
  unsigned long npaths = asInt(pyNPaths);

  // create the product
  qf::SPtrProduct spprod(new qf::EuropeanCallPut(payoffType, strike, timeToExp));
  // create the pricer
  qf::BsMcPricer bsmcpricer(spprod, spyc, divYield, spvol, spot, mcparams);
  // create the statistics calculator
  qf::MeanVarCalculator<double *> sc(bsmcpricer.nVariables());
  // run the simulation
  bsmcpricer.simulate(sc, npaths);
  // collect results
  qf::Matrix const& results = sc.results();
  // read out results
  size_t nsamples = sc.nSamples();
  double mean = results(0, 0);
  double stderror = results(1, 0);
  stderror = std::sqrt(stderror / nsamples);

  // write mean and standard error into a Python dictionary
  PyObject* ret = PyDict_New();
  int ok = PyDict_SetItem(ret, asPyScalar("Mean"), asPyScalar(mean));
  PyDict_SetItem(ret, asPyScalar("StdErr"), asPyScalar(stderror));
  return ret;

PY_END;
}


static
PyObject*  pyQfDigiBSMC(PyObject* pyDummy, PyObject* pyArgs)
{
PY_BEGIN;

  PyObject* pyPayoffType(NULL);
  PyObject* pySpot(NULL);
  PyObject* pyStrike(NULL);
  PyObject* pyTimeToExp(NULL);
  PyObject* pyDiscountCrv(NULL);
  PyObject* pyDivYield(NULL);
  PyObject* pyVolatility(NULL);
  PyObject* pyMcParams(NULL);
  PyObject* pyNPaths(NULL);

  if (!PyArg_ParseTuple(pyArgs, "OOOOOOOOO", &pyPayoffType, &pyStrike, &pyTimeToExp, 
    &pySpot, &pyDiscountCrv, &pyDivYield, &pyVolatility, &pyMcParams, &pyNPaths))
    return NULL;

  int payoffType = asInt(pyPayoffType);
  double spot = asDouble(pySpot);
  double strike = asDouble(pyStrike);
  double timeToExp = asDouble(pyTimeToExp);

  std::string name = asString(pyDiscountCrv);
  qf::SPtrYieldCurve spyc = qf::market().yieldCurves().get(name);
  QF_ASSERT(spyc, "error: yield curve " + name + " not found");

  double divYield = asDouble(pyDivYield);
    // read volatility, either number or term structure
  qf::SPtrVolatilityTermStructure spvol;
  if (isString(pyVolatility)) { // check if input is an object name
    std::string volname = asString(pyVolatility);
    spvol = qf::market().volatilities().get(volname);
  }
  else { // assume real number
  double vol = asDouble(pyVolatility);
    spvol.reset(new qf::VolatilityTermStructure(&timeToExp, &timeToExp + 1,
      &vol, &vol + 1));
  }
 
  // read the MC parameters
  qf::McParams mcparams = asMcParams(pyMcParams);
  // read the number of paths
  unsigned long npaths = asInt(pyNPaths);

  // create the product
  qf::SPtrProduct spprod(new qf::DigitalCallPut(payoffType, strike, timeToExp));
  // create the pricer
  qf::BsMcPricer bsmcpricer(spprod, spyc, divYield, spvol, spot, mcparams);
  // create the statistics calculator
  qf::MeanVarCalculator<double *> sc(bsmcpricer.nVariables());
  // run the simulation
  bsmcpricer.simulate(sc, npaths);
  // collect results
  qf::Matrix const& results = sc.results();
  // read out results
  size_t nsamples = sc.nSamples();
  double mean = results(0, 0);
  double stderror = results(1, 0);
  stderror = std::sqrt(stderror / nsamples);

  // write mean and standard error into a Python dictionary
  PyObject* ret = PyDict_New();
  int ok = PyDict_SetItem(ret, asPyScalar("Mean"), asPyScalar(mean));
  PyDict_SetItem(ret, asPyScalar("StdErr"), asPyScalar(stderror));
  return ret;

PY_END;
}

static
PyObject*  pyQfAsianBasketBSMC(PyObject* pyDummy, PyObject* pyArgs)
{
PY_BEGIN;

  PyObject* pyPayoffType(NULL);
  PyObject* pyStrike(NULL);
  PyObject* pyFixingTimes(NULL);
  PyObject* pyAssetQuantities(NULL);
  PyObject* pySpots(NULL);
  PyObject* pyDiscountCrv(NULL);
  PyObject* pyDivYields(NULL);
  PyObject* pyVolatilities(NULL);
  PyObject* pyCorrelMatrix(NULL);
  PyObject* pyMcParams(NULL);
  PyObject* pyNPaths(NULL);

  if (!PyArg_ParseTuple(pyArgs, "OOOOOOOOOOO", &pyPayoffType, &pyStrike, &pyFixingTimes, 
    &pyAssetQuantities, &pySpots, &pyDiscountCrv, &pyDivYields, &pyVolatilities, 
    &pyCorrelMatrix, &pyMcParams, &pyNPaths))
    return NULL;

  int payoffType = asInt(pyPayoffType);
  double strike = asDouble(pyStrike);
  qf::Vector fixingTimes = asVector(pyFixingTimes);
  qf::Vector assetQuantities = asVector(pyAssetQuantities);
  qf::Vector spots = asVector(pySpots);

  std::string name = asString(pyDiscountCrv);
  qf::SPtrYieldCurve spyc = qf::market().yieldCurves().get(name);
  QF_ASSERT(spyc, "error: yield curve " + name + " not found");

  qf::Vector divYields = asVector(pyDivYields);
  qf::Vector vols = asVector(pyVolatilities);
  qf::Matrix correlMat = asMatrix(pyCorrelMatrix);
  // read the MC parameters
  qf::McParams mcparams = asMcParams(pyMcParams);
  // read the number of paths
  unsigned long npaths = asInt(pyNPaths);

  // create the product
  qf::SPtrProduct spprod(new qf::AsianBasketCallPut(payoffType, strike, fixingTimes, assetQuantities));
  // create the pricer
  qf::MultiAssetBsMcPricer bsmcpricer(spprod, spyc, divYields, vols, spots, correlMat, mcparams);
  // create the statistics calculator
  qf::MeanVarCalculator<double *> sc(bsmcpricer.nVariables());
  // run the simulation
  bsmcpricer.simulate(sc, npaths);
  // collect results
  qf::Matrix const & results = sc.results();
  // read out results
  double mean = results(0, 0);
  double stderror = results(1, 0);
  stderror = std::sqrt(stderror / sc.nSamples());

  // write mean and standard error into a Python dictionary
  PyObject* ret = PyDict_New();
  int ok = PyDict_SetItem(ret, asPyScalar("Mean"), asPyScalar(mean));
  PyDict_SetItem(ret, asPyScalar("StdErr"), asPyScalar(stderror));
  return ret;

PY_END;
}

static
PyObject*  pyQfWorstOfDigiBSMC(PyObject* pyDummy, PyObject* pyArgs)
{
PY_BEGIN;

  PyObject* pyPayoffType(NULL);
  PyObject* pyStrike(NULL);
  PyObject* pyTimeToFix(NULL);  
  PyObject* pyTimeToExp(NULL);
  PyObject* pySpots(NULL);
  PyObject* pyDiscountCrv(NULL);
  PyObject* pyDivYields(NULL);
  PyObject* pyVolatilities(NULL);
  PyObject* pyCorrelMatrix(NULL);
  PyObject* pyMcParams(NULL);
  PyObject* pyNPaths(NULL);

  if (!PyArg_ParseTuple(pyArgs, "OOOOOOOOOOO", &pyPayoffType, &pyStrike, 
    &pyTimeToFix, &pyTimeToExp, &pySpots, &pyDiscountCrv, &pyDivYields, &pyVolatilities, 
    &pyCorrelMatrix, &pyMcParams, &pyNPaths))
    return NULL;

  int payoffType = asInt(pyPayoffType);
  double strike = asDouble(pyStrike);
  double timeToFix = asDouble(pyTimeToFix);
  double timeToExp = asDouble(pyTimeToExp);
  qf::Vector spots = asVector(pySpots);

  std::string name = asString(pyDiscountCrv);
  qf::SPtrYieldCurve spyc = qf::market().yieldCurves().get(name);
  QF_ASSERT(spyc, "error: yield curve " + name + " not found");

  qf::Vector divYields = asVector(pyDivYields);
  qf::Vector vols = asVector(pyVolatilities);
  qf::Matrix correlMat = asMatrix(pyCorrelMatrix);
  // read the MC parameters
  qf::McParams mcparams = asMcParams(pyMcParams);
  // read the number of paths
  unsigned long npaths = asInt(pyNPaths);

  // create the product
  qf::SPtrProduct spprod(new qf::WorstOfDigitalCallPut(payoffType, strike, timeToFix, timeToExp, spots.size()));
  // create the pricer
  qf::MultiAssetBsMcPricer bsmcpricer(spprod, spyc, divYields, vols, spots, correlMat, mcparams);
  // create the statistics calculator
  qf::MeanVarCalculator<double *> sc(bsmcpricer.nVariables());
  // run the simulation
  bsmcpricer.simulate(sc, npaths);
  // collect results
  qf::Matrix const & results = sc.results();
  // read out results
  double mean = results(0, 0);
  double stderror = results(1, 0);
  stderror = std::sqrt(stderror / sc.nSamples());

  // write mean and standard error into a Python dictionary
  PyObject* ret = PyDict_New();
  int ok = PyDict_SetItem(ret, asPyScalar("Mean"), asPyScalar(mean));
  PyDict_SetItem(ret, asPyScalar("StdErr"), asPyScalar(stderror));
  return ret;

PY_END;
}

static
PyObject*  pyQfQEuroBSMC(PyObject* pyDummy, PyObject* pyArgs)
{
PY_BEGIN;

    PyObject* pyPayoffType(NULL);
    PyObject* pySpot(NULL);
    PyObject* pyStrike(NULL);
    PyObject* pyTimeToExp(NULL);
    PyObject* pyDiscountCrv(NULL);
    PyObject* pyDivYield(NULL);
    PyObject* pyVolatility(NULL);
    PyObject* pyGrowthCrv(NULL);
    PyObject* pyFxVol(NULL);
    PyObject* pyCorrel(NULL);
    PyObject* pyMcParams(NULL);
    PyObject* pyNPaths(NULL);

    if (!PyArg_ParseTuple(pyArgs, "OOOOOOOOOOOO", &pyPayoffType, &pyStrike, &pyTimeToExp,
        &pySpot, &pyDiscountCrv, &pyDivYield, &pyVolatility, &pyGrowthCrv, &pyFxVol, &pyCorrel,
        &pyMcParams, &pyNPaths))
        return NULL;

    int payoffType = asInt(pyPayoffType);
    double spot = asDouble(pySpot);
    double strike = asDouble(pyStrike);
    double timeToExp = asDouble(pyTimeToExp);

    std::string name = asString(pyDiscountCrv);
    qf::SPtrYieldCurve spyc = qf::market().yieldCurves().get(name);
    QF_ASSERT(spyc, "error: yield curve " + name + " not found");

    double divYield = asDouble(pyDivYield);
    // read volatility, either number or term structure
    qf::SPtrVolatilityTermStructure spvol;
    if (isString(pyVolatility)) { // check if input is an object name
        std::string volname = asString(pyVolatility);
        spvol = qf::market().volatilities().get(volname);
    }
    else { // assume real number
        double vol = asDouble(pyVolatility);
        spvol.reset(new qf::VolatilityTermStructure(&timeToExp, &timeToExp + 1,
            &vol, &vol + 1));
    }
    // read growth curve
    std::string growthname = asString(pyGrowthCrv);
    qf::SPtrYieldCurve spgrowth = qf::market().yieldCurves().get(growthname);
    QF_ASSERT(spgrowth, "error: growth curve " + growthname + " not found");

    // read FX volatility, either number or term structure
    qf::SPtrVolatilityTermStructure fxvol;
    if (isString(pyFxVol)) {
        std::string fxvolname = asString(pyFxVol);
        fxvol = qf::market().volatilities().get(fxvolname);
        QF_ASSERT(fxvol, "error: fx volatility " + fxvolname + " not found");
    }
    else {
        double fxvolval = asDouble(pyFxVol);
        fxvol.reset(new qf::VolatilityTermStructure(&timeToExp, &timeToExp + 1,
            &fxvolval, &fxvolval + 1));
    }
    // read correlation
    double correl = asDouble(pyCorrel);

    // read the MC parameters
    qf::McParams mcparams = asMcParams(pyMcParams);
    // read the number of paths
    unsigned long npaths = asInt(pyNPaths);

    // create the product
    qf::SPtrProduct spprod(new qf::EuropeanCallPut(payoffType, strike, timeToExp));
    // create the pricer
    qf::BsMcQuantoPricer bsmcpricer(spprod, spyc, spgrowth, divYield, spvol, fxvol, correl, spot, mcparams);
    // create the statistics calculator
    qf::MeanVarCalculator<double *> sc(bsmcpricer.nVariables());
    // run the simulation
    bsmcpricer.simulate(sc, npaths);
    // collect results
    qf::Matrix const& results = sc.results();
    // read out results
    size_t nsamples = sc.nSamples();
    double mean = results(0, 0);
    double stderror = results(1, 0);
    stderror = std::sqrt(stderror / nsamples);

    // write mean and standard error into a Python dictionary
    PyObject* ret = PyDict_New();
    int ok = PyDict_SetItem(ret, asPyScalar("Mean"), asPyScalar(mean));
    PyDict_SetItem(ret, asPyScalar("StdErr"), asPyScalar(stderror));
    return ret;

PY_END;
}

/** Market inputs shared by the multi-asset quanto functions */
struct QuantoAssetInputs
{
  qf::SPtrYieldCurve discountCurve;
  std::vector<qf::SPtrYieldCurve> growthCurves;
  qf::Vector divYields;
  qf::Vector vols;
  qf::Vector fxVols;
  qf::Vector correls;
  qf::Matrix correlMat;
};

/** Reads and checks the multi-asset quanto inputs; sizes are checked by the pricer */
static
QuantoAssetInputs asQuantoAssetInputs(PyObject* pyDiscountCrv, PyObject* pyGrowthCrvs,
                                      PyObject* pyDivYields, PyObject* pyVolatilities,
                                      PyObject* pyFxVols, PyObject* pyCorrels,
                                      PyObject* pyCorrelMatrix)
{
  QuantoAssetInputs in;

  std::string dname = asString(pyDiscountCrv);
  in.discountCurve = qf::market().yieldCurves().get(dname);
  QF_ASSERT(in.discountCurve, "error: discount yield curve " + dname + " not found");

  QF_ASSERT(PyList_Check(pyGrowthCrvs), "error: growthcrvs must be a Python list");
  for (Py_ssize_t j = 0; j < PyList_Size(pyGrowthCrvs); ++j) {
    std::string gname = asString(PyList_GetItem(pyGrowthCrvs, j));
    qf::SPtrYieldCurve spgyc = qf::market().yieldCurves().get(gname);
    QF_ASSERT(spgyc, "error: growth yield curve " + gname + " not found");
    in.growthCurves.push_back(spgyc);
  }

  in.divYields = asVector(pyDivYields);
  in.vols      = asVector(pyVolatilities);
  in.fxVols    = asVector(pyFxVols);
  in.correls   = asVector(pyCorrels);
  in.correlMat = asMatrix(pyCorrelMatrix);

  for (size_t j = 0; j < in.correls.size(); ++j)
    QF_ASSERT(in.correls[j] >= -1.0 && in.correls[j] <= 1.0,
              "error: each asset-FX correlation must lie in [-1, 1]");
  for (size_t j = 0; j < in.fxVols.size(); ++j)
    QF_ASSERT(in.fxVols[j] >= 0.0, "error: each fxvol must be non-negative");

  return in;
}

static
PyObject*  pyQfWorstOfQDigiBSMC(PyObject* pyDummy, PyObject* pyArgs)
{
PY_BEGIN;
  PyObject* pyPayoffType(NULL);
  PyObject* pyStrike(NULL);
  PyObject* pyTimeToFix(NULL);
  PyObject* pyTimeToExp(NULL);
  PyObject* pySpots(NULL);
  PyObject* pyDiscountCrv(NULL);
  PyObject* pyGrowthCrvs(NULL);
  PyObject* pyDivYields(NULL);
  PyObject* pyVolatilities(NULL);
  PyObject* pyFxVols(NULL);
  PyObject* pyCorrels(NULL);
  PyObject* pyCorrelMatrix(NULL);
  PyObject* pyMcParams(NULL);
  PyObject* pyNPaths(NULL);

  if (!PyArg_ParseTuple(pyArgs, "OOOOOOOOOOOOOO",
        &pyPayoffType, &pyStrike, &pyTimeToFix, &pyTimeToExp, &pySpots,
        &pyDiscountCrv, &pyGrowthCrvs, &pyDivYields, &pyVolatilities,
        &pyFxVols, &pyCorrels, &pyCorrelMatrix, &pyMcParams, &pyNPaths))
    return NULL;

  int    payoffType = asInt(pyPayoffType);
  double strike     = asDouble(pyStrike);
  double timeToFix  = asDouble(pyTimeToFix);
  double timeToExp  = asDouble(pyTimeToExp);
  qf::Vector spots  = asVector(pySpots);
  QuantoAssetInputs in = asQuantoAssetInputs(pyDiscountCrv, pyGrowthCrvs, pyDivYields,
                                             pyVolatilities, pyFxVols, pyCorrels, pyCorrelMatrix);
  qf::McParams mcparams = asMcParams(pyMcParams);
  unsigned long npaths  = asInt(pyNPaths);

  // create the product
  qf::SPtrProduct spprod(new qf::WorstOfDigitalCallPut(payoffType, strike, timeToFix, timeToExp, spots.size()));
  // create the pricer
  qf::MultiAssetBsMcQuantoPricer bsmcpricer(spprod, in.discountCurve, in.growthCurves, in.divYields,
                                            in.vols, in.fxVols, in.correls, spots, in.correlMat, mcparams);
  // create the statistics calculator
  qf::MeanVarCalculator<double *> sc(bsmcpricer.nVariables());
  // run the simulation
  bsmcpricer.simulate(sc, npaths);
  // collect results
  qf::Matrix const & results = sc.results();
  // read out results
  double mean = results(0, 0);
  double stderror = results(1, 0);
  stderror = std::sqrt(stderror / sc.nSamples());

  // write mean and standard error into a Python dictionary
  PyObject* ret = PyDict_New();
  int ok = PyDict_SetItem(ret, asPyScalar("Mean"), asPyScalar(mean));
  PyDict_SetItem(ret, asPyScalar("StdErr"), asPyScalar(stderror));
  return ret;

PY_END;
}

static
PyObject*  pyQfBasketQEuroBSMC(PyObject* pyDummy, PyObject* pyArgs)
{
PY_BEGIN;
  PyObject* pyPayoffType(NULL);
  PyObject* pyStrike(NULL);
  PyObject* pyTimeToExp(NULL);
  PyObject* pySpots(NULL);
  PyObject* pyWeights(NULL);
  PyObject* pyDiscountCrv(NULL);
  PyObject* pyGrowthCrvs(NULL);
  PyObject* pyDivYields(NULL);
  PyObject* pyVolatilities(NULL);
  PyObject* pyFxVols(NULL);
  PyObject* pyCorrels(NULL);
  PyObject* pyCorrelMatrix(NULL);
  PyObject* pyMcParams(NULL);
  PyObject* pyNPaths(NULL);

  if (!PyArg_ParseTuple(pyArgs, "OOOOOOOOOOOOOO",
        &pyPayoffType, &pyStrike, &pyTimeToExp, &pySpots, &pyWeights,
        &pyDiscountCrv, &pyGrowthCrvs, &pyDivYields, &pyVolatilities,
        &pyFxVols, &pyCorrels, &pyCorrelMatrix, &pyMcParams, &pyNPaths))
    return NULL;

  int    payoffType = asInt(pyPayoffType);
  double strike     = asDouble(pyStrike);
  double timeToExp  = asDouble(pyTimeToExp);
  qf::Vector spots  = asVector(pySpots);
  qf::Vector weights = asVector(pyWeights);
  QuantoAssetInputs in = asQuantoAssetInputs(pyDiscountCrv, pyGrowthCrvs, pyDivYields,
                                             pyVolatilities, pyFxVols, pyCorrels, pyCorrelMatrix);
  qf::McParams mcparams = asMcParams(pyMcParams);
  unsigned long npaths  = asInt(pyNPaths);

  // create the product
  qf::SPtrProduct spprod(new qf::BasketEuropeanCallPut(payoffType, strike, timeToExp, weights));
  // create the pricer
  qf::MultiAssetBsMcQuantoPricer bsmcpricer(spprod, in.discountCurve, in.growthCurves, in.divYields,
                                            in.vols, in.fxVols, in.correls, spots, in.correlMat, mcparams);
  // create the statistics calculator
  qf::MeanVarCalculator<double *> sc(bsmcpricer.nVariables());
  // run the simulation
  bsmcpricer.simulate(sc, npaths);
  // collect results
  qf::Matrix const & results = sc.results();
  // read out results
  double mean = results(0, 0);
  double stderror = results(1, 0);
  stderror = std::sqrt(stderror / sc.nSamples());

  // write mean and standard error into a Python dictionary
  PyObject* ret = PyDict_New();
  int ok = PyDict_SetItem(ret, asPyScalar("Mean"), asPyScalar(mean));
  PyDict_SetItem(ret, asPyScalar("StdErr"), asPyScalar(stderror));
  return ret;

PY_END;
}
