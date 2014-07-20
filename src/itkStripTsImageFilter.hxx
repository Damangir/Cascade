/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkStripTsImageFilter_hxx
#define __itkStripTsImageFilter_hxx

#include "itkStripTsImageFilter.h"

namespace itk
{

template< class TImageType, class TAtlasLabelType >
StripTsImageFilter< TImageType, TAtlasLabelType >::StripTsImageFilter()
{
  // constructor
  m_WorkingImage = 0;
  m_AtlasMask = 0;
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::GenerateData()
{
  // Copy input as current brain mask
  typedef itk::ImageDuplicator< TAtlasLabelType > AtlasDuplicatorType;
  typename AtlasDuplicatorType::Pointer duplicator = AtlasDuplicatorType::New();
  duplicator->SetInputImage(this->GetAtlasBrainMask());
  duplicator->Update();
  m_AtlasMask = duplicator->GetOutput();
  m_AtlasMask->DisconnectPipeline();
  
  // do the processing
  this->DownsampleImage(1);
  this->RescaleImages();

  this->BinaryErosion();

  /*
   * TODO: Consider estimating the brain volume and limit level set to that volume.
   */
  this->MultiResLevelSet();

  this->UpsampleLabels();

  this->GraftOutput(m_AtlasMask);
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::PrintSelf(
    std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "end of PrintSelf." << std::endl;
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::DownsampleImage(
    int isoSpacing)
{
  // resample patient image to isotropic resolution

  typedef itk::ResampleImageFilter< TImageType, TImageType > SubjectResamplerType;
  typename SubjectResamplerType::Pointer subjectResampler =
      SubjectResamplerType::New();

  typedef itk::ResampleImageFilter< TAtlasLabelType, TAtlasLabelType > AtlasResamplerType;
  typename AtlasResamplerType::Pointer atlasResampler =
      AtlasResamplerType::New();

  typedef itk::IdentityTransform< double, SpaceDimension > TransformType;
  typename TransformType::Pointer transform = TransformType::New();

  typedef itk::LinearInterpolateImageFunction< TImageType, double > LinearInterpolatorType;
  typename LinearInterpolatorType::Pointer lInterp =
      LinearInterpolatorType::New();

  typedef itk::NearestNeighborInterpolateImageFunction< TAtlasLabelType, double > NNInterpolatorType;
  typename NNInterpolatorType::Pointer nnInterp = NNInterpolatorType::New();

  transform->SetIdentity();

  subjectResampler->SetTransform(transform);
  subjectResampler->SetInput(this->GetSubjectImage());

  atlasResampler->SetTransform(transform);
  atlasResampler->SetInput(m_AtlasMask);

  typename TImageType::SpacingType subjectSpacing;
  typename TImageType::SizeType subjectSize;

  typename TAtlasLabelType::SpacingType atlasSpacing;
  typename TAtlasLabelType::SizeType atlasSize;

  for (unsigned int i = 0; i < SpaceDimension; i++)
  {
    subjectSpacing[i] = isoSpacing;
    subjectSize[i] =
        (this->GetSubjectImage()->GetLargestPossibleRegion().GetSize()[i]) * (this->GetSubjectImage()->GetSpacing()[i]) / subjectSpacing[i];
    atlasSpacing[i] = subjectSpacing[i];
    atlasSize[i] = subjectSize[i];
  }

  subjectResampler->SetInterpolator(lInterp);
  subjectResampler->SetSize(subjectSize);
  subjectResampler->SetOutputSpacing(subjectSpacing);
  subjectResampler->SetOutputOrigin(this->GetSubjectImage()->GetOrigin());
  subjectResampler->SetOutputDirection(this->GetSubjectImage()->GetDirection());
  subjectResampler->SetDefaultPixelValue(0);

  atlasResampler->SetInterpolator(nnInterp);
  atlasResampler->SetSize(subjectSize);
  atlasResampler->SetOutputSpacing(subjectSpacing);
  atlasResampler->SetOutputOrigin(this->GetSubjectImage()->GetOrigin());
  atlasResampler->SetOutputDirection(this->GetSubjectImage()->GetDirection());
  atlasResampler->SetDefaultPixelValue(0);

  try
  {
    subjectResampler->Update();
    atlasResampler->Update();
  }
  catch (itk::ExceptionObject &exception)
  {
    itkExceptionMacro(
        "Exception caught during spatial resampling" << std::endl << exception)
  }

  m_WorkingImage = subjectResampler->GetOutput();
  m_WorkingImage->DisconnectPipeline();

  m_AtlasMask = atlasResampler->GetOutput();
  m_AtlasMask->DisconnectPipeline();
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::RescaleImages()
{
  // rescale patient image and atlas image intensities to 0-255

  typedef itk::RescaleIntensityImageFilter< TImageType, TImageType > ImageRescalerType;
  typename ImageRescalerType::Pointer imageRescaler = ImageRescalerType::New();

  imageRescaler->SetInput(m_WorkingImage);
  imageRescaler->SetOutputMinimum(0);
  imageRescaler->SetOutputMaximum(255);

  try
  {
    imageRescaler->Update();
  }
  catch (itk::ExceptionObject &exception)
  {
    itkExceptionMacro(
        "Exception caught during intensity rescaling" << std::endl << exception)
  }

  m_WorkingImage = imageRescaler->GetOutput();
  m_WorkingImage->DisconnectPipeline();
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::BinaryErosion()
{
  // make sure mask is binary
  itk::ImageRegionIterator< AtlasLabelType > iterLabel(
      m_AtlasMask, m_AtlasMask->GetLargestPossibleRegion());

  for (iterLabel.GoToBegin(); !iterLabel.IsAtEnd(); ++iterLabel)
  {
    if (iterLabel.Get() != 0)
    {
      iterLabel.Set(1);
    }
  }

  // erode binary mask
  typedef itk::BinaryBallStructuringElement< typename AtlasLabelType::PixelType,
      SpaceDimension > StructuringElementType;
  typedef itk::BinaryErodeImageFilter< AtlasLabelType, AtlasLabelType,
      StructuringElementType > ErodeFilterType;
  StructuringElementType structuringElement;
  typename ErodeFilterType::Pointer eroder = ErodeFilterType::New();

  structuringElement.SetRadius(3);
  structuringElement.CreateStructuringElement();

  eroder->SetKernel(structuringElement);
  eroder->SetInput(m_AtlasMask);
  eroder->SetErodeValue(1);
  eroder->SetBackgroundValue(0);

  try
  {
    eroder->Update();
  }
  catch (itk::ExceptionObject &exception)
  {
    itkExceptionMacro(
        "Exception caught during erosion" << std::endl << exception)
  }

  m_AtlasMask = eroder->GetOutput();
  m_AtlasMask->DisconnectPipeline();
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::MultiResLevelSet()
{
  // level set refinement of brain mask in two resolution levels

  // coarse (2mm isotropic resolution)
  DownsampleImage(2);
  LevelSetRefinement(2);

  // fine (1mm isotropic resolution)
  DownsampleImage(1);
  LevelSetRefinement(1);
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::LevelSetRefinement(
    int isoSpacing)
{
  // refine brain mask using geodesic active contour level set evolution

  // have to cast images to float first for level-set
  typedef itk::Image< float, SpaceDimension > FloatImageType;

  typedef itk::CastImageFilter< ImageType, FloatImageType > ImageCasterType;
  typename ImageCasterType::Pointer imageCaster = ImageCasterType::New();

  typedef itk::CastImageFilter< AtlasLabelType, FloatImageType > LabelCasterType;
  typename LabelCasterType::Pointer labelCaster = LabelCasterType::New();

  imageCaster->SetInput(m_WorkingImage);
  labelCaster->SetInput(m_AtlasMask);
  try
  {
    imageCaster->Update();
    labelCaster->Update();
  }
  catch (itk::ExceptionObject & exception)
  {
    itkExceptionMacro(
        "Exception caught while casting to float" << std::endl << exception)
  }

  // Geodesic Active Contour level set settings
  typedef itk::GradientAnisotropicDiffusionImageFilter< FloatImageType,
      FloatImageType > SmoothingFilterType;
  typedef itk::GradientMagnitudeRecursiveGaussianImageFilter< FloatImageType,
      FloatImageType > GradientMagFilterType;
  typedef itk::RescaleIntensityImageFilter< FloatImageType, FloatImageType > RescalerType;
  typedef itk::SigmoidImageFilter< FloatImageType, FloatImageType > SigmoidFilterType;
  typedef itk::GeodesicActiveContourLevelSetImageFilter< FloatImageType,
      FloatImageType > GeodesicActiveContourFilterType;
  typename SmoothingFilterType::Pointer smoothingFilter =
      SmoothingFilterType::New();
  typename GradientMagFilterType::Pointer gradientMagnitude =
      GradientMagFilterType::New();
  typename RescalerType::Pointer rescaler = RescalerType::New();
  typename SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();
  typename GeodesicActiveContourFilterType::Pointer geodesicActiveContour =
      GeodesicActiveContourFilterType::New();

  smoothingFilter->SetTimeStep(0.0625);
  smoothingFilter->SetNumberOfIterations(5);
  smoothingFilter->SetConductanceParameter(2.0);

  gradientMagnitude->SetSigma(1.0);

  rescaler->SetOutputMinimum(0);
  rescaler->SetOutputMaximum(255);

  sigmoid->SetOutputMinimum(0.0);
  sigmoid->SetOutputMaximum(1.0);

  geodesicActiveContour->SetIsoSurfaceValue(0.5);
  geodesicActiveContour->SetUseImageSpacing(1);

  // set parameters depending on coarse or fine isotropic resolution
  if (isoSpacing == 2)
  {
    sigmoid->SetAlpha(-2.0);
    sigmoid->SetBeta(12.0);

    geodesicActiveContour->SetMaximumRMSError(0.01);
    geodesicActiveContour->SetPropagationScaling(-2.0);
    geodesicActiveContour->SetCurvatureScaling(10.0);
    geodesicActiveContour->SetAdvectionScaling(2.0);
    geodesicActiveContour->SetNumberOfIterations(100);
  }
  if (isoSpacing == 1)
  {
    sigmoid->SetAlpha(-2.0);
    sigmoid->SetBeta(12.0);

    geodesicActiveContour->SetMaximumRMSError(0.001);
    geodesicActiveContour->SetPropagationScaling(-1.0);
    geodesicActiveContour->SetCurvatureScaling(20.0);
    geodesicActiveContour->SetAdvectionScaling(5.0);
    geodesicActiveContour->SetNumberOfIterations(120);
  }

  smoothingFilter->SetInput(imageCaster->GetOutput());
  gradientMagnitude->SetInput(smoothingFilter->GetOutput());
  rescaler->SetInput(gradientMagnitude->GetOutput());
  sigmoid->SetInput(rescaler->GetOutput());

  try
  {
    sigmoid->Update();
  }
  catch (itk::ExceptionObject & exception)
  {
    itkExceptionMacro(
        "Exception caught while preparing image" << std::endl << exception)
  }

  geodesicActiveContour->SetInput(labelCaster->GetOutput());
  geodesicActiveContour->SetFeatureImage(sigmoid->GetOutput());

  try
  {
    geodesicActiveContour->Update();
  }
  catch (itk::ExceptionObject & exception)
  {
    itkExceptionMacro(
        "Exception caught while doing GeodesicActiveContours" << std::endl << exception)
  }

  // threshold level set output
  typedef itk::BinaryThresholdImageFilter< FloatImageType, FloatImageType > ThresholdFilterType;
  typename ThresholdFilterType::Pointer thresholder =
      ThresholdFilterType::New();

  thresholder->SetUpperThreshold(0.0);
  thresholder->SetLowerThreshold(-1000.0);
  thresholder->SetOutsideValue(1);
  thresholder->SetInsideValue(0);

  thresholder->SetInput(geodesicActiveContour->GetOutput());
  try
  {
    thresholder->Update();
  }
  catch (itk::ExceptionObject & exception)
  {
    itkExceptionMacro(
        "Exception caught while doing threshold" << std::endl << exception)
  }

  // cast back mask from float to char
  typedef itk::CastImageFilter< FloatImageType, AtlasLabelType > LabelReCasterType;
  typename LabelReCasterType::Pointer labelReCaster = LabelReCasterType::New();

  labelReCaster->SetInput(thresholder->GetOutput());
  try
  {
    labelReCaster->Update();
  }
  catch (itk::ExceptionObject & exception)
  {
    itkExceptionMacro(
        "Exception caught while recasting" << std::endl << exception)
  }

  m_AtlasMask = labelReCaster->GetOutput();
  m_AtlasMask->DisconnectPipeline();
}

template< class TImageType, class TAtlasLabelType >
void StripTsImageFilter< TImageType, TAtlasLabelType >::UpsampleLabels()
{
  // upsample atlas label image to original resolution

  typedef itk::ResampleImageFilter< TAtlasLabelType, TAtlasLabelType > ResamplerType;
  typename ResamplerType::Pointer resampler = ResamplerType::New();

  typedef itk::IdentityTransform< double, 3 > TransformType;
  typename TransformType::Pointer transform = TransformType::New();

  typedef itk::NearestNeighborInterpolateImageFunction< TAtlasLabelType, double > NNInterpolatorType;
  typename NNInterpolatorType::Pointer nnInterp = NNInterpolatorType::New();

  transform->SetIdentity();

  resampler->SetTransform(transform);
  resampler->SetInput(m_AtlasMask);

  resampler->SetInterpolator(nnInterp);
  resampler->SetSize(this->GetInput()->GetLargestPossibleRegion().GetSize());
  resampler->SetOutputSpacing(this->GetInput()->GetSpacing());
  resampler->SetOutputOrigin(this->GetInput()->GetOrigin());
  resampler->SetOutputDirection(this->GetInput()->GetDirection());
  resampler->SetDefaultPixelValue(0);

  try
  {
    resampler->Update();
  }
  catch (itk::ExceptionObject & exception)
  {
    itkExceptionMacro(
        "Exception caught while resampling" << std::endl << exception)
  }

  m_AtlasMask = resampler->GetOutput();
  m_AtlasMask->DisconnectPipeline();
}

} // end namespace itk

#endif
