/*
 * HistModelEstimator.h
 *
 *  Created on: Sep 22, 2014
 *      Author: soheil
 */

#ifndef __itkLinearCombinationModelEstimator_h
#define __itkLinearCombinationModelEstimator_h

#include "itkExpectationMaximizationMixtureModelEstimator.h"
#include "itkLinearCombinationMembership.h"

namespace itk
{
namespace Statistics
{

template< typename TSample, typename TComponent >
class LinearCombinationModelEstimator: public Object
{
public:
  /** Standard class typedef */
  typedef LinearCombinationModelEstimator Self;
  typedef Object Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Standard macros */
  itkTypeMacro(LinearCombinationModelEstimator,
      Object)
  ;itkNewMacro(Self)
  ;

  /** TSample template argument related typedefs */
  typedef TSample SampleType;
  typedef typename TSample::MeasurementType MeasurementType;
  typedef typename TSample::MeasurementVectorType MeasurementVectorType;

  typedef TComponent ComponentType;
  typedef typename TComponent::Pointer ComponentPointerType;

  typedef LinearCombinationMembership<MeasurementVectorType> MembershipFunctionType;
  typedef typename MembershipFunctionType::MembershipFunctionType BaseMembershipFunctionType;

  typedef typename MembershipFunctionType::Pointer MembershipFunctionPointerType;
  typedef typename MembershipFunctionType::WeightedComponentType WeightedComponentType;

  typedef std::vector< MembershipFunctionPointerType > DistributionType;

  /** Sets the target data that will be classified by this */
  void SetSample(const TSample *sample);

  /** Returns the target data */
  const TSample * GetSample() const;

  /** Set/Gets the maximum number of iterations. When the optimization
   * process reaches the maximum number of interations, even if the
   * class parameters aren't converged, the optimization process
   * stops. */
  void SetMaximumIteration(int numberOfIterations);

  int GetMaximumIteration() const;

  /** Gets the current iteration. */
  int GetCurrentIteration()
  {
    return m_CurrentIteration;
  }

  itkGetMacro(NumberOfClasses, size_t);
  itkSetMacro(NumberOfClasses, size_t);

  /** Gets the total number of classes currently plugged in. */
  unsigned int GetNumberOfComponents() const;

  const MembershipFunctionType* GetNthComponent(size_t n) const
  {
    return m_Distribution[n];
  }
  const MembershipFunctionType* GetEstimatedDistribution() const
  {
    return m_SummedDistribution;
  }

  /** Runs the optimization process. */
  void Update();

protected:
  LinearCombinationModelEstimator();
  virtual ~LinearCombinationModelEstimator()
  {
  }
  void PrintSelf(std::ostream & os, Indent indent) const;

  /** Starts the estimation process */
  void GenerateData();

private:

  typedef ExpectationMaximizationMixtureModelEstimator< SampleType > EstimatorType;
  typedef Array< double > ParametersType;

  double estimateModel(
      const TSample* hist,
      std::vector< typename ComponentType::Pointer >& componentsHist,
      std::vector< double >& proportion);

  double componentOverlap(const WeightedComponentType& comp1,const WeightedComponentType& comp2);
  /** Target data sample pointer*/
  const TSample *m_Sample;

  int m_MaxIteration;
  int m_CurrentIteration;

  size_t m_NumberOfClasses;
  DistributionType m_Distribution;
  MembershipFunctionPointerType m_SummedDistribution;
};
// end of class

} // end of namespace Statistics
} // end of namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkLinearCombinationModelEstimator.hxx"
#endif

#endif
