/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkImageUtil_hxx
#define __itkImageUtil_hxx

#include "itkImageFileReader.h"
#include "itkImage.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileWriter.h"

#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryMorphologicalClosingImageFilter.h"
#include "itkBinaryMorphologicalOpeningImageFilter.h"

#include "itkImageToHistogramFilter.h"

namespace itk
{

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::ReadImage(
    std::string filename)
{
  ImagePointer image;
  try
  {
    image = Read3DImage(filename);
    //itkDebugMacro(<<"Input is " << ImageType::ImageDimension << "D image");
  }
  catch (itk::ExceptionObject &ex)
  {
    try
    {
      image = ReadDICOMImage(filename)[0];
      //itkDebugMacro(<< "Input is " << ImageType::ImageDimension << "D DICOM series");
    }
    catch (itk::ExceptionObject &ex)
    {
      std::ostringstream message;
      message << "itk::ERROR: ImageUtil : " << "Can not read " << filename
          << ". Only 3D images and DIOM series can be read.";
      ::itk::ExceptionObject e_(__FILE__, __LINE__, message.str().c_str(),
                                ITK_LOCATION);
      throw e_;
    }
  }

  return image;
}

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::ReadMGHImage(
    std::string filename)
{
  typedef GDCMImageIO ImageIOType;
  ImageIOType::Pointer dicomIO = ImageIOType::New();
  typedef ImageFileReader< ImageType > ImageReaderType;
  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(filename);
  ImagePointer image;
  image = reader->GetOutput();
  image->Update();
  image->DisconnectPipeline();
  return image;

}

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::Read3DImage(
    std::string filename)
{
  typedef ImageFileReader< ImageType > ImageReaderType;
  typename ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(filename);
  ImagePointer image;
  image = reader->GetOutput();
  image->Update();
  image->DisconnectPipeline();
  return image;
}

template< typename TImage >
std::vector< typename ImageUtil< TImage >::ImagePointer > ImageUtil< TImage >::ReadDICOMImage(
    std::string filename)
{
  std::vector< ImagePointer > images;

  typedef ImageSeriesReader< ImageType > ReaderType;
  typename ReaderType::Pointer reader = ReaderType::New();

  typedef GDCMImageIO ImageIOType;
  ImageIOType::Pointer dicomIO = ImageIOType::New();
  reader->SetImageIO(dicomIO);

  typedef itk::GDCMSeriesFileNames NamesGeneratorType;
  NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
  nameGenerator->SetUseSeriesDetails(true);
  nameGenerator->AddSeriesRestriction("0008|0021");
  nameGenerator->SetDirectory(filename);

  typedef std::vector< std::string > SeriesIdContainer;
  typedef std::vector< std::string > SeriesIdContainer;
  const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
  SeriesIdContainer::const_iterator seriesItr = seriesUID.begin();
  SeriesIdContainer::const_iterator seriesEnd = seriesUID.end();
  while (seriesItr != seriesEnd)
  {
    std::string seriesIdentifier = *seriesItr;
    //itkDebugMacro(<< "Reading " << seriesIdentifier);
    reader->SetFileNames(nameGenerator->GetFileNames(seriesIdentifier));
    ImagePointer image;
    image = reader->GetOutput();
    image->Update();
    image->DisconnectPipeline();
    images.push_back(image);
    ++seriesItr;
  }
  return images;
}

template< typename TImage >
void ImageUtil< TImage >::WriteImage(std::string filename, const TImage* image)
{
  typedef ImageFileWriter< TImage > ImageWriterType;
  typename ImageWriterType::Pointer writer = ImageWriterType::New();
  writer->SetFileName(filename);
  writer->SetInput(image);
  writer->Update();
}

template< typename TImage >
typename ImageUtil< TImage >::SizeType ImageUtil< TImage >::GetRadiusFromPhysicalSize(
    const TImage* img, float r)
{
  SizeType radius;
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    radius[i] = r / img->GetSpacing()[i];
  }
  return radius;
}

template< typename TImage >
float ImageUtil< TImage >::GetPhysicalPixelSize(const TImage* img)
{
  float s = 1;
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    s *= img->GetSpacing()[i];
  }
  return s;
}

template< typename TImage >
unsigned int ImageUtil< TImage >::GetNumberOfPixels(const SizeType& sz)
{
  unsigned int total = 1;
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    total *= sz[i] * 2 + 1;
  }
  return total;
}

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::GraftOutput(
    ImageSource< ImageType >* imgSource, size_t index)
{
  ImagePointer image = imgSource->GetOutput(index);
  image->Update();
  image->DisconnectPipeline();
  return image;
}

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::Erode(
    const ImageType* img, float r, float foreground)
{
  typedef BinaryBallStructuringElement< PixelType, ImageDimension > Structure;
  typedef BinaryErodeImageFilter< ImageType, ImageType, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(Self::GetRadiusFromPhysicalSize(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  morph->SetBackgroundValue(0);
  ImagePointer output = Self::GraftOutput(morph, 0);
  return output;
}

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::Dilate(
    const ImageType* img, float r, float foreground)
{
  typedef BinaryBallStructuringElement< PixelType, ImageDimension > Structure;
  typedef BinaryDilateImageFilter< ImageType, ImageType, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(Self::GetRadiusFromPhysicalSize(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  morph->SetBackgroundValue(0);
  ImagePointer output = Self::GraftOutput(morph, 0);
  return output;
}

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::Opening(
    const ImageType* img, float r, float foreground)
{
  typedef BinaryBallStructuringElement< PixelType, ImageDimension > Structure;
  typedef BinaryMorphologicalOpeningImageFilter< ImageType, ImageType, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(Self::GetRadiusFromPhysicalSize(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  morph->SetBackgroundValue(0);
  ImagePointer output = Self::GraftOutput(morph, 0);
  return output;
}

template< typename TImage >
typename ImageUtil< TImage >::ImagePointer ImageUtil< TImage >::Closing(
    const ImageType* img, float r, float foreground)
{
  typedef BinaryBallStructuringElement< PixelType, ImageDimension > Structure;
  typedef BinaryMorphologicalClosingImageFilter< ImageType, ImageType, Structure > MorphFilter;
  Structure structure;
  structure.SetRadius(Self::GetRadiusFromPhysicalSize(img, r));
  structure.CreateStructuringElement();
  typename MorphFilter::Pointer morph = MorphFilter::New();
  morph->SetInput(img);
  morph->SetKernel(structure);
  morph->SetForegroundValue(foreground);
  morph->SetBackgroundValue(0);
  ImagePointer output = Self::GraftOutput(morph, 0);
  return output;
}

template< typename TImage >
void ImageUtil< TImage >::ImageExtent(const ImageType* img, PixelType& minValue,
                                      PixelType& maxValue, double quantile)
{
  typedef Statistics::ImageToHistogramFilter< ImageType > HistogramFilterType;
  typename HistogramFilterType::Pointer histogramFilter =
      HistogramFilterType::New();
  typename HistogramFilterType::HistogramSizeType size(1);
  size[0] = 255;        // number of bins for the Red   channel
  histogramFilter->SetHistogramSize(size);
  histogramFilter->SetMarginalScale(10.0);
  histogramFilter->SetInput(img);
  histogramFilter->Update();
  typename HistogramFilterType::HistogramPointer hist =
      histogramFilter->GetOutput();
  hist->SetFrequency(0, 0);
  const unsigned int histogramSize = hist->Size();

  minValue = hist->Quantile(0, quantile);
  maxValue = hist->Quantile(0, 1 - quantile);
}

} // end namespace itk

#endif
