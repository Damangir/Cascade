/*
 * itkLinearCombinationModelEstimator.hxx
 *
 *  Created on: Sep 23, 2014
 *      Author: soheil
 */

#ifndef __itkLinearCombinationModelEstimator_hxx
#define __itkLinearCombinationModelEstimator_hxx

#include "itkNumericTraits.h"

namespace itk
{
namespace Statistics
{

template< typename TSample, typename TComponent, class comparator=std::less<double> >
class ComponentCompare
{
public:
  ComponentCompare(const TSample* _sample)
  {
    sample = _sample;
  }
  bool operator()(const TComponent* c1, const TComponent* c2)
  {
    MeasurementVectorType peak1;
    MeasurementVectorType peak2;
    double peak1Value = 0;
    double peak2Value = 0;
    for (size_t i = 0; i < sample->Size(); i++)
    {
      MeasurementVectorType mv = sample->GetMeasurementVector(i);
      const double v1 = const_cast<TComponent*>(c1)->Evaluate(mv);
      if (v1 > peak1Value)
      {
        peak1 = mv;
        peak1Value = v1;
      }
      const double v2 = const_cast<TComponent*>(c2)->Evaluate(mv);
      if (v2 > peak2Value)
      {
        peak2 = mv;
        peak2Value = v2;
      }
    }
    return comparator()(peak1[0], peak2[0]);
  }
  bool operator()(const TComponent& c1, const TComponent& c2)
  {
    return this->operator ()(&c1, &c2);
  }
private:
  typedef typename TSample::MeasurementVectorType MeasurementVectorType;
  const TSample* sample;
}
;

template< typename TSample, typename TComponent >
LinearCombinationModelEstimator< TSample, TComponent >::LinearCombinationModelEstimator()
{
  m_Sample = 0;
  m_MaxIteration = 100;
  m_CurrentIteration = 0;
  m_NumberOfClasses = 2;
  m_SummedDistribution = MembershipFunctionType::New();
}

template< typename TSample, typename TComponent >
void LinearCombinationModelEstimator< TSample, TComponent >::PrintSelf(
    std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "Maximum Iteration: " << this->GetMaximumIteration()
  << std::endl;
  os << indent << "Sample: " << this->GetSample() << std::endl;
  os << indent << "Number Of Components: " << this->GetNumberOfComponents()
  << std::endl;
}

template< typename TSample, typename TComponent >
void LinearCombinationModelEstimator< TSample, TComponent >::SetMaximumIteration(
    int numberOfIterations)
{
  m_MaxIteration = numberOfIterations;
}

template< typename TSample, typename TComponent >
int LinearCombinationModelEstimator< TSample, TComponent >::GetMaximumIteration() const
{
  return m_MaxIteration;
}

template< typename TSample, typename TComponent >
void LinearCombinationModelEstimator< TSample, TComponent >::SetSample(
    const TSample *sample)
{
  m_Sample = sample;
}

template< typename TSample, typename TComponent >
const TSample *
LinearCombinationModelEstimator< TSample, TComponent >::GetSample() const
{
  return m_Sample;
}

template< typename TSample, typename TComponent >
unsigned int LinearCombinationModelEstimator< TSample, TComponent >::GetNumberOfComponents() const
{
  return 0;
}

template< typename TSample, typename TComponent >
double LinearCombinationModelEstimator< TSample, TComponent >::estimateModel(
    const TSample* hist,
    std::vector< typename ComponentType::Pointer >& componentsHist,
    std::vector< double >& proportion)
{
  const size_t mvSize = hist->GetMeasurementVectorSize();
  const size_t numberOfClasses = proportion.size();

  typename EstimatorType::Pointer estimator = EstimatorType::New();

  estimator->SetSample(hist);
  estimator->SetMaximumIteration(200);

  ParametersType params(mvSize + mvSize * mvSize);
  for (unsigned int j = 0; j < mvSize; j++)
    params[j] = 0;

  std::vector< ParametersType > initialParameters(numberOfClasses);
  Array< double > initialProportions(numberOfClasses);
  componentsHist.erase(componentsHist.begin(), componentsHist.end());
  for (unsigned int i = 0; i < numberOfClasses; i++)
  {
    for (unsigned int j = 0; j < mvSize; j++)
    {
      const double span = hist->GetDimensionMaxs(j)[hist->Size() - 1]
          - hist->GetDimensionMins(j)[0];
      params[j] = (span) * (i + 1.0) / (numberOfClasses + 2.0); // mean of component i
      params[mvSize + j * (1 + mvSize)] = span / numberOfClasses; // variance of component i
    }

    initialParameters[i] = params;
    initialProportions[i] = 1.0 / numberOfClasses;

    componentsHist.push_back(ComponentType::New());
    (componentsHist[i])->SetSample(hist);
    (componentsHist[i])->SetParameters(initialParameters[i]);

    estimator->AddComponent(componentsHist[i]);
  }

  estimator->SetInitialProportions(initialProportions);

  estimator->Update();

  // Output the results
  for (unsigned int i = 0; i < numberOfClasses; i++)
  {
    proportion[i] = estimator->GetProportions()[i];
  }

  double pv = 0;
  double error = 0;
  for (size_t i = 0; i < hist->Size(); i++)
  {
    typename TSample::MeasurementVectorType mv = hist->GetMeasurementVector(i);
    const double currentStep = mv[0] - pv;
    const double targetPD = double(hist->GetFrequency(i))
        / hist->GetTotalFrequency() / currentStep;

    double estimatedPD = 0;
    for (size_t j = 0; j < numberOfClasses; j++)
    {
      const double thisModePD = proportion[j] * componentsHist[j]->Evaluate(mv);
      estimatedPD += thisModePD;
    }
    error += std::abs(targetPD - estimatedPD) * currentStep;
    pv = mv[0];

  }
  return error;
}
template< typename TSample, typename TComponent >
double LinearCombinationModelEstimator< TSample, TComponent >::componentOverlap(
    const WeightedComponentType& comp1, const WeightedComponentType& comp2)
{
  double overlap = 0;
  typename SampleType::ConstIterator it = m_Sample->Begin();
  typename SampleType::ConstIterator end = m_Sample->End();
  for (; it != end; ++it)
  {
    overlap += std::min< double >(
        comp1.first->Evaluate(it.GetMeasurementVector()) * comp1.second,
        comp2.first->Evaluate(it.GetMeasurementVector()) * comp1.second);
  }
  return overlap;
}

template< typename TSample, typename TComponent >
void LinearCombinationModelEstimator< TSample, TComponent >::GenerateData()
{
  typename SampleType::Pointer sampleCopy = SampleType::New();
  sampleCopy->Graft(m_Sample);

  const double totalFrequency =
      static_cast< double >(sampleCopy->GetTotalFrequency());
  const size_t sampleSize = sampleCopy->Size();
  typedef std::vector< double > HistType;
  HistType sample(sampleCopy->Size());
  HistType posHist(sampleCopy->Size());
  HistType negHist(sampleCopy->Size());
  for (size_t i = 0; i < sampleSize; i++)
  {
    sample[i] = sampleCopy->GetFrequency(i);
  }

  std::vector< ComponentPointerType > componentsHist;
  std::vector< double > proportion(m_NumberOfClasses);

  const double err1 = this->estimateModel(sampleCopy, componentsHist,
                                          proportion);

  itkDebugMacro(<< "Error in first estimate: " << err1);

  ComponentCompare< SampleType, ComponentType, std::greater<double> > comparator(sampleCopy);
  for (size_t i = 0; i < m_NumberOfClasses; i++)
  {
    for (size_t j = (i + 1); j < m_NumberOfClasses; j++)
    {
      if (comparator(componentsHist[i], componentsHist[j]))
      {
        std::swap < ComponentPointerType>(componentsHist[i], componentsHist[j]);
        std::swap < double>(proportion[i], proportion[j]);
      }
    }
  }

  // Output the results
  for (unsigned int i = 0; i < m_NumberOfClasses; i++)
  {
    itkDebugMacro(
        << "Cluster[" << i << "]:\t"<< proportion[i] << " x " << componentsHist[i]->GetFullParameters());
  }

  double totalPosNeg = 0;
  {
    double pv = 0;
    double totalEstimatedPD = 0;
    double totalTargetPD = 0;
    for (size_t i = 0; i < sampleSize; i++)
    {
      MeasurementVectorType mv = sampleCopy->GetMeasurementVector(i);
      const double currentStep = mv[0] - pv;
      const double targetPD = sample[i] / totalFrequency / currentStep;
      double estimatedPD = 0;
      for (size_t j = 0; j < m_NumberOfClasses; j++)
      {
        const double thisModePD = proportion[j]
            * componentsHist[j]->Evaluate(mv);
        estimatedPD += thisModePD;
      }

      totalEstimatedPD += estimatedPD * currentStep;
      totalTargetPD += targetPD * currentStep;
      pv = mv[0];

      if (targetPD > estimatedPD)
      {
        posHist[i] = (targetPD - estimatedPD) * totalFrequency * currentStep;
        negHist[i] = 0;
        totalPosNeg += posHist[i];
      }
      else
      {
        negHist[i] = (estimatedPD - targetPD) * totalFrequency * currentStep;
        posHist[i] = 0;
      }

    }
    itkDebugMacro(<< "totalEstimatedPD " << totalEstimatedPD);
    itkDebugMacro(<< "totalTargetPD " << totalTargetPD);
  }
  const double scale = totalPosNeg / totalFrequency;
  itkDebugMacro(<< "Scale: " << scale);

  const double maxError = 0.02;
  double prevErr = 1;
  std::vector< ComponentPointerType > posComponentsHist;
  std::vector< double > posProportion;
  for (size_t i = 0; i < sampleSize; i++)
  {
    sampleCopy->SetFrequency(i, posHist[i]);
  }
  for (unsigned int i = 1; i < 20; i++)
  {
    posProportion.resize(i);
    double err;

    try
    {
      err = estimateModel(sampleCopy, posComponentsHist, posProportion) * scale;
    }
    catch (itk::ExceptionObject &)
    {
      err = 1;
    }
    itkDebugMacro(
        << "Error in positive residual: " << err << " (mode=" << i << ")")

    if (err < maxError) break;
    if (err > prevErr)
    {
      i--;
      posProportion.resize(i);
      estimateModel(sampleCopy, posComponentsHist, posProportion);
      break;
    }
    prevErr = err;
  }

  std::vector< ComponentPointerType > negComponentsHist;
  std::vector< double > negProportion;
  for (size_t i = 0; i < sampleSize; i++)
  {
    sampleCopy->SetFrequency(i, negHist[i]);
  }
  prevErr = 1;
  for (unsigned int i = 1; i < 20; i++)
  {
    negProportion.resize(i);
    double err;
    try
    {
      err = estimateModel(sampleCopy, negComponentsHist, negProportion) * scale;
    }
    catch (itk::ExceptionObject &)
    {
      err = 1;
    }

    itkDebugMacro(
        << "Error in negative residual: " << err << " (mode=" << i << ")")

    if (err < maxError) break;
    if (err > prevErr)
    {
      i--;
      negProportion.resize(i);
      estimateModel(sampleCopy, negComponentsHist, negProportion);
      break;
    }
    prevErr = err;
  }

  m_Distribution.resize(m_NumberOfClasses);
  for (size_t i = 0; i < m_NumberOfClasses; i++)
  {
    m_Distribution[i] = MembershipFunctionType::New();
    m_Distribution[i]->AddMembership(
        static_cast< BaseMembershipFunctionType* >(componentsHist[i]->GetMembershipFunction()),
        proportion[i]);
  }

  for (size_t j = 0; j < posProportion.size(); j++)
  {
    double maxOverlap = 0;
    size_t maxOverlapIndex;
    for (size_t i = 0; i < m_NumberOfClasses; i++)
    {
      const double thisOverlap =
          componentOverlap(
              WeightedComponentType(
                  static_cast< BaseMembershipFunctionType* >(componentsHist[i]->GetMembershipFunction()),
                  proportion[i]),
              WeightedComponentType(
                  static_cast< BaseMembershipFunctionType* >(posComponentsHist[j]->GetMembershipFunction()),
                  scale * posProportion[j]));

      if (thisOverlap > maxOverlap)
      {
        maxOverlap = thisOverlap;
        maxOverlapIndex = i;
      }
    }
    m_Distribution[maxOverlapIndex]->AddMembership(
        static_cast< BaseMembershipFunctionType* >(posComponentsHist[j]->GetMembershipFunction()),
        scale * posProportion[j]);
  }

  for (size_t j = 0; j < negProportion.size(); j++)
  {
    double maxOverlap = 0;
    size_t maxOverlapIndex;
    for (size_t i = 0; i < m_NumberOfClasses; i++)
    {
      const double thisOverlap =
          componentOverlap(
              WeightedComponentType(
                  static_cast< BaseMembershipFunctionType* >(componentsHist[i]->GetMembershipFunction()),
                  proportion[i]),
              WeightedComponentType(
                  static_cast< BaseMembershipFunctionType* >(negComponentsHist[j]->GetMembershipFunction()),
                  scale * negProportion[j]));

      if (thisOverlap > maxOverlap)
      {
        maxOverlap = thisOverlap;
        maxOverlapIndex = i;
      }
    }
    m_Distribution[maxOverlapIndex]->AddMembership(
        static_cast< BaseMembershipFunctionType* >(negComponentsHist[j]->GetMembershipFunction()),
        -scale * negProportion[j]);
  }

  m_SummedDistribution->Reset();
  for (size_t i = 0; i < m_NumberOfClasses; i++)
  {
    m_SummedDistribution->AddMembership(m_Distribution[i], 1);
  }

  for (size_t i = 0; i < sampleSize; i++)
  {
    sampleCopy->SetFrequency(i, sample[i]);
  }
}

template< typename TSample, typename TComponent >
void LinearCombinationModelEstimator< TSample, TComponent >::Update()
{
  this->GenerateData();
}
} // end of namespace Statistics
} // end of namespace itk

#endif

