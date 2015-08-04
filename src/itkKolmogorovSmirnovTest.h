#ifndef __itkKolmogorovSmirnovTest_h
#define __itkKolmogorovSmirnovTest_h

#include "itkFunctionBase.h"
#include "itkMeasurementVectorTraits.h"
#include "itkSample.h"

#include "vector"

namespace itk
{
namespace Statistics
{
template< typename TMeasurement >
class KolmogorovSmirnovTest: public FunctionBase<
    std::pair<std::vector< TMeasurement >,std::vector< TMeasurement > >, double >
{
public:
  /** Standard typedefs */
  typedef KolmogorovSmirnovTest Self;
  typedef FunctionBase< std::pair<std::vector< TMeasurement >,std::vector< TMeasurement > >, double > Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(KolmogorovSmirnovTest, FunctionBase)
  ;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
  ;

  typedef std::vector< TMeasurement > DistributionType;
  typedef std::pair<DistributionType, DistributionType > PairDistribution;
  double Evaluate(const PairDistribution & x) const;

  itkGetConstMacro(Positive, bool);
  itkSetMacro(Positive, bool);
  itkBooleanMacro(Positive);
  itkGetConstMacro(SortedReference, bool);
  itkSetMacro(SortedReference, bool);
  itkBooleanMacro(SortedReference)
  ;

#ifdef ITK_USE_CONCEPT_CHECKING
    // Begin concept checking
    itkConceptMacro( MeasurementLessThanComparableCheck,
        ( Concept::LessThanComparable< TMeasurement > ) );
    // End concept checking
#endif


protected:
  KolmogorovSmirnovTest();
  virtual ~KolmogorovSmirnovTest()
  {
  }
  void PrintSelf(std::ostream & os, Indent indent) const;

private:
  bool m_Positive;
  bool m_SortedReference;
};
// end of class
}// end of namespace Statistics
} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkKolmogorovSmirnovTest.hxx"
#endif

#endif
