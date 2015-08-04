/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkComposeImageFilter.h"
#include "itkImageUtil.h"

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

  typedef itk::ImageUtil< ImageType > ImageUtil;
  typedef itk::ImageUtil< VectorImageType > VectorImageUtil;

  VectorImageType::Pointer image;
  {
    typedef itk::ComposeImageFilter< ImageType, VectorImageType > ComposerType;
    ComposerType::Pointer composer = ComposerType::New();
    for (size_t i = 0; i < numberOfInputs; i++)
    {
      composer->SetInput(i, ImageUtil::ReadImage(argv[i + priorArgNumber]));
    }
    image = VectorImageUtil::GraftOutput(composer, 0);
  }

  VectorImageUtil::WriteImage(output, image);
  return EXIT_SUCCESS;
}

