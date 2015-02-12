/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageUtil.h"

#include "itkStatisticsLabelObject.h"
#include "itkStatisticsLabelMapFilter.h"

#include "itkBinaryImageToLabelMapFilter.h"
#include "itkLabelImageToShapeLabelMapFilter.h"
#include "itkLabelMapMaskImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkConnectedComponentImageFilter.h"

#include "itkLabelStatisticsOpeningImageFilter.h"
#include "itkLabelMapToBinaryImageFilter.h"

#include "itksys/CommandLineArguments.hxx"
#include "itksys/SystemTools.hxx"

int main(int argc, const char **argv)
{
  std::string map;
  std::string seg = "";
  double thresh = 0;
  double minSize = 0;
  double bridgeRad = 0;
  double maxThresh = 0;

  typedef itksys::CommandLineArguments argT;
  argT argParser;
  argParser.Initialize(argc, argv);

  argParser.AddArgument("--binary-seg", argT::SPACE_ARGUMENT, &seg,
                        "Generate binary segmentation");
  argParser.AddArgument("--threshold", argT::SPACE_ARGUMENT, &thresh,
                        "Input map threshold");
  argParser.AddArgument("--min-size", argT::SPACE_ARGUMENT, &minSize,
                        "Minimum detection size in mm3");
  argParser.AddArgument("--bridge-size", argT::SPACE_ARGUMENT, &bridgeRad,
                        "Minimum bridging width for lesion counting in mm");
  argParser.AddArgument("--max-threshold", argT::SPACE_ARGUMENT, &maxThresh,
                        "Threshold for removing detection whose maximum"
                        " is smaller than this value");
  argParser.StoreUnusedArguments(true);

  if (!argParser.Parse())
  {
    std::cerr << "Error parsing arguments." << std::endl;
    std::cerr << "" << " [OPTIONS] map" << std::endl;
    std::cerr << "Options: " << argParser.GetHelp() << std::endl;
    return EXIT_FAILURE;
  }
  char** newArgv = 0;
  int newArgc = 0;
  argParser.GetUnusedArguments(&newArgc, &newArgv);
  if (newArgc == 2)
  {
    map = newArgv[1];
  }
  else
  {
    std::cerr << "Error parsing arguments" << std::endl;
    std::cerr << "Options: " << argParser.GetHelp() << std::endl;
    return EXIT_FAILURE;
  }
  argParser.DeleteRemainingArguments(newArgc, &newArgv);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;
  typedef unsigned int LabelType;

  const LabelType insideLabel = static_cast< LabelType >(1);
  const LabelType outsideLabel = static_cast< LabelType >(0);

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  typedef itk::StatisticsLabelObject< LabelType, ImageDimension > StatisticsLabelObjectType;
  typedef StatisticsLabelObjectType::LabelMapType StatisticsLabelMapType;

  typedef itk::ImageUtil< ImageType > ImageUtil;
  typedef itk::ImageUtil< LabelImageType > LabelImageUtil;

  /*
   * Load images
   */
  ImageType::Pointer mapImg = ImageUtil::ReadImage(map);

  typedef itk::BinaryThresholdImageFilter< ImageType, LabelImageType > BinaryThresholdImageFilterT;
  BinaryThresholdImageFilterT::Pointer thresholdImageFilter =
      BinaryThresholdImageFilterT::New();
  thresholdImageFilter->SetInput(mapImg);
  thresholdImageFilter->SetInsideValue(insideLabel);
  thresholdImageFilter->SetOutsideValue(outsideLabel);
  thresholdImageFilter->SetLowerThreshold(thresh);

  LabelImageType::Pointer binarizedMap = LabelImageUtil::GraftOutput(
      thresholdImageFilter, 0);
  if (bridgeRad > 0)
  {
    binarizedMap = LabelImageUtil::Opening(binarizedMap, bridgeRad, insideLabel);
  }

  typedef itk::ConnectedComponentImageFilter< LabelImageType, LabelImageType > ConnectedComponentImageFilterType;

  ConnectedComponentImageFilterType::Pointer connected =
      ConnectedComponentImageFilterType::New();
  connected->SetInput(binarizedMap);

  typedef itk::LabelImageToShapeLabelMapFilter< LabelImageType,
      StatisticsLabelMapType > LabelImageToShapeLabelType;
  LabelImageToShapeLabelType::Pointer labelMapCreator =
      LabelImageToShapeLabelType::New();
  labelMapCreator->SetInput(connected->GetOutput());
  labelMapCreator->Update();

  typedef itk::StatisticsLabelMapFilter< StatisticsLabelMapType, ImageType > ImageStatisticsLabelMapFilterT;

  ImageStatisticsLabelMapFilterT::Pointer labelStatisticsValuator =
      ImageStatisticsLabelMapFilterT::New();

  labelStatisticsValuator->ComputeFeretDiameterOff();
  labelStatisticsValuator->ComputePerimeterOff();
  labelStatisticsValuator->ComputeHistogramOff();
  labelStatisticsValuator->SetInput(labelMapCreator->GetOutput());
  labelStatisticsValuator->SetFeatureImage(mapImg);

  labelStatisticsValuator->Update();
  StatisticsLabelMapType::Pointer statLabelMap =
      labelStatisticsValuator->GetOutput();

  float totalPhysicalSize = 0;
  for (size_t i = 1; i < statLabelMap->GetNumberOfLabelObjects(); i++)
  {

    StatisticsLabelObjectType::Pointer objMB = statLabelMap->GetLabelObject(i);
    bool toRemove = false;
    std::string reasonDBG;
    if (objMB->GetMaximum() < maxThresh)
    {
      toRemove = true;
      reasonDBG = "Maximum is too low";
    }
    if (objMB->GetPhysicalSize() < minSize)
    {
      toRemove = true;
      reasonDBG = "Detection is too small";
    }

    if (toRemove)
    {
      statLabelMap->RemoveLabel(objMB->GetLabel());
    }
    else
    {
      totalPhysicalSize += objMB->GetPhysicalSize();
    }
  }
  std::cout << totalPhysicalSize << ","
            << statLabelMap->GetNumberOfLabelObjects() << std::endl;

  if (!seg.empty())
  {
    typedef itk::LabelMapToBinaryImageFilter< StatisticsLabelMapType,
        LabelImageType > LabelMapToBinaryImageFilterT;
    LabelMapToBinaryImageFilterT::Pointer labelMapToBinaryImage =
        LabelMapToBinaryImageFilterT::New();

    labelMapToBinaryImage->SetInput(statLabelMap);
    labelMapToBinaryImage->SetBackgroundValue(outsideLabel);
    labelMapToBinaryImage->SetForegroundValue(insideLabel);
    labelMapToBinaryImage->Update();

    LabelImageUtil::WriteImage(seg, labelMapToBinaryImage->GetOutput());
  }

  return EXIT_SUCCESS;
}
