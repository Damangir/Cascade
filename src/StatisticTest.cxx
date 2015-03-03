/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkImageToNeighborhoodSampleAdaptor.h"

#include "itkImageUtil.h"
#include "itkImageToWeightedHistogramFilter.h"

#include "itkCovarianceSampleFilter.h"
#include "itkListSample.h"
#include "itkSubsample.h"

#include "itkNeighborhoodOneSampleStatisticalTestImageFilter.h"

#include "itkAPTest.h"
#include "itkKSTest.h"
#include "itkKernelKSTest.h"

#include "vcl_cmath.h"

#include "itkSimpleFilterWatcher.h"

#include "itksys/CommandLineArguments.hxx"

int main(int argc, const char **argv)
{
  if (false)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " InclusionArea TestArea Input Output Radius [pos/neg]";
    std::cerr << std::endl;
    std::cerr << "InclusionArea, TestArea: 1/0 mask" << std::endl;
    std::cerr << "Input: Input image" << std::endl;
    std::cerr << "Output: P-value image" << std::endl;
    std::cerr << "Radius: Neighborhood radius in millimeter" << std::endl;
    std::cerr << "[pos/neg]: Right tail or left tail. Default: pos"
              << std::endl;
    return EXIT_FAILURE;
  }

  std::string trainMask;
  std::string testMask;
  std::string input;
  std::string pvalueOutput;

  double R = 1;
  std::string directionFlag("pos");
  std::string testType("AP");

  typedef itksys::CommandLineArguments argT;
  argT argParser;
  argParser.Initialize(argc, argv);

  argParser.AddArgument("--direction", argT::SPACE_ARGUMENT, &directionFlag,
                        "Direction of statistical test pos/neg [default=pos]");
  argParser.AddArgument("--radius", argT::SPACE_ARGUMENT, &R,
                        "Neighborhood radius in millimeter [default=1]");
  argParser.AddArgument("--output", argT::SPACE_ARGUMENT, &pvalueOutput,
                        "Output p-value image");
  argParser.AddArgument("--input", argT::SPACE_ARGUMENT, &input, "Input image");
  argParser.AddArgument("--train", argT::SPACE_ARGUMENT, &trainMask,
                        "0/1 mask to create the reference sample");
  argParser.AddArgument("--test", argT::SPACE_ARGUMENT, &testMask,
                        "0/1 mask of the area to be tested");
  argParser.AddArgument("--type", argT::SPACE_ARGUMENT, &testType,
                        "Type of statistical test. AP/KS/KKS [default=AP]");

  argParser.StoreUnusedArguments(true);

  if (!argParser.Parse() || input.empty() || trainMask.empty()
      || testMask.empty() || pvalueOutput.empty())
  {
    std::cerr << "Error parsing arguments." << std::endl;
    std::cerr << argParser.GetArgv0() << " [OPTIONS]" << std::endl;
    std::cerr << "Options: " << argParser.GetHelp() << std::endl;
    return EXIT_FAILURE;
  }

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;

  typedef double ProbabilityType;
  typedef itk::Image< ProbabilityType, ImageDimension > ImageType;
  typedef itk::ImageUtil< ImageType > ImageUtil;

  typedef itk::Statistics::ImageToWeightedHistogramFilter< ImageType > HistogramFilterType;
  typedef HistogramFilterType::HistogramType HistogramType;
  typedef itk::Statistics::Subsample< HistogramType > ReferenceSubsampleType;

  typedef itk::NeighborhoodOneSampleStatisticalTestImageFilter< ImageType,
      ImageType, ReferenceSubsampleType > OneSampleStatisticsType;

  typedef itk::Statistics::StatisticalTestBase<
      OneSampleStatisticsType::ReferenceSampleType,
      OneSampleStatisticsType::InternalSampleType > StatisticalTestType;

  typedef itk::Statistics::KSTest< OneSampleStatisticsType::ReferenceSampleType,
      OneSampleStatisticsType::InternalSampleType > KSTestType;

  typedef itk::Statistics::KernelKSTest<
      OneSampleStatisticsType::ReferenceSampleType,
      OneSampleStatisticsType::InternalSampleType > KernelKSTestType;

  typedef itk::Statistics::APTest< OneSampleStatisticsType::ReferenceSampleType,
      OneSampleStatisticsType::InternalSampleType > APTestType;

  /*
   * Begin: Load Images and calculate the corresponding physical radius
   */
  ImageType::Pointer img = ImageUtil::ReadImage(input);
  ImageType::Pointer trainImg = ImageUtil::ReadImage(trainMask);
  ImageType::Pointer testImg = ImageUtil::ReadImage(testMask);
  ImageType::SizeType radius = ImageUtil::GetRadiusFromPhysicalSize(img, R);

  /*
   * End: Load Images and calculate the corresponding physical radius
   */

  /*
   * Start: Calculate histogram for the training mask as a sorted sample
   */
  HistogramFilterType::Pointer histogramFilter = HistogramFilterType::New();
  typedef HistogramFilterType::HistogramSizeType SizeType;
  SizeType size(img->GetNumberOfComponentsPerPixel());
  size.Fill(255);
  histogramFilter->SetHistogramSize(size);

  histogramFilter->SetInput(img);
  histogramFilter->SetWeightImage(trainImg);
  histogramFilter->Update();
  HistogramType* hist = histogramFilter->GetOutput();

  ReferenceSubsampleType::Pointer refSorted = ReferenceSubsampleType::New();
  refSorted->SetSample(hist);
  refSorted->InitializeWithAllInstances();
  itk::Statistics::Algorithm::HeapSort< ReferenceSubsampleType >(
      refSorted, 0, 0, refSorted->Size());
  /*
   * End: Calculate histogram for the training mask as a sorted sample
   */

  OneSampleStatisticsType::Pointer oneSampleTest =
      OneSampleStatisticsType::New();

  StatisticalTestType::Pointer statTest;

  /*
   * Begin: Plugging the statistics
   */

  if (testType == "AP")
  {
    APTestType::Pointer apTest = APTestType::New();
    apTest->SortedFirstOn();
    statTest = apTest;
  }
  else if (testType == "KS")
  {
    KSTestType::Pointer ksTest = KSTestType::New();
    ksTest->SortedFirstOn();
    statTest = ksTest;
  }
  else if (testType == "KKS")
  {
    typedef itk::Statistics::CovarianceSampleFilter< ReferenceSubsampleType > CovarianceAlgorithmType;
    CovarianceAlgorithmType::Pointer covarianceAlgorithm =
        CovarianceAlgorithmType::New();
    covarianceAlgorithm->SetInput(refSorted);
    covarianceAlgorithm->Update();

    const double sigma = vcl_sqrt(covarianceAlgorithm->GetCovarianceMatrix().operator ()(0, 0));
    std::cout << "Sigma = " << std::endl;
    std::cout << sigma << std::endl;

    KernelKSTestType::Pointer kksTest = KernelKSTestType::New();
    kksTest->SortedFirstOn();
    kksTest->SetSigma(sigma);
    statTest = kksTest;
  }

  if (directionFlag == "neg")
  {
    statTest->LeftTailOn();
    std::cout << "Negative direction" << std::endl;
  }
  else
  {
    statTest->RightTailOn();
  }

  std::cout << "Radius = " << std::endl;
  std::cout << radius << std::endl;

  /*
   * End: Plugging the statistics
   */

  /*
   * Begin: Prepare input image
   */
  oneSampleTest->SetInput(
      ImageUtil::Mask(img, ImageUtil::Dilate(testImg, R, 1), 0));
  /*
   * Begin: End input image
   */

  oneSampleTest->SetRadius(radius);
  oneSampleTest->SetStatistics(statTest);
  oneSampleTest->SetRefrenceSample(refSorted);

  itk::SimpleFilterWatcher watcher(oneSampleTest,
                                   "One sample statistical test.");
  watcher.QuietOn();

  try
  {
    oneSampleTest->Update();
  }
  catch (itk::ExceptionObject & err)
  {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  ImageUtil::WriteImage(
      pvalueOutput, ImageUtil::Mask(oneSampleTest->GetOutput(), testImg, 0));

  return EXIT_SUCCESS;
}

