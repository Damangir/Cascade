/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "itkImageToWeightedHistogramFilter.h"
#include "itkEmpiricalDensityMembershipFunction.h"
#include "itkIterativeBayesianImageFilter.h"
#include "itkFloodFilledImageFunctionConditionalIterator.h"
#include "itkBinaryThresholdImageFunction.h"
#include "itkBinaryFillholeImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkComposeImageFilter.h"

#include "imageHelpers.h"

namespace CU = cascade::util;

int main(int argc, char *argv[])
{
  if (argc < 7)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr
        << " image brainMask csfMask gmPrior wmPrior btOutput [bias] [nIteration]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string subjectImage(argv[1]);
  std::string brainMask(argv[2]);
  std::string csfMask(argv[3]);
  std::string gmPrior(argv[4]);
  std::string wmPrior(argv[5]);
  std::string btOutput(argv[6]);
  float priorBias = 0.3;
  unsigned int numberOfIteration = 1;
  if (argc > 7) priorBias = atof(argv[7]);
  if (argc > 8) numberOfIteration = atof(argv[8]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned char LabelType;
  typedef float Probability;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  typedef itk::IterativeBayesianImageFilter< VectorImageType, LabelImageType,
      Probability > IBFilterType;

  typedef IBFilterType::PriorImageType PriorImageType;
  typedef IBFilterType::PriorsVectorImageType PriorsVectorImageType;
  typedef IBFilterType::MembershipsVectorImageType MembershipsVectorImageType;
  typedef IBFilterType::ClassifierOutputImageType ClassifidImageType;

  typedef IBFilterType::EmpiricalDistributionMembershipType EmpiricalDistributionMembershipType;
  typedef IBFilterType::WeightedHistogramType WeightedHistogramType;
  typedef IBFilterType::MembershipFilterType MembershipFilterType;

  typedef itk::SubtractImageFilter< PriorImageType > PriorSubFilterType;
  typedef itk::DiscreteGaussianImageFilter< PriorImageType, PriorImageType > PriorSmoothingType;
  typedef itk::ComposeImageFilter< PriorImageType, PriorsVectorImageType > ComposePriorsFilterType;
  typedef itk::ComposeImageFilter< ImageType, VectorImageType > ComposeInputFilterType;

  LabelImageType::Pointer brainMaskImg = CU::LoadImage< LabelImageType >(
      brainMask);

  ComposeInputFilterType::Pointer inputComposer = ComposeInputFilterType::New();
  inputComposer->SetInput(0, CU::LoadImage< ImageType >(subjectImage));
  inputComposer->Update();
  VectorImageType::Pointer subjectImg = inputComposer->GetOutput();

  ClassifidImageType::Pointer CSFMask = CU::LoadImage< ClassifidImageType >(
      csfMask);
  ClassifidImageType::Pointer WMMask;
  ClassifidImageType::Pointer GMMask;
  ClassifidImageType::Pointer WMGMMask;
  ClassifidImageType::Pointer LesionMask;
  ClassifidImageType::Pointer BTS;
  /*
   * Create WMGMMask
   */
  {
    typedef itk::BinaryFunctorImageFilter< ClassifidImageType,
        ClassifidImageType, ClassifidImageType,
        itk::Functor::Equal< ClassifidImageType::PixelType,
            ClassifidImageType::PixelType > > EqualClassificationFilterType;
    EqualClassificationFilterType::Pointer eqFilter =
        EqualClassificationFilterType::New();
    eqFilter->SetInput1(CSFMask);
    eqFilter->SetConstant2(0);
    WMGMMask = CU::Mask< ClassifidImageType, LabelImageType >(
        eqFilter->GetOutput(), brainMaskImg);
  }

  PriorImageType::Pointer gmPriorImg = CU::LoadImage< PriorImageType >(gmPrior);
  PriorImageType::Pointer wmPriorImg = CU::LoadImage< PriorImageType >(wmPrior);

  /*
   * Mask All inputs:
   */
  {
    gmPriorImg = CU::Mask< PriorImageType, ClassifidImageType >(gmPriorImg,
                                                                WMGMMask);
    wmPriorImg = CU::Mask< PriorImageType, ClassifidImageType >(wmPriorImg,
                                                                WMGMMask);
  }

  const unsigned int nComp = subjectImg->GetNumberOfComponentsPerPixel();
  const unsigned int hSize = 100;
  WeightedHistogramType::HistogramSizeType size(nComp);
  size.Fill(hSize);

  ComposePriorsFilterType::Pointer composePriorFilter =
      ComposePriorsFilterType::New();
  PriorSmoothingType::Pointer wmPriorSmoothing = PriorSmoothingType::New();
  PriorSmoothingType::Pointer gmPriorSmoothing = PriorSmoothingType::New();
  gmPriorSmoothing->SetInput(gmPriorImg);
  gmPriorSmoothing->SetVariance(10);
  wmPriorSmoothing->SetInput(wmPriorImg);
  wmPriorSmoothing->SetVariance(10);

  composePriorFilter->SetInput(
      0,
      CU::Mask< PriorImageType, ClassifidImageType >(
          gmPriorSmoothing->GetOutput(), WMGMMask));
  composePriorFilter->SetInput(
      1,
      CU::Mask< PriorImageType, ClassifidImageType >(
          wmPriorSmoothing->GetOutput(), WMGMMask));
  composePriorFilter->Update();

  EmpiricalDistributionMembershipType::Pointer wmgmMembership =
      EmpiricalDistributionMembershipType::New();
  EmpiricalDistributionMembershipType::Pointer csfMembership =
      EmpiricalDistributionMembershipType::New();
  EmpiricalDistributionMembershipType::Pointer gmMembership =
      EmpiricalDistributionMembershipType::New();
  EmpiricalDistributionMembershipType::Pointer wmMembership =
      EmpiricalDistributionMembershipType::New();

  EmpiricalDistributionMembershipType::DistributionType::Pointer wmgmDist;
  EmpiricalDistributionMembershipType::DistributionType::Pointer csfDist;
  EmpiricalDistributionMembershipType::DistributionType::Pointer gmDist;
  EmpiricalDistributionMembershipType::DistributionType::Pointer wmDist;

  /*
   * Calculate intensity distribution for each class
   */
  {
    WeightedHistogramType::Pointer hist = WeightedHistogramType::New();
    hist->SetAutoMinimumMaximum(true);
    hist->SetHistogramSize(size);
    hist->SetInput(subjectImg);
    // Create WMGM membership
    hist->SetWeightImage(CU::Cast< PriorImageType >(WMGMMask.GetPointer()));
    hist->Update();
    wmgmDist = hist->GetOutput();
    wmgmDist->DisconnectPipeline();
    // Create CSF membership
    hist->SetWeightImage(CU::Cast< PriorImageType >(CSFMask.GetPointer()));
    hist->Update();
    csfDist = hist->GetOutput();
    csfDist->DisconnectPipeline();
    // Create GM membership
    hist->SetWeightImage(gmPriorImg);
    hist->Update();
    gmDist = hist->GetOutput();
    gmDist->DisconnectPipeline();
    // Create GM membership
    hist->SetWeightImage(wmPriorImg);
    hist->Update();
    wmDist = hist->GetOutput();
    wmDist->DisconnectPipeline();
    // Create GM membership
    hist->SetWeightImage(wmPriorImg);
    hist->Update();
    wmDist = hist->GetOutput();
    wmDist->DisconnectPipeline();

    wmgmMembership->SetDistribution(wmgmDist);
    csfMembership->SetDistribution(csfDist);
    gmMembership->SetDistribution(gmDist);
    wmMembership->SetDistribution(wmDist);
  }

  /*
   * Roughly separate WM and GM with emphasis on Prior
   */
  {
    IBFilterType::Pointer ibFilter = IBFilterType::New();
    ibFilter->AddMembershipFunction(gmMembership);
    ibFilter->AddMembershipFunction(wmMembership);
    ibFilter->SetPriorVectorImage(composePriorFilter->GetOutput());
    ibFilter->SetInput(
        CU::Mask< VectorImageType, ClassifidImageType >(subjectImg, WMGMMask));
    ibFilter->SetNumberOfIterations(0);
    ibFilter->SetPriorBias(0.2);
    ibFilter->Update();
    WMMask = CU::Mask< ClassifidImageType, ClassifidImageType >(
        ibFilter->GetOutput(), WMGMMask);
  }

  /*
   * If the WM is more similar to CSF than a WM it is probably WML
   */
  {
    IBFilterType::Pointer ibFilter = IBFilterType::New();
    ibFilter->AddMembershipFunction(wmMembership);
    ibFilter->AddMembershipFunction(csfMembership);
    ibFilter->SetPriorVectorImage(composePriorFilter->GetOutput());
    ibFilter->SetInput(
        CU::Mask< VectorImageType, ClassifidImageType >(subjectImg, WMGMMask));
    ibFilter->SetNumberOfIterations(0);
    ibFilter->SetPriorBias(100);
    ibFilter->Update();
    LesionMask = CU::Mask< ClassifidImageType, ClassifidImageType >(
        ibFilter->GetOutput(), WMMask);
  }

  if (true)
  {
    MembershipFilterType::Pointer membershipFilter =
        MembershipFilterType::New();
    WeightedHistogramType::Pointer hist = WeightedHistogramType::New();
    EmpiricalDistributionMembershipType::Pointer membership =
        EmpiricalDistributionMembershipType::New();

    itk::DiscreteGaussianImageFilter< ClassifidImageType, PriorImageType >::Pointer smoothFilter =
        itk::DiscreteGaussianImageFilter< ClassifidImageType, PriorImageType >::New();

    smoothFilter->SetInput(LesionMask);
    smoothFilter->SetVariance(2);
    smoothFilter->Update();

    hist->SetWeightImage(
        CU::MultiplyConstant< PriorImageType >(smoothFilter->GetOutput(), 100));
    hist->SetAutoMinimumMaximum(true);
    hist->SetHistogramSize(size);
    hist->SetInput(subjectImg);
    hist->Update();

    membership->SetDistribution(hist->GetOutput());
    membershipFilter->AddMembershipFunction(membership);

    membershipFilter->SetInput(subjectImg);
    membershipFilter->Update();

    /*
     * Probability of each point belong to pre calculated Lesion Mask
     */
    PriorImageType::Pointer lesionFineProb = CU::NthElementVectorImage(
        membershipFilter->GetOutput(), 0);

    typedef itk::BinaryThresholdImageFunction< PriorImageType, double > FunctionType;

    FunctionType::Pointer function = FunctionType::New();

    function->SetInputImage(lesionFineProb);

    /*
     * Flood fill to expand the Lesion mask to the area with high likelihood of
     * being a lesion. The aim is to cover all parts that is misclassified as
     * gray matter even at expense of covering some white matter are.
     * TODO: Try another region growing algorithm that stops at edges.
     */
    typedef itk::FloodFilledImageFunctionConditionalIterator<
        ClassifidImageType, FunctionType > IteratorType;
    IteratorType ffIt(LesionMask, function);

    typedef itk::ImageRegionConstIterator< ClassifidImageType > IRIType;
    typedef itk::ImageRegionConstIterator< PriorImageType > PIRIType;
    IRIType it = IRIType(LesionMask, LesionMask->GetBufferedRegion());
    PIRIType pit = PIRIType(lesionFineProb, LesionMask->GetBufferedRegion());

    unsigned int nElem = 0;
    Probability mean = 0;
    Probability stdDev = 0;
    for (it.GoToBegin(), pit.GoToBegin(); !it.IsAtEnd(); ++it, ++pit)
    {
      if (it.Get() != 0)
      {
        nElem++;
        Probability thisValue = pit.Get();
        Probability delta = thisValue - mean;
        mean += delta / nElem;
        stdDev += delta * (thisValue - mean);
        ffIt.AddSeed(it.GetIndex());
      }
    }
    stdDev = vcl_sqrt(stdDev/( nElem - 1));
    function->ThresholdBetween(mean - 2 * stdDev, 1);

    ffIt.GoToBegin();
    while (!ffIt.IsAtEnd())
    {
      ffIt.Set(1);
      ++ffIt;
    }
    LesionMask = CU::Mask< ClassifidImageType, ClassifidImageType >(LesionMask,
                                                                    WMMask);

  }

  WMGMMask = CU::Subtract< ClassifidImageType, ClassifidImageType >(WMGMMask,
                                                                    LesionMask);

  /*
   * Reclassify white matter and gray matter not taking Lesions into account
   */
  if (true)
  {
    IBFilterType::Pointer ibFilter = IBFilterType::New();
    ibFilter->AddMembershipFunction(gmMembership);
    ibFilter->AddMembershipFunction(wmMembership);
    ibFilter->SetPriorVectorImage(composePriorFilter->GetOutput());
    ibFilter->SetInput(
        CU::Mask< VectorImageType, ClassifidImageType >(subjectImg, WMGMMask));
    ibFilter->SetNumberOfIterations(numberOfIteration);
    ibFilter->SetPriorBias(priorBias);
    ibFilter->Update();
    WMMask = CU::Mask< ClassifidImageType, ClassifidImageType >(
        ibFilter->GetOutput(), WMGMMask);
  }

  /*
   * If a gray matter part is surrounded by WM and Lesions it is probably a
   * lesion.
   */
  if (true)
  {
    WMGMMask = CU::Add< ClassifidImageType, ClassifidImageType >(WMGMMask,
                                                                 LesionMask);
    ClassifidImageType::Pointer LesionAndWMMask = CU::Add< ClassifidImageType,
        ClassifidImageType >(LesionMask, WMMask);
    LesionAndWMMask = CU::Closing< ClassifidImageType >(LesionAndWMMask, 1);
    typedef itk::BinaryFillholeImageFilter< ClassifidImageType > FillHoleType;
    FillHoleType::Pointer fillHole = FillHoleType::New();
    fillHole->SetForegroundValue(1);
    fillHole->SetInput(LesionAndWMMask);
    LesionAndWMMask = CU::GraftOutput< FillHoleType >(fillHole);

    LesionMask = CU::Subtract< ClassifidImageType, ClassifidImageType >(
        LesionAndWMMask, WMMask);
    WMGMMask = CU::Subtract< ClassifidImageType, ClassifidImageType >(
        WMGMMask, LesionMask);

  }
  LesionMask = CU::MultiplyConstant< ClassifidImageType >(LesionMask, 4);

  BTS = CU::MultiplyConstant< ClassifidImageType >(WMGMMask, 2);
  BTS = CU::Add< ClassifidImageType, ClassifidImageType >(BTS, CSFMask);
  BTS = CU::Add< ClassifidImageType, ClassifidImageType >(BTS, WMMask);
  BTS = CU::Add< ClassifidImageType, ClassifidImageType >(BTS, LesionMask);

  CU::WriteImage< ClassifidImageType >(btOutput, BTS);

  return EXIT_SUCCESS;
}
