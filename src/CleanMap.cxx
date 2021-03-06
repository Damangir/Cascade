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
  std::string mask;
  std::string input;
  std::string output = "";
  double minRad = 0;
  double meanThresh = -1;
  bool verbose = false;

  typedef itksys::CommandLineArguments argT;
  argT argParser;
  argParser.Initialize(argc, argv);


  argParser.AddBooleanArgument("--verbose", &verbose,
                        "Report the decision on all detection");

  argParser.AddArgument("--radius", argT::SPACE_ARGUMENT, &minRad,
                        "Minimum distance to cleaning mask");
  argParser.AddArgument("--overlap-thresh", argT::SPACE_ARGUMENT, &meanThresh,
                        "Minimum percentage of the detection allowed to be overlapped by the mask");
  argParser.AddArgument("--output-seg", argT::SPACE_ARGUMENT, &output,
                        "Output segmentation");
  argParser.AddArgument("--mask", argT::SPACE_ARGUMENT, &mask,
                        "Cleaning mask");
  argParser.AddArgument("--input-seg", argT::SPACE_ARGUMENT, &input,
                        "input segmentation");

  argParser.StoreUnusedArguments(true);

  if (!argParser.Parse() || mask=="" || input=="")
  {
    std::cerr << "Error parsing arguments." << std::endl;
    std::cerr << "" << " [OPTIONS]" << std::endl;
    std::cerr << "Options: " << argParser.GetHelp() << std::endl;
    return EXIT_FAILURE;
  }

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;

  typedef unsigned int LabelType;

  const LabelType insideLabel = static_cast< LabelType >(1);
  const LabelType outsideLabel = static_cast< LabelType >(0);

  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  typedef itk::StatisticsLabelObject< LabelType, ImageDimension > StatisticsLabelObjectType;
  typedef StatisticsLabelObjectType::LabelMapType StatisticsLabelMapType;

  typedef itk::ImageUtil< LabelImageType > LabelImageUtil;

  /*
   * Load images
   */

  LabelImageType::Pointer segmentation = LabelImageUtil::ReadImage(input);
  LabelImageType::Pointer cleanMask = LabelImageUtil::ReadImage(mask);
  if (minRad > 0)
  {
    if (verbose)
    {
      std::cerr << "Radius is " << LabelImageUtil::GetRadiusFromPhysicalSize(cleanMask, minRad) << std::endl;
    }
    cleanMask = LabelImageUtil::Dilate(cleanMask, minRad, insideLabel);
  }

  typedef itk::ConnectedComponentImageFilter< LabelImageType, LabelImageType > ConnectedComponentImageFilterType;

  ConnectedComponentImageFilterType::Pointer connected =
      ConnectedComponentImageFilterType::New();
  connected->SetInput(segmentation);

  typedef itk::LabelImageToShapeLabelMapFilter< LabelImageType,
      StatisticsLabelMapType > LabelImageToShapeLabelType;
  LabelImageToShapeLabelType::Pointer labelMapCreator =
      LabelImageToShapeLabelType::New();
  labelMapCreator->SetInput(connected->GetOutput());
  labelMapCreator->Update();

  typedef itk::StatisticsLabelMapFilter< StatisticsLabelMapType, LabelImageType > ImageStatisticsLabelMapFilterT;

  ImageStatisticsLabelMapFilterT::Pointer labelStatisticsValuator =
      ImageStatisticsLabelMapFilterT::New();

  labelStatisticsValuator->ComputeFeretDiameterOff();
  labelStatisticsValuator->ComputePerimeterOff();
  labelStatisticsValuator->ComputeHistogramOff();
  labelStatisticsValuator->SetInput(labelMapCreator->GetOutput());
  labelStatisticsValuator->SetFeatureImage(cleanMask);

  labelStatisticsValuator->Update();
  StatisticsLabelMapType::Pointer statLabelMap =
      labelStatisticsValuator->GetOutput();

  float totalPhysicalSize = 0;
  for (size_t i = 0; i < statLabelMap->GetNumberOfLabelObjects(); i++)
  {
    StatisticsLabelObjectType::Pointer objMB = statLabelMap->GetNthLabelObject(i);
    bool toRemove = false;
    std::stringstream reasonDBG;
    if (objMB->GetMean() > meanThresh)
    {
      toRemove = true;
      reasonDBG << "Touches - (Overlap " << objMB->GetMean() << ")";
    }

    if (toRemove)
    {
      if (verbose)
      {
        std::cerr << "Removed:" << objMB->GetLabel() << " ( reason=" << reasonDBG.str() << ")" << std::endl;
      }
      statLabelMap->RemoveLabelObject(objMB);
      i--;
    }
    else
    {
      if (verbose)
      {
        std::cerr << "Not removed:" << objMB->GetLabel() << std::endl;
      }
      totalPhysicalSize += objMB->GetPhysicalSize();
    }
  }

  std::cout << totalPhysicalSize << ","
            << statLabelMap->GetNumberOfLabelObjects() << std::endl;

  if (!output.empty())
  {
    typedef itk::LabelMapToBinaryImageFilter< StatisticsLabelMapType,
        LabelImageType > LabelMapToBinaryImageFilterT;
    LabelMapToBinaryImageFilterT::Pointer labelMapToBinaryImage =
        LabelMapToBinaryImageFilterT::New();

    labelMapToBinaryImage->SetInput(statLabelMap);
    labelMapToBinaryImage->SetBackgroundValue(outsideLabel);
    labelMapToBinaryImage->SetForegroundValue(insideLabel);
    labelMapToBinaryImage->Update();

    LabelImageUtil::WriteImage(output, labelMapToBinaryImage->GetOutput());
  }

  return EXIT_SUCCESS;
}
