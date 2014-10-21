/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkComposeImageFilter.h"
#include "itkConstNeighborhoodIterator.h"
#include "itkImageRegionConstIterator.h"

#include "itkListSample.h"
#include "itkWeightedCentroidKdTreeGenerator.h"

#include "itkCovarianceSampleFilter.h"
#include "itkGaussianMembershipFunction.h"
#include "itkChiSquareDistribution.h"
#include "vnl/vnl_math.h"

#include "itkMahalanobisDistanceMetric.h"
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

double bhattacharyyaCoef(const vnl_matrix< double >& m1,
                         const vnl_matrix< double >& m2,
                         const vnl_matrix< double >& c1,
                         const vnl_matrix< double >& c2)
{
  /*
   * Estimate the overlap between two gaussians.
   */
  vnl_matrix< double > c = (c1 + c2) / 2;
  vnl_svd< double > c_svd(c);

  const double mahDist = ((m1 - m2) * c_svd.inverse() * (m1 - m2).transpose())(
      0, 0);
  const double varDiff = vcl_log(c_svd.determinant_magnitude() /
  (vnl_svd<double>(c1).determinant_magnitude() +
      vnl_svd<double>(c2).determinant_magnitude()));

  const double bhattacharyyaDist = 0.125 * mahDist + 0.5 * varDiff;

  return vcl_exp(-bhattacharyyaDist);
}

double pValue(const vnl_matrix< double >& m1, const vnl_matrix< double >& m2,
              const vnl_matrix< double >& c1, const vnl_matrix< double >& c2)
{
  typedef vnl_svd< double > SVD;
  /*
   * Number of sample point in integration
   */
  const size_t df = c1.cols();
  const size_t n = 10;
  double spread = 2;

  itk::Statistics::ChiSquareDistribution::Pointer chiSquare =
      itk::Statistics::ChiSquareDistribution::New();

  const double dx = 1.0 / n;
  double pv = 0;
  double preFactor;
  vnl_matrix< double > x(m1.rows(), m1.cols());

  SVD svd1(c1);
  SVD svd2(c2);

  std::vector< vnl_matrix< double > > eigv1(df, m1);
  std::vector< double > idx(df, 0);

  for (size_t a = 0; a < df; a++)
  {
    for (size_t b = 0; b < df; b++)
    {
      eigv1[a][0][b] =svd1.U(b,a) * svd1.W(a);
    }
  }

  double total_n = vcl_pow(n, df);

  for (size_t i = 0; i < total_n; i++)
  {
    size_t idx = i;
    x = m2;
    for (size_t b = 0; b < df; b++)
    {
      double spread_b = spread * 2 * ((idx % n) / n - 0.5);
      x += spread_b * eigv1[b];
      idx /= n;
    }

    const double mahDist = ((m1 - x) * svd1.inverse() * (m1 - x).transpose())(
        0, 0);

    const double g = preFactor * vcl_exp(-0.5*((m2 - x) * svd2.inverse() * (m2 - x).transpose())(0, 0));
    pv += chiSquare->CDF(mahDist, df) * g;
  }

  return pv;
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

  return pValue(m1, m2, c1, c2);
}

