/**
@file  basketeuropeancallput.hpp
@brief The payoff of a European Call/Put option on a basket of assets
*/
#pragma once

#include <qflib/products/product.hpp>

BEGIN_NAMESPACE(qf)

/** The basket european call/put class
*/
class BasketEuropeanCallPut : public Product
{

public:
  /** Initializing ctor */
  BasketEuropeanCallPut(int payoffType,
                        double strike,
                        double timeToExp,
                        Vector const& weights);

  /** The number of assets this product depends on */
  virtual size_t nAssets() const override;

  /** Evaluates the product given the passed-in path
      The "pricePath" matrix must have as many rows as
      the number of fixing times
      */
  virtual void eval(Matrix const & pricePath) override;

  /** Evaluates the product at fixing time index idx
  */
  virtual void eval(size_t idx, Vector const& spots, double contValue) override;

private:
  int payoffType_;          // 1: call; -1 put
  double strike_;
  Vector weights_;
};

///////////////////////////////////////////////////////////////////////////////
// Inline definitions

inline
BasketEuropeanCallPut::BasketEuropeanCallPut(int payoffType,
                                              double strike,
                                              double timeToExp,
                                              Vector const& weights)
: payoffType_(payoffType), strike_(strike), weights_(weights)
{
  QF_ASSERT(payoffType == 1 || payoffType == -1,
    "BasketEuropeanCallPut: payoffType must be 1 (call) or -1 (put)");
  QF_ASSERT(strike >= 0.0,
    "BasketEuropeanCallPut: strike must be non-negative");
  QF_ASSERT(timeToExp > 0.0,
    "BasketEuropeanCallPut: timeToExp must be strictly positive");
  QF_ASSERT(weights.size() >= 1,
    "BasketEuropeanCallPut: must have at least one asset");
  for (size_t j = 0; j < weights.size(); ++j)
    QF_ASSERT(weights[j] > 0.0,
      "BasketEuropeanCallPut: each weight must be strictly positive");

  fixTimes_.resize(1);
  fixTimes_[0] = timeToExp;

  payTimes_.resize(1);
  payTimes_[0] = timeToExp;

  payAmounts_.resize(1);
}

inline
size_t BasketEuropeanCallPut::nAssets() const
{
  return weights_.size();
}

inline void BasketEuropeanCallPut::eval(Matrix const & pricePath)
{
  size_t nfixings = pricePath.n_rows;
  QF_ASSERT(fixTimes_.size() == nfixings, "BasketEuropeanCallPut: number of fixings mismatch in price path!");
  size_t nassets = pricePath.n_cols;
  QF_ASSERT(weights_.size() == nassets, "BasketEuropeanCallPut: number of assets mismatch in price path!");

  double basket = 0.0;
  for (size_t j = 0; j < nassets; ++j)
    basket += weights_[j] * pricePath(0, j);

  if (payoffType_ == 1)
    payAmounts_[0] = basket >= strike_ ? basket - strike_ : 0.0;
  else
    payAmounts_[0] = basket >= strike_ ? 0.0 : strike_ - basket;
}

// Not implemented
inline void BasketEuropeanCallPut::eval(size_t idx, Vector const& spots, double contValue)
{
  QF_ASSERT(0, "not implemented!");
}

END_NAMESPACE(qf)
