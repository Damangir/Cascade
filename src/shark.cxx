/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkComposeImageFilter.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkImageRegionConstIterator.h"

#include "itkListSample.h"
#include "itkCovarianceSampleFilter.h"

#include "itkChiSquareDistribution.h"
#include "vnl/vnl_math.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

template< class ImageType >
void extractMeasurementVector(
    itk::ConstNeighborhoodIterator< ImageType >&imageIterator,
    itk::VariableLengthVector< double >&mvOut)
{
  typedef itk::ConstNeighborhoodIterator< ImageType > NITTtyep;
  typedef itk::VariableLengthVector< double > MeasurementVector;
  const unsigned int mvSize =
      imageIterator.GetImagePointer()->GetNumberOfComponentsPerPixel();
  const unsigned int nSize = imageIterator.Size();
  typedef itk::Statistics::ListSample< MeasurementVector > SampleType;
  typename SampleType::Pointer sample = SampleType::New();
  sample->SetMeasurementVectorSize(mvSize);

  MeasurementVector mv;
  mv.SetSize(mvSize);
  for (unsigned int i = 0; i < nSize; i++)
  {
    itk::NumericTraits< typename ImageType::PixelType >::AssignToArray(
        imageIterator.GetPixel(i), mv);
    sample->PushBack(mv);
  }

  typedef itk::Statistics::CovarianceSampleFilter< SampleType > CovarianceAlgorithmType;
  typename CovarianceAlgorithmType::Pointer covarianceAlgorithm =
      CovarianceAlgorithmType::New();
  covarianceAlgorithm->SetInput(sample);
  covarianceAlgorithm->Update();

  mvOut.SetSize(mvSize + mvSize * (mvSize + 1) / 2);
  for (unsigned int i = 0; i < mvSize; i++)
  {
    mvOut[i] = covarianceAlgorithm->GetMean()[i];
    for (unsigned int j = i; j < mvSize; j++)
    {
      const unsigned int index = mvSize + i * mvSize + j - i * (i + 1) / 2;
      mvOut[index] = covarianceAlgorithm->GetCovarianceMatrix()(i, j);
    }
  }
}

double pValue(const itk::VariableLengthVector< double >&ref,
              const itk::VariableLengthVector< double >&test)
{
  typedef itk::VariableLengthVector< double > MeasurementVectorType;

  const unsigned int fSize2 = 2 * ref.GetNumberOfElements();
  const unsigned int mvSize = (vcl_sqrt(9 + 4 * fSize2) - 3) / 2; // fSize = mvSize^2 + 3*mvSize; (sqrt(9 + 4*fSize) - 3) / 2

  vnl_matrix< double > m1;
  vnl_matrix< double > m2;
  vnl_matrix< double > c1;
  vnl_matrix< double > c2;

  m1.set_size(1, mvSize);
  m2.set_size(1, mvSize);
  c1.set_size(mvSize, mvSize);
  c2.set_size(mvSize, mvSize);

  for (unsigned int i = 0; i < mvSize; i++)
  {
    m1(0, i) = ref[i];
    m2(0, i) = test[i];
    for (unsigned int j = 0; j < mvSize; j++)
    {
      const unsigned int index = mvSize + i * mvSize + j - i * (i + 1) / 2;
      c1(i, j) = ref[index];
      c1(j, i) = ref[index];
      c2(i, j) = test[index];
      c2(j, i) = test[index];
    }
  }

  const double dist = vcl_sqrt(((m1 - m2) * vnl_matrix_inverse< double >((c1 + c2))
      * (m1 - m2).transpose())(0, 0));

  itk::Statistics::ChiSquareDistribution::Pointer chi =
      itk::Statistics::ChiSquareDistribution::New();
  return chi->EvaluateCDF(dist, mvSize);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " out Img1 [Img2 [Img3 ...]]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  ::itk::Object::GetGlobalWarningDisplay();

  const size_t priorArgNumber = 2;
  std::string pvalueOutput(argv[1]);
  const size_t numberOfModalities = argc - priorArgNumber;

  std::cerr << "Number of modalities are " << numberOfModalities << std::endl;

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef float ProbabilityType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef itk::Image< ProbabilityType, ImageDimension > ProbabilityImageType;

  VectorImageType::Pointer image;
  {
    typedef itk::ComposeImageFilter< ImageType, VectorImageType > ComposerType;
    ComposerType::Pointer composer = ComposerType::New();
    for (size_t i = 0; i < numberOfModalities; i++)
    {
      composer->SetInput(i,
                         CU::LoadImage< ImageType >(argv[i + priorArgNumber]));
    }
    image = CU::GraftOutput< ComposerType >(composer, 0);
  }

  typedef VectorImageType::PixelType MeasurementVectorType;
  typedef itk::VariableLengthVector< double > FeatureType;

  const double r1 = 2;
  const double r2 = 4;
  VectorImageType::RegionType region = image->GetLargestPossibleRegion();
  VectorImageType::SizeType radius1;
  VectorImageType::SizeType radius2;
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    radius1[i] = r1 / image->GetSpacing()[i];
    radius2[i] = r2 / image->GetSpacing()[i];
  }

  itk::ConstNeighborhoodIterator< VectorImageType > imageIteratorSmall(radius1,
                                                                       image,
                                                                       region);
  itk::ConstNeighborhoodIterator< VectorImageType > imageIteratorBig(radius2,
                                                                     image,
                                                                     region);

  ProbabilityImageType::Pointer outImage = ProbabilityImageType::New();
  outImage->CopyInformation(image);
  outImage->SetRequestedRegion(image->GetRequestedRegion());
  outImage->SetBufferedRegion(image->GetBufferedRegion());
  outImage->Allocate();
  itk::ImageRegionIterator< ProbabilityImageType > outIterator(outImage,
                                                               region);

  FeatureType featSmall;
  FeatureType featBig;
  featSmall.SetSize(
      numberOfModalities + numberOfModalities * (numberOfModalities + 1) / 2);
  featBig.SetSize(featSmall.GetSize());

  std::cerr << "Start comparing features" << std::endl;
  while (!imageIteratorSmall.IsAtEnd())
  {
    {
      extractMeasurementVector< VectorImageType >(imageIteratorSmall,
                                                  featSmall);
      extractMeasurementVector< VectorImageType >(imageIteratorBig, featBig);
      const double currentPValue = pValue(featBig, featSmall);
      outIterator.Set(static_cast< ProbabilityType >(currentPValue));
    }
    ++outIterator;
    ++imageIteratorSmall;
    ++imageIteratorBig;
  }

  std::cerr << "Done comparing features" << std::endl;

  CU::WriteImage< ProbabilityImageType >(pvalueOutput, outImage);
  return EXIT_SUCCESS;
}