int main(int argc, char *argv[])
{
  if (argc < 6)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " inclusion test out Img1 [Img2 [Img3 ...]]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  ::itk::Object::GetGlobalWarningDisplay();

  const size_t priorArgNumber = 4;
  std::string trainMask(argv[1]);
  std::string testMask(argv[2]);
  std::string pvalueOutput(argv[3]);
  const size_t numberOfModalities = argc - priorArgNumber;

  std::cerr << "Number of modalities are " << numberOfModalities << std::endl;

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float ProbabilityType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef itk::Image< ProbabilityType, ImageDimension > ProbabilityImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  LabelImageType::Pointer trainMaskImg = CU::LoadImage< LabelImageType >(
      trainMask);
  LabelImageType::Pointer testMaskImg = CU::LoadImage< LabelImageType >(
      testMask);

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
  typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
  typedef itk::Statistics::ListSample< FeatureType > IndexSampleType;

  VectorImageType::RegionType region = image->GetLargestPossibleRegion();
  VectorImageType::SizeType radius;
  for (unsigned int i = 0; i < ImageDimension; i++)
  {
    radius[i] = 2.0 / image->GetSpacing()[i];
  }

  itk::ConstNeighborhoodIterator< VectorImageType > imageIterator(radius, image,
                                                                  region);

  itk::ImageRegionConstIterator< LabelImageType > trainMaskIterator(
      trainMaskImg, region);

  SampleType::Pointer sample = SampleType::New();
  sample->SetMeasurementVectorSize(numberOfModalities);
  IndexSampleType::Pointer featureSample = IndexSampleType::New();
  featureSample->SetMeasurementVectorSize(
      numberOfModalities + numberOfModalities * (numberOfModalities + 1) / 2);
  FeatureType feat;
  feat.SetSize(featureSample->GetMeasurementVectorSize());

  std::cerr << "Start collecting features" << std::endl;
  while (!imageIterator.IsAtEnd())
  {
    if (trainMaskIterator.Get() > itk::NumericTraits< LabelType >::Zero)
    {
      sample->PushBack(imageIterator.GetCenterPixel());
      extractMeasurementVector< VectorImageType >(imageIterator, feat);
      featureSample->PushBack(feat);
    }
    ++imageIterator;
    ++trainMaskIterator;
  }

  std::cerr << "Done collecting features" << std::endl;
  std::cerr << "Start creating tree" << std::endl;

  const unsigned int bucketSize = 16;
  typedef itk::Statistics::WeightedCentroidKdTreeGenerator< SampleType > CentroidTreeGeneratorType;
  CentroidTreeGeneratorType::Pointer centroidTreeGenerator =
      CentroidTreeGeneratorType::New();
  centroidTreeGenerator->SetSample(sample);
  centroidTreeGenerator->SetBucketSize(bucketSize);
  centroidTreeGenerator->Update();

  typedef CentroidTreeGeneratorType::KdTreeType TreeType;
  typedef TreeType::KdTreeNodeType NodeType;
  TreeType::Pointer tree = centroidTreeGenerator->GetOutput();

  std::cerr << "Done creating tree" << std::endl;
  std::cerr << "Start searching" << std::endl;

  itk::ImageRegionConstIterator< LabelImageType > testMaskIterator(testMaskImg,
                                                                   region);

  ProbabilityImageType::Pointer outImage = ProbabilityImageType::New();
  outImage->CopyInformation(image);
  outImage->SetRequestedRegion(image->GetRequestedRegion());
  outImage->SetBufferedRegion(image->GetBufferedRegion());
  outImage->Allocate();
  itk::ImageRegionIterator< ProbabilityImageType > outIterator(outImage,
                                                               region);

  imageIterator.GoToBegin();

  MeasurementVectorType queryPoint;
  queryPoint.SetSize(sample->GetMeasurementVectorSize(), true);

  const unsigned int numberOfNeighbors = 3;

  while (!imageIterator.IsAtEnd())
  {
    if (testMaskIterator.Get() > itk::NumericTraits< LabelType >::Zero)
    {
      extractMeasurementVector< VectorImageType >(imageIterator, feat);

      for (unsigned int i = 0; i < numberOfModalities; i++)
      {
        queryPoint[i] = imageIterator.GetCenterPixel()[i];
      }
      TreeType::InstanceIdentifierVectorType neighbors;
      tree->Search(queryPoint, numberOfNeighbors, neighbors);
      double pvalue = itk::NumericTraits< double >::max();
      for (unsigned int i = 0; i < numberOfNeighbors; i++)
      {
        const double currentPValue = pValue(
            featureSample->GetMeasurementVector(neighbors[i]), feat);
        if (currentPValue < pvalue)
        {
          pvalue = currentPValue;
        }
      }
      outIterator.Set(static_cast< ProbabilityType >(pvalue));

      if (false)
      {
        std::cout << queryPoint;
        std::cout << "->" << tree->GetMeasurementVector(neighbors[0]);
        std::cout << "->" << pvalue << std::endl;
      }
    }
    else
    {
      outIterator.Set(itk::NumericTraits< ProbabilityType >::ZeroValue());
    }

    ++imageIterator;
    ++testMaskIterator;
    ++outIterator;
  }

  std::cerr << "Done searching" << std::endl;
  CU::WriteImage< ProbabilityImageType >(pvalueOutput, outImage);
  return EXIT_SUCCESS;
}
