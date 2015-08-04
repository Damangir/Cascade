/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkAffineTransformCalculator_hxx
#define __itkAffineTransformCalculator_hxx

#include "itkAffineTransformCalculator.h"
#include "vnl/vnl_inverse.h"
namespace itk
{

template< class TFixedImageType, class TMovingImageType >
AffineTransformCalculator< TFixedImageType, TMovingImageType >::AffineTransformCalculator()
{
  // constructor
  m_FixedImage = 0;
  m_MovingImage = 0;

  m_TransitionRelativeScale = 1.0 / 1e7;
  m_NumberOfBins = 50;
  m_NumberOfSamples = 5000;

  m_AffineTransform = TransformType::New();
  m_AffineTransform->SetIdentity();

  m_NumberOfIterations = 200;
  m_RelaxationFactor = 0.8;
  m_NumberOfLevels = 3;

  m_IntraRegistration = false;

  this->SetNumberOfRequiredOutputs(1);
  this->ProcessObject::SetNthOutput(0, TransformObjectType::New().GetPointer());
}

template< class TFixedImageType, class TMovingImageType >
const typename AffineTransformCalculator< TFixedImageType, TMovingImageType >::TransformObjectType*
AffineTransformCalculator< TFixedImageType, TMovingImageType >::GetTransformationOutput() const
{
  return static_cast< const TransformObjectType * >(this->ProcessObject::GetOutput(
      0));
}

template< class TFixedImageType, class TMovingImageType >
typename AffineTransformCalculator< TFixedImageType, TMovingImageType >::TransformObjectType*
AffineTransformCalculator< TFixedImageType, TMovingImageType >::GetTransformationOutput()
{
  return static_cast< TransformObjectType * >(this->ProcessObject::GetOutput(0));
}

template< class TFixedImageType, class TMovingImageType >
void AffineTransformCalculator< TFixedImageType, TMovingImageType >::GenerateData()
{

  OptimizerType::Pointer optimizer = OptimizerType::New();
  typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
  typename RegistrationType::Pointer registration = RegistrationType::New();
  typename MetricType::Pointer metric = MetricType::New();

  registration->SetOptimizer(optimizer);
  registration->SetInterpolator(interpolator);
  registration->SetMetric(metric);

  registration->SetTransform(m_AffineTransform);

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
  initializer->SetTransform(m_AffineTransform);
  initializer->SetFixedImage(this->GetFixedImage());
  initializer->SetMovingImage(this->GetMovingImage());
  initializer->MomentsOn();
  initializer->InitializeTransform();
  registration->SetInitialTransformParameters(
      m_AffineTransform->GetParameters());

  OptimizerScalesType optimizerScales(
      m_AffineTransform->GetNumberOfParameters());

  for (unsigned int i = 0; i < ImageDimension * ImageDimension; i++)
  {
    optimizerScales[i] = 1.0; // scale for matrix
  }

  for (unsigned int i = ImageDimension * ImageDimension;
      i < ImageDimension * (ImageDimension + 1); i++)
  {
    optimizerScales[i] = m_TransitionRelativeScale; // scale for translation
  }

  optimizer->SetScales(optimizerScales);

  metric->SetNumberOfHistogramBins(m_NumberOfBins);
  metric->SetNumberOfSpatialSamples(m_NumberOfSamples);

  metric->ReinitializeSeed(76926294);

  optimizer->SetNumberOfIterations(m_NumberOfIterations);
  optimizer->SetRelaxationFactor(m_RelaxationFactor);

  typename RegistrationInterfaceCommand::Pointer command =
      RegistrationInterfaceCommand::New();
  registration->AddObserver(itk::IterationEvent(), command);
  registration->SetNumberOfLevels(m_NumberOfLevels);

  registration->Update();

  m_AffineTransform->SetParameters(registration->GetLastTransformParameters());

  /*
   * Remove shearing and scaling.
   */
  if (m_IntraRegistration)
  {
    typedef vnl_matrix< double > VnlMatrixType;

    VnlMatrixType M = m_AffineTransform->GetMatrix().GetVnlMatrix();

    VnlMatrixType PQ = M;
    VnlMatrixType NQ = M;
    VnlMatrixType PQNQDiff;

    const unsigned int maximumIterations = 100;

    for (unsigned int ni = 0; ni < maximumIterations; ni++)
    {
      // Average current Qi with its inverse transpose
      NQ = (PQ + vnl_inverse_transpose(PQ)) / 2.0;
      PQNQDiff = NQ - PQ;
      if (PQNQDiff.frobenius_norm() < 1e-7)
      {
        break;
      }
      else
      {
        PQ = NQ;
      }
    }
    m_AffineTransform->SetMatrix(NQ);
  }

  this->GetTransformationOutput()->Set(m_AffineTransform);
}

template< class TFixedImageType, class TMovingImageType >
void AffineTransformCalculator< TFixedImageType, TMovingImageType >::PrintSelf(
    std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}

} // end namespace itk

#endif

#include "itkNeighborhoodOperatorImageFilter.hxx"
