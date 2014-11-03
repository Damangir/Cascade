#ifndef __itkKolmogorovSmirnovTest_hxx
#define __itkKolmogorovSmirnovTest_hxx

#include "itkKolmogorovSmirnovTest.h"
#include "stdlib.h"
#include <algorithm>
#include "vcl_algorithm.h"

namespace itk
{
namespace Statistics
{
template< typename TMeasurement >
KolmogorovSmirnovTest< TMeasurement >::KolmogorovSmirnovTest()
{
  m_Positive = true;
  m_SortedReference = false;
}

template< typename TMeasurement >
double KolmogorovSmirnovTest< TMeasurement >::Evaluate(
    const PairDistribution & s) const
{
  DistributionType D1(s.first);
  DistributionType D2(s.second);
  if (!this->GetSortedReference())
  {
    std::sort(D1.begin(), D1.end());
  }
  std::sort(D2.begin(), D2.end());
  double cdf1 = 0;
  double cdf2 = 0;
  double step1 = 1.0 / D1.size();
  double step2 = 1.0 / D2.size();

  double x;
  typename DistributionType::const_iterator it1 = D1.begin();
  typename DistributionType::const_iterator it1End = D1.end();
  typename DistributionType::const_iterator it2 = D2.begin();
  typename DistributionType::const_iterator it2End = D2.end();

  it1 = D1.begin();
  it2 = D2.begin();

  double dp = 0;
  double dn = 0;
  while (it2 != it2End)
  {
    while (it1 != it1End)
    {
      if (*it1 >= *it2)
      {
        break;
      }
      cdf1 += step1;
      dp = vcl_max(dp, cdf1 - cdf2);
      dn = vcl_max(dn, cdf2 - cdf1);
      x = *it1;
      it1++;
    }
    x = *it2;
    cdf2 += step2;
    dp = vcl_max(dp, cdf1 - cdf2);
    dn = vcl_max(dn, cdf2 - cdf1);
    it2++;
  }
  if (this->GetPositive())
  {
    return dp;
  }
  else
  {
    return dn;
  }
}

template< typename TMeasurement >
void KolmogorovSmirnovTest< TMeasurement >::PrintSelf(std::ostream & os,
                                                      Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Right tail test: " << (m_Positive?"Yes":"No") << std::endl;
  os << indent << "Reference samples are sorted: " << (m_SortedReference?"Yes":"No") << std::endl;
}

} // end of namespace Statistics
} // end of namespace itk

#endif
