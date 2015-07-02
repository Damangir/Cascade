#ifndef __itkAPTest_h
#define __itkAPTest_h

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
class APTest: public StatisticalTestBase<SubSampleT1, SubSampleT2, TRealValueType >
{
public:
  /** Standard typedefs */
  typedef APTest Self;
  typedef StatisticalTestBase<SubSampleT1, SubSampleT2, TRealValueType > Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(APTest, StatisticalTestBase)
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

    TRealValueType cdf1=0; // CDF for sample 1

    TRealValueType p=0; // Cummulative sample CDF
    TRealValueType pr=0; // Reference CDF autoconvolution

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
        cdf1 += step1*it1.GetFrequency();
        pr+= cdf1*it1.GetFrequency();
        ++it1;
        }
      p += cdf1*it2.GetFrequency();
      ++it2;
      }
    pr /= subsample1->GetTotalFrequency();
    p /= subsample2->GetTotalFrequency();

    return this->APStatistics(p, pr);
  }

  inline TRealValueType APStatistics(const TRealValueType& p,const TRealValueType& pr) const
  {
    if (this->GetRightTail())
    {
      return vcl_max(TRealValueType(0), (p-pr)/(1-pr));
    }

    if (this->GetLeftTail())
    {
      return vcl_max(TRealValueType(0), (pr-p)/pr);
    }

    if (this->GetTwoTail())
    {
      return vcl_max((pr-p)/pr, (p-pr)/(1-pr));
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
  APTest()
  {
    m_SortedFirst = false;
    m_SortedSecond = false;
  }
  virtual ~APTest()
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
