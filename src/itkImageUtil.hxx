/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkImageUtil_hxx
#define __itkImageUtil_hxx

#include "itkImageFileReader.h"
#include "itkImage.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImageSeriesReader.h"
#include "itkImageFileWriter.h"

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
unsigned int ImageUtil< TImage >::GetNumberOfPixels(const SizeType& sz)
{
  unsigned int total = 1;
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    total *= sz[i] * 2 + 1;
  }
  return total;
}

} // end namespace itk

#endif
