/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkAffineTransformCalculator_hxx
#define __itkAffineTransformCalculator_hxx

#include "itkAffineTransformCalculator.h"

namespace itk
{

template <class TFixedImageType, class TMovingImageType>
AffineTransformCalculator<TFixedImageType, TMovingImageType>
::AffineTransformCalculator()
{
  // constructor
  m_FixedImage = 0;
  m_MovingImage = 0;

  m_RigidTransform = RigidTransformType::New();
  m_AffineTransform = TransformType::New();

  m_RigidTransform->SetIdentity();
  m_AffineTransform->SetIdentity();

  this->SetNumberOfRequiredOutputs(1);
  this->ProcessObject::SetNthOutput( 0, TransformObjectType::New().GetPointer() );
}

template <class TFixedImageType, class TMovingImageType>
const typename AffineTransformCalculator<TFixedImageType, TMovingImageType>::TransformObjectType*
AffineTransformCalculator<TFixedImageType, TMovingImageType>
::GetTransformationOutput() const
{
  return static_cast< const TransformObjectType * >( this->ProcessObject::GetOutput(0) );
}

template <class TFixedImageType, class TMovingImageType>
typename AffineTransformCalculator<TFixedImageType, TMovingImageType>::TransformObjectType*
AffineTransformCalculator<TFixedImageType, TMovingImageType>
::GetTransformationOutput()
{
  return static_cast< TransformObjectType * >( this->ProcessObject::GetOutput(0) );
}


template <class TFixedImageType, class TMovingImageType>
void AffineTransformCalculator<TFixedImageType, TMovingImageType>
::GenerateData()
{
  // do the processing

  this->DownsampleImage();
  this->RescaleImages();

  this->RigidRegistration();
  this->AffineRegistration();

  this->GetTransformationOutput()->Set(m_AffineTransform);
}


template <class TFixedImageType, class TMovingImageType>
void AffineTransformCalculator<TFixedImageType, TMovingImageType>
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf(os,indent);

  os
    << indent << "end of PrintSelf."
    << std::endl;
}

template <class TFixedImageType, class TMovingImageType>
void AffineTransformCalculator<TFixedImageType, TMovingImageType>
::DownsampleImage()
{
  // resample image to isotropic resolution

  typedef itk::ResampleImageFilter<TFixedImageType, TFixedImageType> ResamplerType;
  typename ResamplerType::Pointer resampler = ResamplerType::New();

  typedef itk::IdentityTransform<double, SpaceDimension> TransformType;
  typename TransformType::Pointer transform = TransformType::New();

  typedef itk::LinearInterpolateImageFunction<TFixedImageType, double> LinearInterpolatorType;
  typename LinearInterpolatorType::Pointer lInterp = LinearInterpolatorType::New();

  transform->SetIdentity();
  resampler->SetTransform( transform );
  resampler->SetInput( this->GetFixedImage() );

  typename FixedImageType::SpacingType spacing;
  typename FixedImageType::SizeType size;
  for(unsigned int i=0;i<SpaceDimension;i++)
  {
    spacing[i] = 1.0;
    size[i] = (this->GetFixedImage()->GetLargestPossibleRegion().GetSize()[i])
              *(this->GetFixedImage()->GetSpacing()[i])/spacing[i];
  }

  resampler->SetInterpolator(lInterp);
  resampler->SetSize(size);
  resampler->SetOutputSpacing(spacing);
  resampler->SetOutputOrigin( this->GetFixedImage()->GetOrigin() );
  resampler->SetOutputDirection( this->GetFixedImage()->GetDirection() );
  resampler->SetDefaultPixelValue( 0 );

  try
    {
    resampler->Update();
    }
  catch (itk::ExceptionObject &exception)
    {
    itkExceptionMacro("Exception caught during spatial resampling" << std::endl << exception)
    }

  m_FixedImage = resampler->GetOutput();
  m_FixedImage->DisconnectPipeline();
}

