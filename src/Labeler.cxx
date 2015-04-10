/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageUtil.h"

#include "itkBinaryThresholdImageFilter.h"
#include "itkConnectedComponentImageFilter.h"

#include "itksys/CommandLineArguments.hxx"
#include "itksys/SystemTools.hxx"

int main(int argc, const char **argv)
{
  std::string seg;
  std::string labeled;

  typedef itksys::CommandLineArguments argT;
  argT argParser;
  argParser.Initialize(argc, argv);

  argParser.AddArgument("--binary-seg", argT::SPACE_ARGUMENT, &seg,
                        "Input binary segmentation");
  argParser.AddArgument("--threshold", argT::SPACE_ARGUMENT, &labeled,
                        "Output labeled segmentation");

  argParser.StoreUnusedArguments(true);

  if (!argParser.Parse() || seg.empty() || labeled.empty())
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

  typedef itk::ImageUtil< LabelImageType > LabelImageUtil;

  /*
   * Labelize binary image
   */

  typedef itk::BinaryThresholdImageFilter< LabelImageType, LabelImageType > BinaryThresholdImageFilterT;
  BinaryThresholdImageFilterT::Pointer thresholdImageFilter =
      BinaryThresholdImageFilterT::New();
  thresholdImageFilter->SetInput(LabelImageUtil::ReadImage(seg));
  thresholdImageFilter->SetInsideValue(insideLabel);
  thresholdImageFilter->SetOutsideValue(outsideLabel);
  thresholdImageFilter->SetLowerThreshold(1);

  typedef itk::ConnectedComponentImageFilter< LabelImageType, LabelImageType > ConnectedComponentImageFilterType;

  ConnectedComponentImageFilterType::Pointer connected =
      ConnectedComponentImageFilterType::New();
  connected->SetInput(thresholdImageFilter->GetOutput());

  LabelImageUtil::WriteImage(labeled, connected->GetOutput());

  return EXIT_SUCCESS;
}
