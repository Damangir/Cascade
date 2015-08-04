#ifndef __itkKernelKSTest_h
#define __itkKernelKSTest_h

#include "itkConceptChecking.h"
#include "itkMeasurementVectorTraits.h"
#include "itkFixedArray.h"
#include "itkSubsample.h"
#include "itkStatisticsAlgorithm.h"
#include "itkGaussianDistribution.h"

#include "vcl_algorithm.h"

#include "itkStatisticalTestBase.h"
namespace itk
{
namespace Statistics
{
template< typename TScalarValueType = double,
    typename TRealValueType = TScalarValueType >
class KernelKSTest: public StatisticalTestBase<
    ListSample< FixedArray< TScalarValueType, 1 > >, TRealValueType >
{
public:
  /** Standard typedefs */
  typedef KernelKSTest Self;
  typedef StatisticalTestBase< ListSample< FixedArray< TScalarValueType, 1 > >,
      TRealValueType > Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(KernelKSTest, StatisticalTestBase)
  ;

  /** Method for creation through the object factory. */
  itkNewMacro (Self);

  typedef typename Superclass::SampleType SampleType;
  typedef typename Superclass::MeasurementVectorType MeasurementVectorType;
  typedef typename Superclass::PairSample PairSample;

  typedef Subsample< SampleType > SubsampleType;
  typedef typename SubsampleType::ConstIterator SubsampleConstIter;

  virtual TRealValueType Evaluate(const PairSample & x) const
  {
    typename SubsampleType::Pointer subsample1 = SubsampleType::New();
    subsample1->SetSample(x.first);
    subsample1->InitializeWithAllInstances();

    typename SubsampleType::Pointer subsample2 = SubsampleType::New();
    subsample2->SetSample(x.second);
    subsample2->InitializeWithAllInstances();

    if (!m_SortedFirst)
    {
      Algorithm::HeapSort< SubsampleType >(subsample1, 0, 0,
                                           subsample1->Size());
    }
    if (!m_SortedSecond)
    {
      Algorithm::HeapSort< SubsampleType >(subsample2, 0, 0,
                                           subsample2->Size());
    }

    TRealValueType bandwidth1 = 1.06
        * m_Sigma1 * vcl_pow(subsample1->GetTotalFrequency(), -0.2);
    TRealValueType bandwidth2 = 1.06
        * m_Sigma2 * vcl_pow(subsample2->GetTotalFrequency(), -0.2);

    TRealValueType interval = vcl_max(bandwidth1, bandwidth2) * 1.96;

    TRealValueType min = vcl_min(subsample1->GetMeasurementVector(0)[0],
                                 subsample2->GetMeasurementVector(0)[0])
                         - interval;
    TRealValueType max = vcl_max(
        subsample1->GetMeasurementVector(subsample1->Size() - 1)[0],
        subsample2->GetMeasurementVector(subsample2->Size() - 1)[0])
                         + interval;

    TRealValueType step = vcl_min(bandwidth1, bandwidth2) / 10.0;

    SubsampleConstIter it1 = subsample1->Begin();
    SubsampleConstIter it1min = subsample1->Begin();
    SubsampleConstIter it1max = subsample1->Begin();
    SubsampleConstIter it1End = subsample1->End();

    SubsampleConstIter it2 = subsample2->Begin();
    SubsampleConstIter it2min = subsample2->Begin();
    SubsampleConstIter it2max = subsample2->Begin();
    SubsampleConstIter it2End = subsample2->End();

    TRealValueType precdf1 = 0; // CDF for sample 1
    TRealValueType precdf2 = 0; // CDF for sample 2

    TRealValueType dp = 0; // Distance toward positive
    TRealValueType dn = 0; // Distance toward negative

    for (TRealValueType x = min; x <= max; x += step)
    {

      while ((it1min != it1End)
          && (x - it1min.GetMeasurementVector()[0] > interval))
      {
        ++it1min;
        ++precdf1;
      }
      while ((it1max != it1End)
          && (it1max.GetMeasurementVector()[0] - x < interval))
      {
        ++it1max;
      }

      while ((it2min != it2End)
          && (x - it2min.GetMeasurementVector()[0] > interval))
      {
        ++it2min;
        ++precdf2;
      }
      while ((it2max != it2End)
          && (it2max.GetMeasurementVector()[0] - x < interval))
      {
        ++it2max;
      }

      TRealValueType cdf1 = precdf1; // CDF for sample 1
      TRealValueType cdf2 = precdf2; // CDF for sample 2

      for (it1 = it1min; it1 != it1max; ++it1)
      {
        cdf1 += it1.GetFrequency() * gaussian->EvaluateCDF(
            (x - it1.GetMeasurementVector()[0]) / bandwidth1);
      }
      cdf1 /= subsample1->GetTotalFrequency();

      for (it2 = it2min; it2 != it2max; ++it2)
      {
        cdf2 += it2.GetFrequency() *gaussian->EvaluateCDF(
            (x - it2.GetMeasurementVector()[0]) / bandwidth1);
      }
      cdf2 /= subsample2->GetTotalFrequency();

      dp = vcl_max(dp, cdf1 - cdf2);
      dn = vcl_max(dn, cdf2 - cdf1);

    }

    if (this->GetRightTail())
    {
      return dp;
    }

    if (this->GetLeftTail())
    {
      return dn;
    }

    return vcl_max(dn, dp);
  }

  itkGetConstMacro(Sigma1, TScalarValueType);itkSetMacro(Sigma1, TScalarValueType);itkGetConstMacro(Sigma2, TScalarValueType);itkSetMacro(Sigma2, TScalarValueType);

  virtual void SetSigma(const TScalarValueType arg_)
  {
    this->SetSigma1(arg_);
    this->SetSigma2(arg_);
  }

#ifdef ITK_USE_CONCEPT_CHECKING
// Begin concept checking
  itkConceptMacro( MeasurementLessThanComparableCheck,
      ( Concept::LessThanComparable< TScalarValueType > ) );
// End concept checking
#endif

protected:
  KernelKSTest()
  {
    m_Sigma1 = 1;
    m_Sigma2 = 1;
    m_SortedFirst = false;
    m_SortedSecond = false;
    gaussian = GaussianDistribution::New();
    gaussian->SetMean(0);
    gaussian->SetVariance(1);
  }
  virtual ~KernelKSTest()
  {
  }
  void PrintSelf(std::ostream & os, Indent indent) const
  {
    Superclass::PrintSelf(os, indent);
  }

private:
  bool m_SortedFirst;
  bool m_SortedSecond;
  TScalarValueType m_Sigma1;
  TScalarValueType m_Sigma2;

  GaussianDistribution::Pointer gaussian;

};
// end of class
}// end of namespace Statistics
} // end of namespace itk

#endif
