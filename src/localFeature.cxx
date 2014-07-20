/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkVectorImage.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkComposeImageFilter.h"
#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image mask output [radius] [levels]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string subjectImage(argv[1]);
  std::string mask(argv[2]);
  std::string output(argv[3]);
  float radius = 2;
  float levels = 5;
  if (argc > 4) radius = atof(argv[4]);
  if (argc > 5) levels = atof(argv[5]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef itk::Image< LabelType, ImageDimension > ClassifidImageType;

  typedef itk::DiscreteGaussianImageFilter< ImageType, ImageType > SmoothFilterType;
  typedef itk::ComposeImageFilter< ImageType, VectorImageType > ComposeFilterType;

  ComposeFilterType::Pointer composeFilter = ComposeFilterType::New();

  SmoothFilterType::Pointer smoother = SmoothFilterType::New();
  smoother->SetInput(
      CU::Mask< ImageType, ClassifidImageType >(
          CU::LoadImage< ImageType >(subjectImage),
          CU::LoadImage< ClassifidImageType >(mask)));

  for (unsigned int level = 0; level < levels; level++)
  {
    smoother->SetVariance(level * radius);
    ImageType::Pointer levelImg = CU::GraftOutput< SmoothFilterType >(smoother);
    composeFilter->SetInput(level, levelImg);
  }

  CU::WriteImage< VectorImageType >(output, composeFilter->GetOutput());

  return EXIT_SUCCESS;
}