template <class TFixedImageType, class TMovingImageType>
void AffineTransformCalculator<TFixedImageType, TMovingImageType>
::RescaleImages()
{
  // rescale patient image and atlas image intensities to 0-255

  typedef itk::RescaleIntensityImageFilter<TFixedImageType, TFixedImageType> FixedRescalerType;
  typename FixedRescalerType::Pointer fixedRescaler = FixedRescalerType::New();

  typedef itk::RescaleIntensityImageFilter<TMovingImageType, TMovingImageType> MovingRescalerType;
  typename MovingRescalerType::Pointer movingRescaler = MovingRescalerType::New();

  fixedRescaler->SetInput( m_FixedImage );
  fixedRescaler->SetOutputMinimum( 0 );
  fixedRescaler->SetOutputMaximum( 255 );

  movingRescaler->SetInput( this->GetMovingImage() );
  movingRescaler->SetOutputMinimum( 0 );
  movingRescaler->SetOutputMaximum( 255 );

  try
    {
    fixedRescaler->Update();
    movingRescaler->Update();
    }
  catch (itk::ExceptionObject &exception)
    {
    itkExceptionMacro("Exception caught during intensity rescaling" << std::endl << exception)
    }

  m_FixedImage = fixedRescaler->GetOutput();
  m_FixedImage->DisconnectPipeline();

  m_MovingImage = movingRescaler->GetOutput();
  m_MovingImage->DisconnectPipeline();
}

template <class TFixedImageType, class TMovingImageType>
void AffineTransformCalculator<TFixedImageType, TMovingImageType>
::RigidRegistration()
{
  // perform intial rigid alignment of atlas with patient image
  typedef itk::VersorRigid3DTransformOptimizer OptimizerType;
  typedef itk::MattesMutualInformationImageToImageMetric<TFixedImageType, TMovingImageType> MetricType;
  typedef itk::MultiResolutionImageRegistrationMethod<TFixedImageType, TMovingImageType> MultiResRegistrationType;
  typedef itk::LinearInterpolateImageFunction<TMovingImageType, double> LinearInterpolatorType;

  typename RigidTransformType::Pointer  transform = RigidTransformType::New();
  typename OptimizerType::Pointer optimizer = OptimizerType::New();
  typename MetricType::Pointer metric = MetricType::New();
  typename MultiResRegistrationType::Pointer registration = MultiResRegistrationType::New();
  typename LinearInterpolatorType::Pointer linearInterpolator = LinearInterpolatorType::New();

  metric->SetNumberOfHistogramBins( 64 );
  metric->SetNumberOfSpatialSamples( 100000 ); // default number is too small

  registration->SetMetric( metric );
  registration->SetOptimizer( optimizer );
  registration->SetInterpolator( linearInterpolator );
  //registration->SetNumberOfLevels( 3 );

  // perform registration only on subsampled image for speed gains
  typename MultiResRegistrationType::ScheduleType schedule;
  schedule.SetSize(2,SpaceDimension);

  for(unsigned int i=0;i<SpaceDimension;i++)
    {
    schedule[0][i] = 4;
    schedule[1][i] = 2;
    }

  registration->SetSchedules(schedule, schedule);

  registration->SetFixedImageRegion( m_FixedImage->GetBufferedRegion() );

  registration->SetTransform( transform );

  registration->SetFixedImage( m_FixedImage );
  registration->SetMovingImage( m_MovingImage );

  // transform initialization
  typedef itk::CenteredTransformInitializer<RigidTransformType, TFixedImageType, TMovingImageType> TransformInitializerType;
  typename TransformInitializerType::Pointer initializer = TransformInitializerType::New();

  initializer->SetTransform( transform );
  initializer->SetFixedImage( m_FixedImage );
  initializer->SetMovingImage( m_MovingImage );

  initializer->GeometryOn(); // geometry initialization because of multimodality

  try
    {
    initializer->InitializeTransform();
    }
  catch (itk::ExceptionObject &exception)
    {
    itkExceptionMacro("Exception caught during transform initialization " << std::endl << exception)
    }

  registration->SetInitialTransformParameters( transform->GetParameters() );

  typedef OptimizerType::ScalesType OptimizerScalesType;
  OptimizerScalesType optimizerScales( transform->GetNumberOfParameters() );
  const double scale = 1.0;
  const double translationScale = scale/2500;

  for(unsigned int i=0;i<SpaceDimension;i++)
    {
    optimizerScales[i] = scale;
    optimizerScales[SpaceDimension+i] = translationScale;
    }
  optimizer->SetScales( optimizerScales );
  optimizer->SetMaximumStepLength( 0.05  );
  optimizer->SetMinimumStepLength( 0.001 );
  optimizer->SetNumberOfIterations( 250 );
  optimizer->MinimizeOn();

  try
    {
    registration->Update();
    }
  catch (itk::ExceptionObject &exception)
    {
    itkExceptionMacro("Exception caught during rigid registration" << std::endl << exception)
    }

  typename OptimizerType::ParametersType finalParameters = registration->GetLastTransformParameters();

  transform->SetParameters( finalParameters );

  m_RigidTransform->SetCenter( transform->GetCenter() );
  m_RigidTransform->SetParameters( finalParameters );
}


