/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkConstNeighborhoodIterator.h"
#include "imageHelpers.h"
#include "itkNeighborhoodOneSampleKSImageFilter.h"
namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " InclusionArea TestArea Input Output Radius [pos/neg]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string trainMask(argv[1]);
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
  typedef itk::NeighborhoodOneSampleKSImageFilter< ImageType, ProbabilityType, LabelType > KSFilter;

  typedef KSFilter::OutputImageType ProbabilityImageType;
  typedef KSFilter::LabelImageType LabelImageType;

  LabelImageType::Pointer trainMaskImg = CU::LoadImage< LabelImageType >(
      trainMask);
  LabelImageType::Pointer testMaskImg = CU::LoadImage< LabelImageType >(
      testMask);

  ImageType::Pointer image = CU::LoadImage< ImageType >(input);

  ImageType::RegionType region = image->GetLargestPossibleRegion();
  ImageType::SizeType radius;
  ImageType::SpacingType spacing = image->GetSpacing();
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    radius[i] = R / spacing[i];
  }

  std::cerr << "Radius is: " << radius << std::endl;
  const size_t num_train = CU::CountNEq< LabelImageType >(trainMaskImg, 0);
  std::cerr << "Number of train: " << num_train << std::endl;

  std::cerr << "Start setting up reference CDF" << std::endl;
  KSFilter::DistributionType refrence;
  size_t testLength = 0;
  {
    itk::ConstNeighborhoodIterator< LabelImageType > trainMaskIterator(
        radius, trainMaskImg, region);
    itk::ConstNeighborhoodIterator< ImageType > imgIterator(radius, image,
                                                            region);
    testLength = imgIterator.Size();
    while (!imgIterator.IsAtEnd())
    {
      if (trainMaskIterator.GetCenterPixel() > itk::NumericTraits< LabelType >::Zero)
      {
        if (rand() % (num_train / 100) == 0)
        {
          for (size_t i = 0; i < testLength; i++)
          {
            if (trainMaskIterator.GetPixel(i) > itk::NumericTraits< LabelType >::Zero)
            {
              refrence.push_back(imgIterator.GetPixel(i));
            }
          }
        }
      }
      ++trainMaskIterator;
      ++imgIterator;
    }
  }
  std::sort(refrence.begin(), refrence.end());
  std::cerr << "Done setting up reference CDF" << std::endl;

  std::cerr << "["<< refrence.front() << ", " << refrence.back() << "]" << std::endl;

  //return 0;
  KSFilter::Pointer ks = KSFilter::New();
  ks->SetRefrenceDistribution(refrence);
  ks->SetInput(image);
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

