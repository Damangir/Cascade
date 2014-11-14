#include "itkImageRegionIterator.h"
#include "itkBinaryImageToShapeLabelMapFilter.h"
#include "itkShapeOpeningLabelMapFilter.h"
#include "itkLabelMapToLabelImageFilter.h"

#include "imageHelpers.h"
namespace CU = cascade::util;

int
main(int argc, char *argv[])
{
  if (argc < 5)
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image output attribute threshold u";
    std::cerr << std::endl;
    return EXIT_FAILURE;
    }

  std::string inputImg(argv[1]);
  std::string outputImg(argv[2]);
  std::string attribute(argv[3]);
  float threshold = atof(argv[4]);
  bool reverse = false;
  if (argc > 5) reverse = std::string(argv[4]) == "reverse";

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef unsigned char LabelType;

  itk::ShapeLabelObject< LabelType, ImageDimension >::GetAttributeFromName(attribute);

  typedef itk::Image< LabelType, ImageDimension > LabelImageType;

  LabelImageType::Pointer image = CU::LoadImage < LabelImageType
      > (inputImg);

  // Create a ShapeLabelMap from the image
  typedef itk::BinaryImageToShapeLabelMapFilter< LabelImageType > BinaryImageToShapeLabelMapFilterType;
  BinaryImageToShapeLabelMapFilterType::Pointer binaryImageToShapeLabelMapFilter =
      BinaryImageToShapeLabelMapFilterType::New();
  binaryImageToShapeLabelMapFilter->SetInput(image);
  binaryImageToShapeLabelMapFilter->SetInputForegroundValue(1);
  binaryImageToShapeLabelMapFilter->Update();

  // Remove label objects that have PERIMETER less than 50
  typedef itk::ShapeOpeningLabelMapFilter<
      BinaryImageToShapeLabelMapFilterType::OutputImageType > ShapeOpeningLabelMapFilterType;
  ShapeOpeningLabelMapFilterType::Pointer shapeOpeningLabelMapFilter =
      ShapeOpeningLabelMapFilterType::New();
  shapeOpeningLabelMapFilter->SetInput(
      binaryImageToShapeLabelMapFilter->GetOutput());
  shapeOpeningLabelMapFilter->SetLambda(threshold);
  if (reverse)
    {
    shapeOpeningLabelMapFilter->ReverseOrderingOn();
    }
  else
    {
    shapeOpeningLabelMapFilter->ReverseOrderingOff();
    }

  shapeOpeningLabelMapFilter->SetAttribute(attribute);
  shapeOpeningLabelMapFilter->Update();

  // Create a label image
  typedef itk::LabelMapToLabelImageFilter<
      BinaryImageToShapeLabelMapFilterType::OutputImageType, LabelImageType > LabelMapToLabelImageFilterType;
  LabelMapToLabelImageFilterType::Pointer labelMapToLabelImageFilter =
      LabelMapToLabelImageFilterType::New();
  labelMapToLabelImageFilter->SetInput(shapeOpeningLabelMapFilter->GetOutput());
  labelMapToLabelImageFilter->Update();

  CU::WriteImage< LabelImageType >(outputImg, labelMapToLabelImageFilter->GetOutput());

  return EXIT_SUCCESS;
}
