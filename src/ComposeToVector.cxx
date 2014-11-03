/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkComposeImageFilter.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " VectorOutput Img1 [Img2 [Img3 ...]]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  const size_t priorArgNumber = 2;
  std::string output(argv[1]);
  const size_t numberOfInputs = argc - priorArgNumber;

  std::cerr << "Number of inputs are " << numberOfInputs << std::endl;

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;

  VectorImageType::Pointer image;
  {
    typedef itk::ComposeImageFilter< ImageType, VectorImageType > ComposerType;
    ComposerType::Pointer composer = ComposerType::New();
    for (size_t i = 0; i < numberOfInputs; i++)
    {
      composer->SetInput(i,
                         CU::LoadImage< ImageType >(argv[i + priorArgNumber]));
    }
    image = CU::GraftOutput< ComposerType >(composer, 0);
  }

  CU::WriteImage< VectorImageType >(output, image);
  return EXIT_SUCCESS;
}

