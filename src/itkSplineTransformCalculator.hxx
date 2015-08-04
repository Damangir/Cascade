/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkSplineTransformCalculator_hxx
#define __itkSplineTransformCalculator_hxx

#include "itkSplineTransformCalculator.h"
#include "itkBSplineTransformParametersAdaptor.h"

namespace itk
{
template< class TFixedImageType, class TMovingImageType >
SplineTransformCalculator< TFixedImageType, TMovingImageType >::SplineTransformCalculator()
{
  m_UseExplicitPDFDerivatives = false;
  m_UseCachingOfBSplineWeights = false;
  m_NumberOfIterations = 100;

  m_FixedImageMaskSpatialObject = SpatialObjectMaskType::New();

  m_Transform = CompositeTransformType::New();
  this->SetNumberOfRequiredOutputs(1);
  this->ProcessObject::SetNthOutput(0, TransformObjectType::New().GetPointer());
}

template< class TFixedImageType, class TMovingImageType >
const typename SplineTransformCalculator< TFixedImageType, TMovingImageType >::TransformObjectType*
SplineTransformCalculator< TFixedImageType, TMovingImageType >::GetTransformationOutput() const
{
  return static_cast< const TransformObjectType * >(this->ProcessObject::GetOutput(
      0));
}

template< class TFixedImageType, class TMovingImageType >

typename SplineTransformCalculator< TFixedImageType, TMovingImageType >::TransformObjectType*
SplineTransformCalculator< TFixedImageType, TMovingImageType >::GetTransformationOutput()
{
  return static_cast< TransformObjectType * >(this->ProcessObject::GetOutput(0));
}

template< class TFixedImageType, class TMovingImageType >

void SplineTransformCalculator< TFixedImageType, TMovingImageType >::GenerateData()
{
  this->UpdateProgress(0.0);

  const FixedImageType* fixedImage = this->GetFixedImage();
  const MaskImageType* fixedImageMask = this->GetFixedImageMask();
  const MovingImageType* movingImage = this->GetMovingImage();

  this->GetTransformationOutput()->Set(0);

  /*
   * Definitions
   */
  const unsigned int numberOfLevels = 3;

  typename MetricType::Pointer metric = MetricType::New();
  metric->SetNumberOfHistogramBins(50);
  metric->SetUseMovingImageGradientFilter(false);
  metric->SetUseFixedImageGradientFilter(false);

  typedef itk::RegistrationParameterScalesFromPhysicalShift< MetricType > ScalesEstimatorType;
  typename ScalesEstimatorType::Pointer scalesEstimator =
      ScalesEstimatorType::New();
  scalesEstimator->SetMetric(metric);
  scalesEstimator->SetTransformForward(true);
  scalesEstimator->SetSmallParameterVariation(1.0);

  typename OptimizerType::Pointer optimizer = OptimizerType::New();
  optimizer->SetLearningRate(1.0);
  optimizer->SetNumberOfIterations(m_NumberOfIterations);
  optimizer->SetScalesEstimator(scalesEstimator);
  optimizer->SetDoEstimateLearningRateOnce(false); //true by default
  optimizer->SetDoEstimateLearningRateAtEachIteration(true);
  optimizer->SetConvergenceWindowSize(m_NumberOfIterations / 10);

  typename BSplineRegistrationType::Pointer bsplineRegistration =
      BSplineRegistrationType::New();

  // Shrink the virtual domain by specified factors for each level.  See documentation
  // for the itkShrinkImageFilter for more detailed behavior.
  typename BSplineRegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
  shrinkFactorsPerLevel.SetSize(3);
  shrinkFactorsPerLevel[0] = 3;
  shrinkFactorsPerLevel[1] = 2;
  shrinkFactorsPerLevel[2] = 1;

  // Smooth by specified gaussian sigmas for each level.  These values are specified in
  // physical units.
  typename BSplineRegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
  smoothingSigmasPerLevel.SetSize(3);
  smoothingSigmasPerLevel[0] = 2;
  smoothingSigmasPerLevel[1] = 1;
  smoothingSigmasPerLevel[2] = 1;

  typename BSplineTransformType::Pointer outputBSplineTransform =
      const_cast< BSplineTransformType * >(bsplineRegistration->GetOutput()->Get());

  typename BSplineTransformType::PhysicalDimensionsType physicalDimensions;
  typename BSplineTransformType::MeshSizeType meshSize;
  for (unsigned int d = 0; d < SpaceDimension; d++)
  {
    physicalDimensions[d] =
        fixedImage->GetSpacing()[d] * static_cast< CoordinateRepType >(fixedImage->GetLargestPossibleRegion().GetSize()[d]
            - 1);
    meshSize[d] = 5;
  }

  // Create the transform adaptors

  typedef itk::BSplineTransformParametersAdaptor< BSplineTransformType > BSplineTransformAdaptorType;
  typename BSplineRegistrationType::TransformParametersAdaptorsContainerType adaptors;
  // Create the transform adaptors specific to B-splines
  for (unsigned int level = 0; level < numberOfLevels; level++)
  {
    typedef itk::ShrinkImageFilter< FixedImageType, FixedImageType > ShrinkFilterType;
    typename ShrinkFilterType::Pointer shrinkFilter = ShrinkFilterType::New();
    shrinkFilter->SetShrinkFactors(shrinkFactorsPerLevel[level]);
    shrinkFilter->SetInput(fixedImage);
    shrinkFilter->Update();

    // A good heuristic is to double the b-spline mesh resolution at each level

    typename BSplineTransformType::MeshSizeType requiredMeshSize;
    for (unsigned int d = 0; d < SpaceDimension; d++)
    {
      requiredMeshSize[d] = meshSize[d] << level;
    }

    typedef itk::BSplineTransformParametersAdaptor< BSplineTransformType > BSplineAdaptorType;
    typename BSplineAdaptorType::Pointer bsplineAdaptor =
        BSplineAdaptorType::New();
    bsplineAdaptor->SetTransform(outputBSplineTransform);
    bsplineAdaptor->SetRequiredTransformDomainMeshSize(requiredMeshSize);
    bsplineAdaptor->SetRequiredTransformDomainOrigin(
        shrinkFilter->GetOutput()->GetOrigin());
    bsplineAdaptor->SetRequiredTransformDomainDirection(
        shrinkFilter->GetOutput()->GetDirection());
    bsplineAdaptor->SetRequiredTransformDomainPhysicalDimensions(
        physicalDimensions);

    adaptors.push_back(bsplineAdaptor.GetPointer());
  }

  typename BSplineRegistrationType::MetricSamplingPercentageArrayType metricSamplingPercentagePerLevel;

  metricSamplingPercentagePerLevel.SetSize(numberOfLevels);
  metricSamplingPercentagePerLevel.Fill(0.02);

  bsplineRegistration->SetMetricSamplingPercentagePerLevel(metricSamplingPercentagePerLevel);
  bsplineRegistration->SetMetricSamplingStrategy(
      BSplineRegistrationType::REGULAR);

  bsplineRegistration->SetFixedImage(0, fixedImage);
  bsplineRegistration->SetMovingImage(0, movingImage);
  bsplineRegistration->SetMetric(metric);
  bsplineRegistration->SetNumberOfLevels(numberOfLevels);
  bsplineRegistration->SetSmoothingSigmasPerLevel(smoothingSigmasPerLevel);
  bsplineRegistration->SetShrinkFactorsPerLevel(shrinkFactorsPerLevel);
  bsplineRegistration->SetOptimizer(optimizer);
  bsplineRegistration->SetMovingInitialTransform(m_Transform);
  bsplineRegistration->SetTransformParametersAdaptorsPerLevel(adaptors);

  outputBSplineTransform->SetTransformDomainOrigin(fixedImage->GetOrigin());
  outputBSplineTransform->SetTransformDomainPhysicalDimensions(
      physicalDimensions);
  outputBSplineTransform->SetTransformDomainMeshSize(meshSize);
  outputBSplineTransform->SetTransformDomainDirection(
      fixedImage->GetDirection());
  outputBSplineTransform->SetIdentity();

  bsplineRegistration->Update();
  m_Transform->AddTransform(
      const_cast< BSplineTransformType * >(bsplineRegistration->GetOutput()->Get()));
  this->GetTransformationOutput()->Set(m_Transform.GetPointer());

}

template< class TFixedImageType, class TMovingImageType >
void SplineTransformCalculator< TFixedImageType, TMovingImageType >::PrintSelf(
    std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "end of PrintSelf." << std::endl;
}

} // end namespace itk

#endif
