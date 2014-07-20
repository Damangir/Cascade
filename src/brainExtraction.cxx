/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkStripTsImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

int main( int argc, char *argv[] )
{
  if( argc < 4 )
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image  stdBrainMask outputBrainMask";
    std::cerr << std::endl;
    return EXIT_FAILURE;
    }

  std::string subjectImage(argv[1]);
  std::string standardBrainMask(argv[2]);
  std::string outputBrainMask(argv[3]);

  const    unsigned int    ImageDimension = 3;
  typedef  signed short    PixelType;
  typedef itk::Image< PixelType, ImageDimension >  ImageType;
  typedef itk::Image< PixelType, ImageDimension >  MaskImageType;
  const unsigned int SpaceDimension = ImageDimension;
  const unsigned int SplineOrder = 3;

  typedef itk::ImageFileReader< ImageType  > ImageReaderType;
  typedef itk::ImageFileReader< MaskImageType > MaskImageReaderType;
  ImageReaderType::Pointer  imageReader  = ImageReaderType::New();
  MaskImageReaderType::Pointer atlasBrainReader = MaskImageReaderType::New();
  imageReader->SetFileName(  subjectImage );
  atlasBrainReader->SetFileName( standardBrainMask );

  imageReader->Update();
  atlasBrainReader->Update();

  typedef itk::StripTsImageFilter<ImageType, MaskImageType> BrainExtractorType;
  BrainExtractorType::Pointer brainExtractor = BrainExtractorType::New();

  brainExtractor->SetSubjectImage(imageReader->GetOutput());
  brainExtractor->SetAtlasBrainMask(atlasBrainReader->GetOutput());

  brainExtractor->Update();

  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetInput(brainExtractor->GetOutput());
  writer->SetFileName( outputBrainMask );
  writer->Update();

  return EXIT_SUCCESS;
}
