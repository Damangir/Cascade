#ifndef __itkKSTest_h
#define __itkKSTest_h

#include "itkConceptChecking.h"
#include "itkMeasurementVectorTraits.h"
#include "itkFixedArray.h"
#include "itkSubsample.h"
#include "itkStatisticsAlgorithm.h"
#include "vcl_algorithm.h"

#include "itkStatisticalTestBase.h"
namespace itk
{
namespace Statistics
{
template< typename SubSampleT1, typename SubSampleT2=SubSampleT1, typename TRealValueType = double >
class KSTest: public StatisticalTestBase<SubSampleT1, SubSampleT2, TRealValueType >
{
public:
  /** Standard typedefs */
  typedef KSTest Self;
  typedef StatisticalTestBase<SubSampleT1, SubSampleT2, TRealValueType > Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(KSTest, StatisticalTestBase)
  ;

  /** Method for creation through the object factory. */
  itkNewMacro (Self);

  typedef typename Superclass::SampleType1 SampleType1;
  typedef typename Superclass::SampleType2 SampleType2;

  typedef Subsample<SampleType1> SubSampleType1;
  typedef Subsample<SampleType2> SubSampleType2;

  typedef typename SubSampleType1::ConstIterator SubsampleConstIter1;
  typedef typename SubSampleType2::ConstIterator SubsampleConstIter2;

  virtual TRealValueType Evaluate(const SampleType1 * x1, const SampleType2 * x2) const
  {
    typename SubSampleType1::Pointer subsample1 = SubSampleType1::New();
    subsample1->SetSample(x1);
    subsample1->InitializeWithAllInstances();

    typename SubSampleType2::Pointer subsample2 = SubSampleType2::New();
    subsample2->SetSample(x2);
    subsample2->InitializeWithAllInstances();

    if (!this->GetSortedFirst())
    {
      Algorithm::HeapSort<SubSampleType1>(subsample1, 0, 0, subsample1->Size());
    }
    if (!this->GetSortedSecond())
    {
      Algorithm::HeapSort<SubSampleType2>(subsample2, 0, 0, subsample2->Size());
    }

    TRealValueType step1 = 1.0 / subsample1->GetTotalFrequency();
    TRealValueType step2 = 1.0 / subsample2->GetTotalFrequency();

    TRealValueType cdf1=0; // CDF for sample 1
    TRealValueType cdf2=0; // CDF for sample 2

    TRealValueType dp=0; // Distance toward positive
    TRealValueType dn=0; // Distance toward negative


    SubsampleConstIter1 it1 = subsample1->Begin();
    SubsampleConstIter1 it1End = subsample1->End();
    SubsampleConstIter2 it2 = subsample2->Begin();
    SubsampleConstIter2 it2End = subsample2->End();

    TRealValueType last1=it1.GetMeasurementVector()[0];
    TRealValueType last2=it2.GetMeasurementVector()[0];

    while (it2 != it2End)
      {
      while (it1 != it1End)
        {
        if (it1.GetMeasurementVector()[0] >= it2.GetMeasurementVector()[0])
          {
          break;
          }
        last1=it1.GetMeasurementVector()[0];
        cdf1 += step1*it1.GetFrequency();
        dp = vcl_max(dp, cdf1 - cdf2);
        dn = vcl_max(dn, cdf2 - cdf1);
        ++it1;
        }
      last2=it2.GetMeasurementVector()[0];
      cdf2 += step2*it2.GetFrequency();
      dp = vcl_max(dp, cdf1 - cdf2);
      dn = vcl_max(dn, cdf2 - cdf1);
      ++it2;
      }

    while (it1 != it1End)
      {
      last1=it1.GetMeasurementVector()[0];
      cdf1 += step1;
      dp = vcl_max(dp, cdf1 - cdf2);
      dn = vcl_max(dn, cdf2 - cdf1);
      ++it1;
      }

    return this->KSStatistics(dn,dp);
  }

  inline TRealValueType KSStatistics(const TRealValueType& dn, const TRealValueType& dp) const
  {
    if (this->GetRightTail())
    {
      return dp;
    }

    if (this->GetLeftTail())
    {
      return dn;
    }
    
    if (this->GetTwoTail())
    {
      return vcl_max(dn, dp);
    }
    
    itkExceptionMacro ("No direction set for the test.");
  }

  itkGetConstMacro(SortedFirst, bool);
  itkSetMacro(SortedFirst, bool);
  itkBooleanMacro(SortedFirst);

  itkGetConstMacro(SortedSecond, bool);
  itkSetMacro(SortedSecond, bool);
  itkBooleanMacro(SortedSecond);

protected:
  KSTest()
  {
    m_SortedFirst = false;
    m_SortedSecond = false;
  }
  virtual ~KSTest()
  {
  }
  void PrintSelf(std::ostream & os, Indent indent) const
  {
    Superclass::PrintSelf(os, indent);
  }

private:
  bool m_SortedFirst;
  bool m_SortedSecond;
};
// end of class
}// end of namespace Statistics
} // end of namespace itk

#endif
