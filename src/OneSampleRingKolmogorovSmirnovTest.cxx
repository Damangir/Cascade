/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkConstNeighborhoodIterator.h"
#include "imageHelpers.h"
#include "itkNeighborhoodRingOneSampleKSImageFilter.h"
namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " TestArea Input Output InnerRadius OuterRadius [pos/neg]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string testMask(argv[1]);
  std::string input(argv[2]);
  std::string pvalueOutput(argv[3]);
  double outerR = 1;
  double innerR = 1;
  std::string directionFlag("pos");
  if (argc > 4) innerR = atof(argv[4]);
  if (argc > 5) outerR = atof(argv[5]);
  if (argc > 6) directionFlag = argv[6];

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float ProbabilityType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::NeighborhoodRingOneSampleKSImageFilter< ImageType, ProbabilityType, LabelType > KSFilter;

  typedef KSFilter::OutputImageType ProbabilityImageType;
  typedef KSFilter::LabelImageType LabelImageType;

  LabelImageType::Pointer testMaskImg = CU::LoadImage< LabelImageType >(
      testMask);

  ImageType::Pointer image = CU::LoadImage< ImageType >(input);

  ImageType::RegionType region = image->GetLargestPossibleRegion();
  ImageType::SizeType innerRadius;
  ImageType::SizeType outerRadius;
  ImageType::SpacingType spacing = image->GetSpacing();
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    innerRadius[i] = innerR / spacing[i];
    outerRadius[i] = outerR / spacing[i];
  }

  std::cerr << "Outer Radius is: " << outerRadius << std::endl;
  std::cerr << "Inenr Radius is: " << innerRadius << std::endl;

  KSFilter::Pointer ks = KSFilter::New();
  ks->SetInput(image);
  ks->SetMask(testMaskImg);
  ks->SetRadius(outerRadius);
  ks->SetInnerRadius(innerRadius);
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

