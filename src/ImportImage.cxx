/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkImageUtil.h"

#include "itkGaussianDistribution.h"

#include "itkOrientImageFilter.h"

#include <map>

int
main(int argc, char *argv[])
{
  typedef std::map< std::string,
      itk::SpatialOrientation::ValidCoordinateOrientationFlags > OrientationNameMap;
  OrientationNameMap orientMap;

  orientMap["RIP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIP;
  orientMap["LIP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIP;
  orientMap["RSP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSP;
  orientMap["LSP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSP;
  orientMap["RIA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RIA;
  orientMap["LIA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LIA;
  orientMap["RSA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RSA;
  orientMap["LSA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LSA;
  orientMap["IRP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRP;
  orientMap["ILP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILP;
  orientMap["SRP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRP;
  orientMap["SLP"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLP;
  orientMap["IRA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IRA;
  orientMap["ILA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ILA;
  orientMap["SRA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SRA;
  orientMap["SLA"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SLA;
  orientMap["RPI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI;
  orientMap["LPI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPI;
  orientMap["RAI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAI;
  orientMap["LAI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAI;
  orientMap["RPS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPS;
  orientMap["LPS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LPS;
  orientMap["RAS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS;
  orientMap["LAS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS;
  orientMap["PRI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRI;
  orientMap["PLI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLI;
  orientMap["ARI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARI;
  orientMap["ALI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALI;
  orientMap["PRS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PRS;
  orientMap["PLS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PLS;
  orientMap["ARS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ARS;
  orientMap["ALS"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ALS;
  orientMap["IPR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPR;
  orientMap["SPR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPR;
  orientMap["IAR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAR;
  orientMap["SAR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAR;
  orientMap["IPL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IPL;
  orientMap["SPL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SPL;
  orientMap["IAL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_IAL;
  orientMap["SAL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_SAL;
  orientMap["PIR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIR;
  orientMap["PSR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSR;
  orientMap["AIR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIR;
  orientMap["ASR"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASR;
  orientMap["PIL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PIL;
  orientMap["PSL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_PSL;
  orientMap["AIL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_AIL;
  orientMap["ASL"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_ASL;

  orientMap["NEURO"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RAS;
  orientMap["MNI"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_RPI;
  orientMap["RADIO"] = itk::SpatialOrientation::ITK_COORDINATE_ORIENTATION_LAS;

  if (std::string(argv[1]) == "-h")
    {
    std::cout << "Usage: " << argv[0];
    std::cout << " input output Orientation";
    std::cout << std::endl;
    std::cout << "Valid Orientations: " << std::endl;

    for (OrientationNameMap::iterator it = orientMap.begin();
        it != orientMap.end(); ++it)
      {
      std::cout << it->first;
      if (it->first.length() == 3)
        {
        std::cout << " :";
        for (unsigned int i = 0; i < 3; i++)
          {
          char key = it->first.at(i);
          switch (key)
          {
            case 'R':
              std::cout << " Right-To-Left";
              break;
            case 'L':
              std::cout << " Left-To-Right";
              break;
            case 'I':
              std::cout << " Inferior-To-Superior";
              break;
            case 'S':
              std::cout << " Superior-To-Inferior";
              break;
            case 'A':
              std::cout << " Anterior-To-Posterior";
              break;
            case 'P':
              std::cout << " Posterior-To-Anterior";
              break;
            default:
              break;
          }
          }
        }
      std::cout << std::endl;
      }
    std::cout << std::endl;
    return EXIT_SUCCESS;
    }

  if (argc < 3)
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input output Orientation";
    std::cerr << std::endl;
    std::cerr << "Help: " << argv[0];
    std::cerr << " -h" << std::endl;
    return EXIT_FAILURE;
    }

  std::string input(argv[1]);
  std::string output(argv[2]);
  std::string orientation = "None";
  if (argc > 3) orientation = argv[3];

  typedef float PixelType;
  const unsigned int ImageDimension = 3;

  typedef itk::Image< PixelType, ImageDimension > ImageT;
  typedef itk::ImageUtil< ImageT > ImageUtilT;
  ImageT::Pointer image;
  try
    {
    image = ImageUtilT::ReadImage(input);
    }
  catch (itk::ExceptionObject &ex)
    {
    std::cerr << ex.GetDescription() << std::endl;
    return EXIT_FAILURE;
    }

  typedef itk::OrientImageFilter< ImageT, ImageT > OrientFilterType;
  typename OrientFilterType::Pointer orient = OrientFilterType::New();

  if (orientMap.find(orientation) != orientMap.end())
    {
    orient->SetDesiredCoordinateOrientation(orientMap[orientation]);
    }
  else
    {
    std::cerr << "No target orientation provided. The image will be saved with"
              " original orientation"
              << std::endl;

    ImageUtilT::WriteImage(output, image);

    return EXIT_SUCCESS;
    }

  orient->UseImageDirectionOn();
  orient->SetInput(image);
  orient->Update();

  ImageUtilT::WriteImage(output, orient->GetOutput());

  return EXIT_SUCCESS;
}

