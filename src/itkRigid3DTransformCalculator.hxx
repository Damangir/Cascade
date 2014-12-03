/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkRigid3DTransformCalculator_hxx
#define __itkRigid3DTransformCalculator_hxx

#include "itkRigid3DTransformCalculator.h"

namespace itk
{

template< class TFixedImageType, class TMovingImageType >
Rigid3DTransformCalculator< TFixedImageType, TMovingImageType >::Rigid3DTransformCalculator()
{
  // constructor
  m_FixedImage = 0;
  m_MovingImage = 0;

  m_TransitionRelativeScale = 1.0 / 1e7;
  m_NumberOfBins = 50;
  m_NumberOfSamples = 5000;

  m_Rigid3DTransform = TransformType::New();
  m_Rigid3DTransform->SetIdentity();

  m_NumberOfIterations = 200;
  m_RelaxationFactor = 0.8;
  m_NumberOfLevels = 3;

  this->SetNumberOfRequiredOutputs(1);
  this->ProcessObject::SetNthOutput(0, TransformObjectType::New().GetPointer());
}

template< class TFixedImageType, class TMovingImageType >
const typename Rigid3DTransformCalculator< TFixedImageType, TMovingImageType >::TransformObjectType*
Rigid3DTransformCalculator< TFixedImageType, TMovingImageType >::GetTransformationOutput() const
{
  return static_cast< const TransformObjectType * >(this->ProcessObject::GetOutput(
      0));
}

template< class TFixedImageType, class TMovingImageType >
typename Rigid3DTransformCalculator< TFixedImageType, TMovingImageType >::TransformObjectType*
Rigid3DTransformCalculator< TFixedImageType, TMovingImageType >::GetTransformationOutput()
{
  return static_cast< TransformObjectType * >(this->ProcessObject::GetOutput(0));
}

template< class TFixedImageType, class TMovingImageType >
void Rigid3DTransformCalculator< TFixedImageType, TMovingImageType >::GenerateData()
{

  OptimizerType::Pointer optimizer = OptimizerType::New();
  typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
  typename RegistrationType::Pointer registration = RegistrationType::New();
  typename MetricType::Pointer metric = MetricType::New();

  registration->SetOptimizer(optimizer);
  registration->SetInterpolator(interpolator);
  registration->SetMetric(metric);

  registration->SetTransform(m_Rigid3DTransform);

  typedef CastImageFilter< FixedImageType, InternalImageType > FixedCastFilterType;
  typedef CastImageFilter< MovingImageType, InternalImageType > MovingCastFilterType;
  typename FixedCastFilterType::Pointer fixedCaster =
      FixedCastFilterType::New();
  typename MovingCastFilterType::Pointer movingCaster =
      MovingCastFilterType::New();

  fixedCaster->SetInput(this->GetFixedImage());
  movingCaster->SetInput(this->GetMovingImage());

  registration->SetFixedImage(fixedCaster->GetOutput());
  registration->SetMovingImage(movingCaster->GetOutput());

  typedef BinaryThresholdImageFilter< FixedImageType, MaskImageType > MaskThreshFilterType;
  typename MaskThreshFilterType::Pointer thresholdFilter =
      MaskThreshFilterType::New();
  thresholdFilter->SetInput(this->GetFixedImage());
  thresholdFilter->SetLowerThreshold(1);
  thresholdFilter->SetInsideValue(1);
  thresholdFilter->SetOutsideValue(0);

  fixedCaster->Update();
  registration->SetFixedImageRegion(
      fixedCaster->GetOutput()->GetBufferedRegion());

  typename MaskType::Pointer spatialObjectMask = MaskType::New();
  thresholdFilter->Update();
  spatialObjectMask->SetImage(thresholdFilter->GetOutput());
  metric->SetFixedImageMask(spatialObjectMask);

  typedef CenteredTransformInitializer< TransformType, FixedImageType,
      MovingImageType > TransformInitializerType;
  typename TransformInitializerType::Pointer initializer =
      TransformInitializerType::New();
  initializer->SetTransform(m_Rigid3DTransform);
  initializer->SetFixedImage(this->GetFixedImage());
  initializer->SetMovingImage(this->GetMovingImage());
  initializer->MomentsOn();
  initializer->InitializeTransform();

  typedef typename TransformType::VersorType VersorType;
  typedef typename VersorType::VectorType VectorType;
  VersorType rotation;
  VectorType axis;
  axis[0] = 0.0;
  axis[1] = 0.0;
  axis[2] = 1.0;
  const double angle = 0;
  rotation.Set(axis, angle);
  m_Rigid3DTransform->SetRotation(rotation);

  registration->SetInitialTransformParameters(
      m_Rigid3DTransform->GetParameters());

  OptimizerScalesType optimizerScales(
      m_Rigid3DTransform->GetNumberOfParameters());

  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    optimizerScales[i] = 1.0; // scale for matrix
  }

  for (unsigned int i = ImageDimension; i < 2 * ImageDimension; i++)
  {
    optimizerScales[i] = m_TransitionRelativeScale; // scale for translation
  }

  optimizer->SetScales(optimizerScales);
  optimizer->SetNumberOfIterations(m_NumberOfIterations);
  optimizer->SetRelaxationFactor(m_RelaxationFactor);

  metric->SetNumberOfHistogramBins(m_NumberOfBins);
  metric->SetNumberOfSpatialSamples(m_NumberOfSamples);
  metric->ReinitializeSeed(76926294);

  typename RegistrationInterfaceCommand::Pointer command =
      RegistrationInterfaceCommand::New();
  registration->AddObserver(itk::IterationEvent(), command);
  registration->SetNumberOfLevels(m_NumberOfLevels);

  registration->Update();

  m_Rigid3DTransform->SetParameters(registration->GetLastTransformParameters());

  this->GetTransformationOutput()->Set(m_Rigid3DTransform);
}

template< class TFixedImageType, class TMovingImageType >
void Rigid3DTransformCalculator< TFixedImageType, TMovingImageType >::PrintSelf(
    std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif

#include "itkNeighborhoodOperatorImageFilter.hxx"
