/*
 * Copyright (C) 2013 Soheil Damangir - All Rights Reserved
 * You may use and distribute, but not modify this code under the terms of the
 * Creative Commons Attribution-NonCommercial-NoDerivs 3.0 Unported License
 * under the following conditions:
 *
 * Attribution: You must attribute the work in the manner specified by the
 * author or licensor (but not in any way that suggests that they endorse you
 * or your use of the work).
 * Noncommercial: You may not use this work for commercial purposes.
 * No Derivative Works: You may not alter, transform, or build upon this
 * work
 *
 * To view a copy of the license, visit
 * http://creativecommons.org/licenses/by-nc-nd/3.0/
 */

#ifndef IMAGEHELPERS_H_
#define IMAGEHELPERS_H_

#include <string>
#include <iostream>

#include "itkImage.h"
#include "itkMaskImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageMaskSpatialObject.h"
#include "itkExtractImageFilter.h"
#include "itkMinimumMaximumImageCalculator.h"

#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryMorphologicalClosingImageFilter.h"
#include "itkBinaryMorphologicalOpeningImageFilter.h"

#include "itkVectorIndexSelectionCastImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkAddImageFilter.h"
#include "itkMultiplyImageFilter.h"

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#define ReportFilterMacro(FILTER) {\
  FILTER->Update();\
  ::cascade::util::WriteImage( "debug/" #FILTER LINE_STRING ".nii.gz" , FILTER->GetOutput());\
}
#define ReportImage(IMAGE) {\
  ::cascade::util::WriteImage( "debug/" #IMAGE LINE_STRING ".nii.gz" , IMAGE.GetPointer());\
}

#define ReportVectorImage(FILTER){\
  FILTER->Update();\
  ::cascade::util::WriteVectorImage( "debug/" #FILTER LINE_STRING "_" , FILTER->GetOutput());\
}

#define DispatchFilterOutput(filterName, ImageType)\
  {ImageType image;\
  image = filterName->GetOutput();\
  image->Update();\
  image->DisconnectPipeline();\
  return image;}\


#define BinaryFilterAsFunctionImage(FILTERNAME) \
template< class ImageT, class ImageT2 >\
typename ImageT::Pointer FILTERNAME(const ImageT* img1, const ImageT2* img2)\
{\
  typedef itk::FILTERNAME##ImageFilter< ImageT, ImageT2, ImageT > FilterType;\
  typename FilterType::Pointer filter = FilterType::New();\
  filter->SetInput1(img1);\
  filter->SetInput2(img2);\
  DispatchFilterOutput(filter, typename ImageT::Pointer)\
}

#define BinaryFilterAsFunctionConstant(FILTERNAME) \
template< class ImageT, class PixelT2 >\
typename ImageT::Pointer FILTERNAME##Constant(const ImageT* img1, const PixelT2& pixel2)\
{\
  typedef itk::Image<PixelT2, ImageT::ImageDimension> ImageT2;\
  typedef itk::FILTERNAME##ImageFilter< ImageT, ImageT2, ImageT > FilterType;\
  typename FilterType::Pointer filter = FilterType::New();\
  filter->SetInput1(img1);\
  filter->SetConstant2(pixel2);\
  DispatchFilterOutput(filter, typename ImageT::Pointer)\
}

#define BinaryFilterAsFunction(FILTERNAME) \
BinaryFilterAsFunctionImage(FILTERNAME) \
BinaryFilterAsFunctionConstant(FILTERNAME) \


