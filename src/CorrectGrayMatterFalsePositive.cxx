/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageUtil.h"

#include "itkStatisticsLabelObject.h"
#include "itkStatisticsLabelMapFilter.h"

#include "itkLabelImageToLabelMapFilter.h"
#include "itkLabelMapMaskImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkGrayscaleDilateImageFilter.h"

#include "itkLabelContourImageFilter.h"

#include "itkLabelStatisticsOpeningImageFilter.h"
#include "itkLabelMapToBinaryImageFilter.h"

#include "itkMaximumImageFilter.h"

int
main(int argc, char *argv[])
{
  if (argc < 4)
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr
        << " image brainTissueSegmentationFilenames newbrainTissueSegmentationFilename [alpha] [beta]"
        << std::endl;
    std::cerr << "alpha: Percentile of GM to be double checked" << std::endl;
    std::cerr
        << "beta:  Percentage of WM to GM voxels in surrounding in order for a voxel to be change to WM"
        << std::endl;
    std::cerr << std::endl;
    return EXIT_FAILURE;
    }

  std::string imageFilename(argv[1]);
  std::string brainTissueSegmentationFilename(argv[2]);
  std::string output(argv[3]);
  float alpha = 0.5;
  float beta = 0.5;
  if (argc > 4) alpha = atof(argv[4]);
  if (argc > 5) beta = atof(argv[5]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned int LabelType;

  const LabelType CerebrospinalFluidLabel = static_cast< LabelType >(1);
  const LabelType GrayMatterLabel = static_cast< LabelType >(2);
  const LabelType WhiteMatterLabel = static_cast< LabelType >(3);
  const LabelType WhiteMatterLesionLabel = static_cast< LabelType >(4);

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  typedef itk::StatisticsLabelObject< LabelType, ImageDimension > StatisticsLabelObjectType;
  typedef StatisticsLabelObjectType::LabelMapType StatisticsLabelMapType;

  typedef itk::ImageUtil< ImageType > ImageUtil;
  typedef itk::ImageUtil< LabelImageType > LabelImageUtil;

  /*
   * Load images
   */
  ImageType::Pointer subjectImg = ImageUtil::ReadImage(imageFilename);

  LabelImageType::Pointer btsImg = LabelImageUtil::ReadImage(
      brainTissueSegmentationFilename);

  typedef itk::LabelImageToLabelMapFilter< LabelImageType,
      StatisticsLabelMapType > LabelImageToLabelMapFilterT;

  LabelImageToLabelMapFilterT::Pointer btsLabelMapFilter =
      LabelImageToLabelMapFilterT::New();
  btsLabelMapFilter->SetInput(btsImg);
  btsLabelMapFilter->Update();

  const size_t numberOfLables =
      btsLabelMapFilter->GetOutput()->GetNumberOfLabelObjects();
  std::cerr << "There are " << numberOfLables << " objects." << std::endl;

  typedef itk::StatisticsLabelMapFilter< StatisticsLabelMapType, ImageType > ImageStatisticsLabelMapFilterT;

  ImageStatisticsLabelMapFilterT::Pointer imageStatisticsFilter =
      ImageStatisticsLabelMapFilterT::New();

  imageStatisticsFilter->ComputeFeretDiameterOff();
  imageStatisticsFilter->ComputePerimeterOff();
  imageStatisticsFilter->ComputeHistogramOn();
  imageStatisticsFilter->SetInput(btsLabelMapFilter->GetOutput());
  imageStatisticsFilter->SetFeatureImage(subjectImg);

  imageStatisticsFilter->Update();

  const PixelType grayMatterDoubleCheckThreshold =
      imageStatisticsFilter->GetOutput()->GetLabelObject(GrayMatterLabel)->GetHistogram()->Quantile(
          0, alpha);

  std::cerr << "All gray matter voxels brighter than ";
  std::cerr << grayMatterDoubleCheckThreshold;
  std::cerr << "(";
  std::cerr << alpha * 100;
  std::cerr << "%) is going to be double checked.";
  std::cerr << std::endl;

  typedef itk::LabelMapMaskImageFilter< StatisticsLabelMapType, ImageType > LabelMapMaskImageFilterT;
  LabelMapMaskImageFilterT::Pointer labelMapMaskFilter =
      LabelMapMaskImageFilterT::New();
  labelMapMaskFilter->NegatedOff();
  labelMapMaskFilter->CropOff();
  labelMapMaskFilter->SetInput(imageStatisticsFilter->GetOutput());
  labelMapMaskFilter->SetFeatureImage(subjectImg);
  labelMapMaskFilter->SetLabel(GrayMatterLabel);
  labelMapMaskFilter->Update();

  typedef itk::BinaryThresholdImageFilter< ImageType, LabelImageType > BinaryThresholdImageFilterT;
  BinaryThresholdImageFilterT::Pointer thresholdImageFilter =
      BinaryThresholdImageFilterT::New();
  thresholdImageFilter->SetInput(labelMapMaskFilter->GetOutput());
  thresholdImageFilter->SetLowerThreshold(grayMatterDoubleCheckThreshold);
  thresholdImageFilter->Update();

  typedef itk::ConnectedComponentImageFilter< LabelImageType, LabelImageType > ConnectedComponentImageFilterType;

  ConnectedComponentImageFilterType::Pointer connected =
      ConnectedComponentImageFilterType::New();
  connected->SetInput(thresholdImageFilter->GetOutput());
  connected->Update();

  typedef itk::BinaryBallStructuringElement< LabelType, ImageDimension > StructuringElementType;
  StructuringElementType structuringElement;
  structuringElement.SetRadius(1);
  structuringElement.CreateStructuringElement();

  typedef itk::GrayscaleDilateImageFilter< LabelImageType, LabelImageType,
      StructuringElementType > GrayscaleDilateImageFilterType;

  GrayscaleDilateImageFilterType::Pointer dilateFilter =
      GrayscaleDilateImageFilterType::New();
  dilateFilter->SetInput(connected->GetOutput());
  dilateFilter->SetKernel(structuringElement);

  typedef itk::LabelContourImageFilter< LabelImageType, LabelImageType > LabelContourImageFilterT;
  LabelContourImageFilterT::Pointer labelContourFilter =
      LabelContourImageFilterT::New();

  labelContourFilter->SetInput(dilateFilter->GetOutput());
  labelContourFilter->Update();

  LabelImageToLabelMapFilterT::Pointer contourLabelMapFilter =
      LabelImageToLabelMapFilterT::New();
  contourLabelMapFilter->SetInput(labelContourFilter->GetOutput());
  contourLabelMapFilter->Update();

  typedef itk::StatisticsLabelMapFilter< StatisticsLabelMapType, LabelImageType > LabelStatisticsLabelMapFilterT;

  LabelStatisticsLabelMapFilterT::Pointer contourStatFilter =
      LabelStatisticsLabelMapFilterT::New();
  contourStatFilter->ComputeFeretDiameterOff();
  contourStatFilter->ComputePerimeterOff();
  contourStatFilter->ComputeHistogramOn();
  contourStatFilter->SetNumberOfBins(numberOfLables);
  contourStatFilter->SetInput(contourLabelMapFilter->GetOutput());
  contourStatFilter->SetFeatureImage(btsImg);
  contourStatFilter->Update();
  StatisticsLabelMapType::Pointer statContourLabelMap =
      contourStatFilter->GetOutput();

  LabelImageToLabelMapFilterT::Pointer connectedLabelMapFilter =
      LabelImageToLabelMapFilterT::New();
  connectedLabelMapFilter->SetInput(connected->GetOutput());
  connectedLabelMapFilter->Update();

  LabelStatisticsLabelMapFilterT::Pointer connectedStatFilter =
      LabelStatisticsLabelMapFilterT::New();
  connectedStatFilter->ComputeFeretDiameterOff();
  connectedStatFilter->ComputePerimeterOff();
  connectedStatFilter->ComputeHistogramOff();
  connectedStatFilter->SetInput(connectedLabelMapFilter->GetOutput());
  connectedStatFilter->SetFeatureImage(btsImg);
  connectedStatFilter->Update();

  StatisticsLabelMapType::Pointer statLabelMap =
      connectedStatFilter->GetOutput();

  std::cerr << "All gray matter blobs that " << beta * 100 << "% ";
  std::cerr << "of its border is white matter will convert to gray matter.";
  std::cerr << std::endl;
  StatisticsLabelObjectType::HistogramType::MeasurementVectorType mv(
      1);
  for (size_t i = 1; i < statContourLabelMap->GetNumberOfLabelObjects(); i++)
    {

    StatisticsLabelObjectType::Pointer objContour =
        statContourLabelMap->GetLabelObject(i);

    mv[0] = WhiteMatterLabel;
    const double numberOfWhiteMatters =
        objContour->GetHistogram()->GetFrequency(mv);
    const double ratioOfWhiteMatters = numberOfWhiteMatters
        / objContour->GetNumberOfPixels();

    if (ratioOfWhiteMatters < beta)
      {
      statLabelMap->RemoveLabel(objContour->GetLabel());
      }
    }

  typedef itk::LabelMapToBinaryImageFilter< StatisticsLabelMapType,
      LabelImageType > LabelMapToBinaryImageFilterT;
  LabelMapToBinaryImageFilterT::Pointer labelMapToBinaryImage =
      LabelMapToBinaryImageFilterT::New();

  labelMapToBinaryImage->SetInput(statLabelMap);
  labelMapToBinaryImage->SetForegroundValue(WhiteMatterLesionLabel);
  labelMapToBinaryImage->Update();

  typedef itk::MaximumImageFilter<LabelImageType> MaximumImageFilterT;
  MaximumImageFilterT::Pointer btsMaxFilter = MaximumImageFilterT::New();
  btsMaxFilter->SetInput1(labelMapToBinaryImage->GetOutput());
  btsMaxFilter->SetInput2(btsImg);
  btsMaxFilter->Update();
  LabelImageUtil::WriteImage(output, btsMaxFilter->GetOutput());

  return EXIT_SUCCESS;
}
