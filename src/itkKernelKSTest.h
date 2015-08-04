#ifndef __itkKernelKSTest_h
#define __itkKernelKSTest_h

#include "itkConceptChecking.h"
#include "itkMeasurementVectorTraits.h"
#include "itkFixedArray.h"
#include "itkSubsample.h"
#include "itkStatisticsAlgorithm.h"
#include "itkGaussianDistribution.h"

#include "vcl_algorithm.h"

#include "itkKSTest.h"
#include "itkTimeProbe.h"

namespace itk
{
namespace Statistics
{
template< typename SubSampleT1, typename SubSampleT2 = SubSampleT1,
    typename TRealValueType = double >
class KernelKSTest: public KSTest< SubSampleT1, SubSampleT2, TRealValueType >
{
public:
  /** Standard typedefs */
  typedef KernelKSTest Self;
  typedef StatisticalTestBase< SubSampleT1, SubSampleT2, TRealValueType > Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(KernelKSTest, StatisticalTestBase)
  ;

  /** Method for creation through the object factory. */
  itkNewMacro (Self);

  typedef typename Superclass::SampleType1 SampleType1;
  typedef typename Superclass::SampleType2 SampleType2;

  typedef Subsample< SampleType1 > SubSampleType1;
  typedef Subsample< SampleType2 > SubSampleType2;

  typedef typename SubSampleType1::ConstIterator SubsampleConstIter1;
  typedef typename SubSampleType2::ConstIterator SubsampleConstIter2;

  virtual TRealValueType Evaluate(const SampleType1 * x1,
                                  const SampleType2 * x2) const
  {
    if(x1->Size() == 0 || x2->Size() == 0)
    {
      return 0;
    }

    typename SubSampleType1::Pointer subsample1 = SubSampleType1::New();
    subsample1->SetSample(x1);
    subsample1->InitializeWithAllInstances();

    typename SubSampleType2::Pointer subsample2 = SubSampleType2::New();
    subsample2->SetSample(x2);
    subsample2->InitializeWithAllInstances();

    if (!this->GetSortedFirst())
    {
      Algorithm::HeapSort< SubSampleType1 >(subsample1, 0, 0,
                                            subsample1->Size());
    }
    if (!this->GetSortedSecond())
    {
      Algorithm::HeapSort< SubSampleType2 >(subsample2, 0, 0,
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

    TRealValueType step = vcl_min(bandwidth1, bandwidth2);
    SubsampleConstIter1 it1 = subsample1->Begin();
    SubsampleConstIter1 it1min = subsample1->Begin();
    SubsampleConstIter1 it1max = subsample1->Begin();
    SubsampleConstIter1 it1End = subsample1->End();

    SubsampleConstIter2 it2 = subsample2->Begin();
    SubsampleConstIter2 it2min = subsample2->Begin();
    SubsampleConstIter2 it2max = subsample2->Begin();
    SubsampleConstIter2 it2End = subsample2->End();

    TRealValueType precdf1 = 0; // CDF for sample 1
    TRealValueType precdf2 = 0; // CDF for sample 2

    TRealValueType dp = 0; // Distance toward positive
    TRealValueType dn = 0; // Distance toward negative

    return 1;

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

      if(it1min!=it1max)
        for (it1 = it1min; it1 != it1max; ++it1)
      {
        cdf1 += it1.GetFrequency()
           * this->Kernel((x - it1.GetMeasurementVector()[0]) / bandwidth1);
      }
      cdf1 /= subsample1->GetTotalFrequency();

      if(it2min!=it2max)
      for (it2 = it2min; it2 != it2max; ++it2)
      {
        cdf2 += it2.GetFrequency()
            * this->Kernel((x - it2.GetMeasurementVector()[0]) / bandwidth2);
      }
      cdf2 /= subsample2->GetTotalFrequency();

      dp = vcl_max(dp, cdf1 - cdf2);
      dn = vcl_max(dn, cdf2 - cdf1);
    }

    return this->KSStatistics(dn, dp);
  }

  itkGetConstMacro(Sigma1, TRealValueType);itkSetMacro(Sigma1, TRealValueType);itkGetConstMacro(Sigma2, TRealValueType);itkSetMacro(Sigma2, TRealValueType);

  virtual void SetSigma(const TRealValueType arg_)
  {
    this->SetSigma1(arg_);
    this->SetSigma2(arg_);
  }

protected:
  KernelKSTest()
  {
    m_Sigma1 = 1;
    m_Sigma2 = 1;
    m_Z = 1.96;
    m_N = 500;
    UpdateCache();
  }
  virtual ~KernelKSTest()
  {
  }
  void PrintSelf(std::ostream & os, Indent indent) const
  {
    Superclass::PrintSelf(os, indent);
  }

private:

  inline TRealValueType Kernel(const TRealValueType& x) const
  {
    if (x < -m_Z) return 0;
    if (x >= m_Z) return 1;
    return 1;//(x + m_Z) / (2 * m_Z);//m_Cache.GetElement(m_N * (x + m_Z) / (2 * m_Z));
  }

  void UpdateCache()
  {
    double x = -m_Z;
    const double step = 2 * m_Z / (m_N - 1);
    m_Cache = itk::Array< double >(m_N+1);
    for (unsigned int i = 0; i <= m_N; i++)
    {
      m_Cache.SetElement(i, GaussianDistribution::CDF(x));
      x += step;
    }
  }

  TRealValueType m_Sigma1;
  TRealValueType m_Sigma2;

  itk::Array< double > m_Cache;

  TRealValueType m_Z;
  unsigned int m_N;

};
// end of class
}// end of namespace Statistics
} // end of namespace itk

#endif