template <class TFixedImageType, class TMovingImageType>
void AffineTransformCalculator<TFixedImageType, TMovingImageType>
::AffineRegistration()
{
  // perform refined affine alignment of atlas with patient image

  typedef itk::RegularStepGradientDescentOptimizer OptimizerType;
  typedef itk::MattesMutualInformationImageToImageMetric<TFixedImageType, TMovingImageType> MetricType;
  typedef itk::MultiResolutionImageRegistrationMethod<TFixedImageType, TMovingImageType> MultiResRegistrationType;
  typedef itk::LinearInterpolateImageFunction<TMovingImageType, double> LinearInterpolatorType;

  typename TransformType::Pointer  transform = TransformType::New();
  typename OptimizerType::Pointer optimizer = OptimizerType::New();
  typename MetricType::Pointer metric = MetricType::New();
  typename MultiResRegistrationType::Pointer registration = MultiResRegistrationType::New();
  typename LinearInterpolatorType::Pointer linearInterpolator = LinearInterpolatorType::New();

  metric->SetNumberOfHistogramBins( 64 );
  metric->SetNumberOfSpatialSamples( 100000 ); // default number is too small

  registration->SetMetric( metric );
  registration->SetOptimizer( optimizer );
  registration->SetInterpolator( linearInterpolator );
  //registration->SetNumberOfLevels( 3 );

  // perform registration only on subsampled image for speed gains
  typename MultiResRegistrationType::ScheduleType schedule;
  schedule.SetSize(2,SpaceDimension);
  for(unsigned int i=0;i<SpaceDimension;i++)
    {
    schedule[0][i] = 4;
    schedule[1][i] = 2;
    }

  registration->SetSchedules(schedule, schedule);

  registration->SetFixedImageRegion( m_FixedImage->GetBufferedRegion() );

  registration->SetTransform( transform );

  registration->SetFixedImage( m_FixedImage );
  registration->SetMovingImage( m_MovingImage );

  transform->SetCenter( m_RigidTransform->GetCenter() );
  transform->SetTranslation( m_RigidTransform->GetTranslation() );
  transform->SetMatrix( m_RigidTransform->GetMatrix() );

  registration->SetTransform( transform );

  registration->SetInitialTransformParameters( transform->GetParameters() );

  typedef OptimizerType::ScalesType OptimizerScalesType;
  OptimizerScalesType optimizerScales( transform->GetNumberOfParameters() );
  const double matrixScale = 1.0;
  const double translationScale = matrixScale/200;

  for(unsigned int i=0;i<SpaceDimension*SpaceDimension;i++)
  {
  optimizerScales[i] = matrixScale;
  }
  for(unsigned int i=SpaceDimension*SpaceDimension;i<SpaceDimension*(SpaceDimension+1);i++)
  {
  optimizerScales[i] = translationScale;
  }
  optimizer->SetScales( optimizerScales );
  optimizer->SetMaximumStepLength( 0.05  );
  optimizer->SetMinimumStepLength( 0.001 );
  optimizer->SetNumberOfIterations( 200 );
  optimizer->MinimizeOn();

  try
    {
    registration->Update();
    }
  catch (itk::ExceptionObject &exception)
    {
    itkExceptionMacro("Exception caught during Affine registration" << std::endl << exception)
    }

  typename OptimizerType::ParametersType finalParameters = registration->GetLastTransformParameters();

  transform->SetParameters( finalParameters );

  m_AffineTransform->SetCenter( transform->GetCenter() );
  m_AffineTransform->SetParameters( finalParameters );
}

} // end namespace itk

#endif

#include "itkNeighborhoodOperatorImageFilter.hxx"
