/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkImageUtil.h"

#include "itkStatisticsLabelObject.h"
#include "itkStatisticsLabelMapFilter.h"
#include "itkLabelImageToShapeLabelMapFilter.h"

#include "itksys/CommandLineArguments.hxx"
#include "itksys/SystemTools.hxx"

#include "map"
#include "algorithm"
#include "ctype.h"

int main(int argc, char *argv[])
{
  std::string feature;
  std::string atlas;
  std::string labelNamesFile;
  std::string delimiter = ": ";
  std::string prefix;
  std::string toReport;
  bool reportAll = false;

  typedef itksys::CommandLineArguments argT;
  argT argParser;
  argParser.Initialize(argc, argv);

  argParser.AddArgument("--feature", argT::SPACE_ARGUMENT, &feature,
                        "Input feature");
  argParser.AddArgument("--atlas", argT::SPACE_ARGUMENT, &atlas,
                        "Reporting Atlas");
  argParser.AddArgument("--label-name", argT::SPACE_ARGUMENT, &labelNamesFile,
                        "Atlas label names");

  argParser.AddArgument("--report", argT::SPACE_ARGUMENT, &toReport,
                        "Stats to report");
  argParser.AddBooleanArgument("--report-all", &reportAll,
                               "Report all available stats");

  argParser.AddArgument("--delimiter", argT::SPACE_ARGUMENT, &delimiter,
                        "Output delimiter");
  argParser.AddArgument("--prefix", argT::SPACE_ARGUMENT, &prefix,
                        "Prefix to each line");

  argParser.StoreUnusedArguments(true);

  if (!argParser.Parse() || feature.empty() || atlas.empty())
  {
    std::cerr << "Error parsing arguments." << std::endl;
    std::cerr << "" << " [OPTIONS]" << std::endl;
    std::cerr << "Options: " << argParser.GetHelp() << std::endl;
    return EXIT_FAILURE;
  }

  if (toReport.empty())
  {
    reportAll = true;
  }
  std::transform(toReport.begin(), toReport.end(), toReport.begin(), toupper);
  toReport = toReport + " ";

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;

  typedef double PixelType;
  typedef unsigned int LabelType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  typedef itk::StatisticsLabelObject< LabelType, ImageDimension > StatisticsLabelObjectType;
  typedef StatisticsLabelObjectType::LabelMapType StatisticsLabelMapType;

  typedef itk::ImageUtil< ImageType > ImageUtil;
  typedef itk::ImageUtil< LabelImageType > LabelImageUtil;

  typedef std::map< LabelType, std::string > AtlasNameMapType;

  /*
   * Load label names
   */
  AtlasNameMapType atlasNameMap;
  if (!labelNamesFile.empty())
  {
    std::ifstream infile(labelNamesFile.c_str());
    std::string line;
    while (std::getline(infile, line))
    {
      /*
       * If the line starts with a digit
       */
      if (line.find_first_of("0123456789") == 0)
      {
        std::istringstream iss(line);
        PixelType atl;
        std::string atlLabel;
        if (!(iss >> atl >> atlLabel))
        {
          continue;
        } // error
        atlasNameMap[atl] = atlLabel;
      }
    }
  }

  /*
   * Calculate statistics
   */
  typedef itk::LabelImageToShapeLabelMapFilter< LabelImageType,
      StatisticsLabelMapType > LabelImageToShapeLabelType;
  LabelImageToShapeLabelType::Pointer labelMapCreator =
      LabelImageToShapeLabelType::New();
  labelMapCreator->SetInput(LabelImageUtil::ReadImage(atlas));

  typedef itk::StatisticsLabelMapFilter< StatisticsLabelMapType, ImageType > ImageStatisticsLabelMapFilterT;

  ImageStatisticsLabelMapFilterT::Pointer labelStatisticsValuator =
      ImageStatisticsLabelMapFilterT::New();

  labelStatisticsValuator->ComputeFeretDiameterOff();
  labelStatisticsValuator->ComputePerimeterOff();
  labelStatisticsValuator->ComputeHistogramOff();
  labelStatisticsValuator->SetInput(labelMapCreator->GetOutput());
  labelStatisticsValuator->SetFeatureImage(ImageUtil::ReadImage(feature));

  labelStatisticsValuator->Update();
  StatisticsLabelMapType::Pointer statLabelMap =
      labelStatisticsValuator->GetOutput();

  for (size_t i = 0; i < statLabelMap->GetNumberOfLabelObjects(); i++)
  {
    StatisticsLabelObjectType::Pointer objMB = statLabelMap->GetNthLabelObject(
        i);
    std::stringstream attribs;

    std::stringstream preText;

    if (!prefix.empty())
    {
      preText << prefix << delimiter;
    }
    AtlasNameMapType::const_iterator nit = atlasNameMap.find(objMB->GetLabel());
    if (nit == atlasNameMap.end())
    {
      preText << objMB->GetLabel() << delimiter;
    }
    else
    {
      preText << nit->second << delimiter;
    }

    if (reportAll || toReport.find("MINIMUM ") != std::string::npos)
    {
      attribs << preText.str() << "Minimum" << delimiter << objMB->GetMinimum()
                               << std::endl;
    }
    if (reportAll || toReport.find("MAXIMUM ") != std::string::npos)
    {
      attribs << preText.str() << "Maximum" << delimiter << objMB->GetMaximum()
                               << std::endl;
    }
    if (reportAll || toReport.find("MEAN ") != std::string::npos)
    {
      attribs << preText.str() << "Mean" << delimiter << objMB->GetMean() << std::endl;
    }
    if (reportAll || toReport.find("SUM ") != std::string::npos)
    {
      attribs << preText.str() << "Sum" << delimiter << objMB->GetSum() << std::endl;
    }
    if (reportAll || toReport.find("STANDARDDEVIATION ") != std::string::npos)
    {
      attribs << preText.str() << "StandardDeviation" << delimiter
                               << objMB->GetStandardDeviation() << std::endl;
    }
    if (reportAll || toReport.find("VARIANCE ") != std::string::npos)
    {
      attribs << preText.str() << "Variance" << delimiter << objMB->GetVariance()
                               << std::endl;
    }
    if (reportAll || toReport.find("MEDIAN ") != std::string::npos)
    {
      attribs << preText.str() << "Median" << delimiter << objMB->GetMedian() << std::endl;
    }
    if (reportAll || toReport.find("SKEWNESS ") != std::string::npos)
    {
      attribs << preText.str() << "Skewness" << delimiter << objMB->GetSkewness()
                               << std::endl;
    }
    if (reportAll || toReport.find("KURTOSIS ") != std::string::npos)
    {
      attribs << preText.str() << "Kurtosis" << delimiter << objMB->GetKurtosis()
                               << std::endl;
    }
    if (reportAll || toReport.find("WEIGHTEDELONGATION ") != std::string::npos)
    {
      attribs << preText.str() << "WeightedElongation" << delimiter
                               << objMB->GetWeightedElongation() << std::endl;
    }
    if (reportAll || toReport.find("WEIGHTEDFLATNESS ") != std::string::npos)
    {
      attribs << preText.str() << "WeightedFlatness" << delimiter
                               << objMB->GetWeightedFlatness() << std::endl;
    }
    if (reportAll || toReport.find("MAXIMUMINDEX ") != std::string::npos)
    {
      attribs << preText.str() << "MaximumIndex" << delimiter << objMB->GetMaximumIndex()
                               << std::endl;
    }
    if (reportAll || toReport.find("MINIMUMINDEX ") != std::string::npos)
    {
      attribs << preText.str() << "MinimumIndex" << delimiter << objMB->GetMinimumIndex()
                               << std::endl;
    }
    if (reportAll || toReport.find("CENTEROFGRAVITY ") != std::string::npos)
    {
      attribs << preText.str() << "CenterOfGravity" << delimiter
                               << objMB->GetCenterOfGravity() << std::endl;
    }

    std::cout << attribs.str();
  }

  return EXIT_SUCCESS;
}
