/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "imageHelpers.h"
#include "itkNeighborhoodTwoSampleKSImageFilter.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " Reference TestArea Input Output Radius [pos/neg]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string reference(argv[1]);
  std::string testMask(argv[2]);
  std::string input(argv[3]);
  std::string pvalueOutput(argv[4]);
  double R = 1;
  std::string directionFlag("pos");
  if (argc > 5) R = atof(argv[5]);
  if (argc > 6) directionFlag = argv[6];

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float ProbabilityType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef itk::NeighborhoodTwoSampleKSImageFilter< ImageType, VectorImageType, ProbabilityType, LabelType > KSFilter;

  typedef KSFilter::OutputImageType ProbabilityImageType;
  typedef KSFilter::LabelImageType LabelImageType;

  ImageType::Pointer image = CU::LoadImage< ImageType >(input);
  VectorImageType::Pointer referenceImg = CU::LoadImage< VectorImageType >(
      reference);
  LabelImageType::Pointer testMaskImg = CU::LoadImage< LabelImageType >(
      testMask);

  ImageType::SizeType radius;
  ImageType::SpacingType spacing = image->GetSpacing();
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    radius[i] = R / spacing[i];
  }

  std::cerr << "Radius is: " << radius << std::endl;

  KSFilter::Pointer ks = KSFilter::New();
  ks->SetInput(image);
  ks->SetReference(referenceImg);
  ks->SetMask(testMaskImg);
  ks->SetRadius(radius);
  if (directionFlag == "neg")
  {
    ks->PositiveOff();
    std::cerr << "Negative direction" << std::endl;
  }
  else
  {
    ks->PositiveOn();
  }
  std::cerr << "Start searching" << std::endl;
  ks->Update();
  CU::WriteImage< ProbabilityImageType >(pvalueOutput, ks->GetOutput());
  std::cerr << "Done searching" << std::endl;
  return EXIT_SUCCESS;
}

