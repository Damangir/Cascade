/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkN4ImageFilter.h"
#include "imageHelpers.h"

namespace CU = cascade::util;


int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input mask output";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string inputImage(argv[1]);
  std::string maskImage(argv[2]);
  std::string outputImage(argv[3]);

  const unsigned int ImageDimension = 3;
  typedef float PixelType;
  typedef double CoordinateRepType;
  const unsigned int SpaceDimension = ImageDimension;

  typedef itk::Image< PixelType, ImageDimension > ImageType;

  typedef itk::N4ImageFilter<ImageType> N4CorrectorType;
  N4CorrectorType::Pointer n4Corrector = N4CorrectorType::New();
  n4Corrector->SetInput(0,CU::LoadImage<ImageType>(inputImage));
  n4Corrector->SetInput(1,CU::LoadImage<ImageType>(maskImage));
  CU::WriteImage<ImageType>(outputImage, n4Corrector->GetOutput());

  return EXIT_SUCCESS;
}
