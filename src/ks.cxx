/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkBinaryFunctorImageFilter.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

namespace itk
{
namespace Functor
{

template< typename TFeature1, typename TFeature2 = TFeature1,
    typename TOutput = float >
class FeatureDifferenceTest
{
public:
  typedef FeatureDifferenceTest Self;
  typedef NumericTraits< TFeature1 > Feature1Trait;
  typedef NumericTraits< TFeature2 > Feature2Trait;
  typedef NumericTraits< TOutput > OutputTrait;

  FeatureDifferenceTest()
  {
  }
  ;
  ~FeatureDifferenceTest()
  {
  }
  ;
  bool operator!=(const Self &) const
  {
    return false;
  }
  bool operator==(const Self & other) const
  {
    return !(*this != other);
  }
  inline TOutput operator()(const TFeature1 & A, const TFeature2 & B)
  {
    size_t nA = Feature1Trait::GetLength(A);
    size_t nB = Feature2Trait::GetLength(B);

    TOutput ksTest = 0;
    // Assuming unit variance for each peak Gaussian
    for (size_t i = 1; i < nA; i++)
    {
      ksTest += (2.0 * (A[i] - B[i])) / ((A[i] + B[i]) * nA);
    }
    return ksTest > 0? ksTest:0;
  }
};
}
}

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " featureImage model output";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string subjectImage(argv[1]);
  std::string modelImage(argv[2]);
  std::string output(argv[3]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef float Probability;

  typedef itk::VectorImage< PixelType, ImageDimension > FeatureImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > ModelImageType;
  typedef itk::Image< Probability, ImageDimension > PValueImageType;

  FeatureImageType::Pointer subjectImg = CU::LoadImage< FeatureImageType >(
      subjectImage);
  ModelImageType::Pointer modelImg = CU::LoadImage< ModelImageType >(
      modelImage);

  itkAssertOrThrowMacro(
      subjectImg->GetNumberOfComponentsPerPixel() == modelImg->GetNumberOfComponentsPerPixel(),
      "Feature image and model image should have the same number of components per pixel");

  typedef itk::BinaryFunctorImageFilter< FeatureImageType, ModelImageType,
      PValueImageType,
      itk::Functor::FeatureDifferenceTest< FeatureImageType::PixelType,
          ModelImageType::PixelType > > KSFilterType;

  KSFilterType::Pointer KSFilter = KSFilterType::New();
  KSFilter->SetInput1(subjectImg);
  KSFilter->SetInput2(modelImg);
  CU::WriteImage< PValueImageType >(output, KSFilter->GetOutput());

  return EXIT_SUCCESS;
}