namespace cascade
{

namespace util
{

template< typename FilterT >
typename FilterT::OutputImageType::Pointer GraftOutput(typename FilterT::Pointer filter, unsigned int index = 0)
{
  typename FilterT::OutputImageType::Pointer image;
  image = filter->GetOutput(index);
  image->Update();
  image->DisconnectPipeline();
  return image;
}

BinaryFilterAsFunction(Add);
BinaryFilterAsFunction(Subtract);
BinaryFilterAsFunction(Multiply);


template< class ImageT >
itk::Size< ImageT::ImageDimension > GetPhysicalRadius(const ImageT* img, float r)
{
  itk::Size< ImageT::ImageDimension > radius;
  for (unsigned int i = 0; i < ImageT::ImageDimension; i++)
  {
    radius[i] = r / img->GetSpacing()[i];
  }
  return radius;
}

template< unsigned int VImageDimension >
unsigned int GetNumberOfPixels(const itk::Size< VImageDimension >& sz)
{
  unsigned int total = 1;
  for (unsigned int i = 0; i < VImageDimension; i++)
  {
    total *= sz[i]*2 + 1;
  }
  return total;
}
template< class ImageT >
typename ImageT::Pointer Erode(const ImageT* img, float r, float foreground = 1)
{
  typedef itk::BinaryBallStructuringElement< typename ImageT::PixelType,
      ImageT::ImageDimension > Structure;
  typedef itk::BinaryErodeImageFilter< ImageT, ImageT, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(GetPhysicalRadius(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  morph->SetBackgroundValue(0);
  DispatchFilterOutput(morph, typename ImageT::Pointer);
}
template< class ImageT >
typename ImageT::Pointer Dilate(const ImageT* img, float r,
                                float foreground = 1)
{
  typedef itk::BinaryBallStructuringElement< typename ImageT::PixelType,
      ImageT::ImageDimension > Structure;
  typedef itk::BinaryDilateImageFilter< ImageT, ImageT, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(GetPhysicalRadius(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  morph->SetBackgroundValue(0);
  DispatchFilterOutput(morph, typename ImageT::Pointer);
}
template< class ImageT >
typename ImageT::Pointer Closing(const ImageT* img, float r, float foreground = 1)
{
  typedef itk::BinaryBallStructuringElement< typename ImageT::PixelType,
      ImageT::ImageDimension > Structure;
  typedef itk::BinaryMorphologicalClosingImageFilter< ImageT, ImageT, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(GetPhysicalRadius(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  DispatchFilterOutput(morph, typename ImageT::Pointer);
}
template< class ImageT >
typename ImageT::Pointer Opening(const ImageT* img, float r, float foreground = 1)
{
  typedef itk::BinaryBallStructuringElement< typename ImageT::PixelType,
      ImageT::ImageDimension > Structure;
  typedef itk::BinaryMorphologicalOpeningImageFilter< ImageT, ImageT, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(GetPhysicalRadius(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  DispatchFilterOutput(morph, typename ImageT::Pointer);
}
template< class ImageT >
typename ImageT::RegionType ImageLargestNonZeroRegion(const ImageT* image)
{
  typedef ::itk::ImageMaskSpatialObject< ImageT::ImageDimension > ImageMaskSpatialObjectType;
  typedef ::itk::BinaryThresholdImageFilter< ImageT,
      typename ImageMaskSpatialObjectType::ImageType > ThresholdFilterType;

  typename ThresholdFilterType::Pointer threshold = ThresholdFilterType::New();
  threshold->SetLowerThreshold(1);
  threshold->SetInput(image);
  threshold->SetOutsideValue(0);
  threshold->SetInsideValue(1);
  threshold->Update();

  typename ImageMaskSpatialObjectType::Pointer imageMaskSpatialObject =
      ImageMaskSpatialObjectType::New();

  imageMaskSpatialObject->SetImage(threshold->GetOutput());
  ::itk::ImageRegion< ImageT::ImageDimension > boundingBoxRegion =
      imageMaskSpatialObject->GetAxisAlignedBoundingBoxRegion();

  return boundingBoxRegion;
}

template< class ImageT >
std::vector< typename ImageT::PixelType > ImageMinimumMaximum(
    const ImageT* image)
{
  typedef ::itk::MinimumMaximumImageCalculator< ImageT > ImageCalculatorFilterType;
  typename ImageCalculatorFilterType::Pointer imageCalculatorFilter =
      ImageCalculatorFilterType::New();
  imageCalculatorFilter->SetImage(image);
  imageCalculatorFilter->Compute();
  std::vector< typename ImageT::PixelType > output;
  output.resize(2);
  output[0] = imageCalculatorFilter->GetMinimum();
  output[1] = imageCalculatorFilter->GetMaximum();
  return output;
}

template< class ImageT >
typename ImageT::Pointer CropImage(const ImageT* image,
                                   typename ImageT::RegionType desiredRegion)
{
  typedef ::itk::ExtractImageFilter< ImageT, ImageT > CroppingFilterType;
  typename CroppingFilterType::Pointer croppingFilter =
      CroppingFilterType::New();
  croppingFilter->SetExtractionRegion(desiredRegion);
  croppingFilter->SetInput(image);
#if ITK_VERSION_MAJOR >= 4
  croppingFilter->SetDirectionCollapseToIdentity();
#endif
  DispatchFilterOutput(croppingFilter, typename ImageT::Pointer);
}
template< class ImageT >
typename ImageT::Pointer LoadImage(std::string filename)
{
  typedef ::itk::ImageFileReader< ImageT > ImageReaderType;
  typename ImageT::Pointer image;
  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(filename);
  DispatchFilterOutput(reader, typename ImageT::Pointer);
}

template< class ImageT >
void WriteImage(std::string filename, const ImageT* image)
{
  typedef ::itk::ImageFileWriter< ImageT > ImageWriterType;
  typename ImageWriterType::Pointer writer = ImageWriterType::New();
  writer->SetFileName(filename);
  writer->SetInput(image);
  writer->Update();
}

template< class ImageT >
void WriteVectorImage(std::string filename, const ImageT* image)
{
  unsigned int nCmp = image->GetNumberOfComponentsPerPixel();
  for (unsigned int i = 0; i < nCmp; i++)
  {
    std::ostringstream fname;
    fname << filename << i << ".nii.gz";
    ::cascade::util::WriteImage(fname.str(),
                                NthElementVectorImage(image, i).GetPointer());
  }
}

template< class ImageT >
typename itk::Image< typename ImageT::InternalPixelType, ImageT::ImageDimension >::Pointer NthElementVectorImage(
    const ImageT* img, const unsigned int n)
{
  typedef itk::Image< typename ImageT::InternalPixelType, ImageT::ImageDimension > ScalarImageType;
  typedef itk::VectorIndexSelectionCastImageFilter< ImageT, ScalarImageType > IndexSelectionType;
  typename IndexSelectionType::Pointer indexSelectionFilter =
      IndexSelectionType::New();
  indexSelectionFilter->SetInput(img);
  indexSelectionFilter->SetIndex(n);
  DispatchFilterOutput(indexSelectionFilter, typename ScalarImageType::Pointer);
}

template< class ImageT, class ScalarImageT >
typename ImageT::Pointer Mask(const ImageT* img, const ScalarImageT* mask)
{
  typedef ::itk::MaskImageFilter< ImageT, ScalarImageT, ImageT > MaskType;
  typename MaskType::Pointer maskFilter = MaskType::New();
  maskFilter->SetInput(img);
  maskFilter->SetMaskImage(mask);
  DispatchFilterOutput(maskFilter, typename ImageT::Pointer);
}

template< class ImageT2, class ImageT >
typename ImageT2::Pointer Cast(const ImageT* img1)
{
  typedef ::itk::CastImageFilter< ImageT, ImageT2 > CastType;
  typename CastType::Pointer castFilter = CastType::New();
  castFilter->SetInput(img1);
  DispatchFilterOutput(castFilter, typename ImageT2::Pointer);
}


template<class HistIteratorT>
float histogramMode(HistIteratorT iter,  HistIteratorT iterEnd)
{
  float maxFreq = 0;
  float maxFreqIntensity = 0;
  while (iter != iterEnd)
  {
    if(maxFreq < iter.GetFrequency())
    {
      maxFreqIntensity = iter.GetMeasurementVector()[0];
      maxFreq =  iter.GetFrequency();
    }
    ++iter;
  }
  return maxFreqIntensity;
}


}  // namespace util

}  // namespace cascade

#endif /* IMAGEHELPERS_H_ */
