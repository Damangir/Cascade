/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkAffineTransformCalculator.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkTransformFileWriter.h"
#include "itkTransformFileReader.h"
#include "itkTransformFactoryBase.h"

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " fixedImage movingImage transferFile [invTransFile] [mode=intra]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string fixedImage(argv[1]);
  std::string movingImage(argv[2]);
  std::string transferFile(argv[3]);

  std::string invTransferFile;
  if (argc > 4) invTransferFile = argv[4];

  std::string mode;
  if (argc > 5) mode = argv[5];

  const unsigned int ImageDimension = 3;
  typedef signed short PixelType;
  typedef itk::Image< PixelType, ImageDimension > ImageType;
  const unsigned int SpaceDimension = ImageDimension;

  typedef itk::ImageFileReader< ImageType > ImageReaderType;

  typedef itk::AffineTransformCalculator< ImageType > AffineCalculatorType;

  if (fixedImage != movingImage)
  {
    AffineCalculatorType::Pointer affineCalculator =
        AffineCalculatorType::New();
    ImageReaderType::Pointer fixedImageReader = ImageReaderType::New();
    ImageReaderType::Pointer movingImageReader = ImageReaderType::New();
    fixedImageReader->SetFileName(fixedImage);
    movingImageReader->SetFileName(movingImage);

    affineCalculator->SetFixedImage(fixedImageReader->GetOutput());
    affineCalculator->SetMovingImage(movingImageReader->GetOutput());
    /*
     * TODO: Make intra registration robust on partial brain.
     */
    if (mode == "intra")
    {
      affineCalculator->IntraRegistrationOn();
    }else
    {
      affineCalculator->IntraRegistrationOff();
    }
    affineCalculator->Update();
    itk::TransformFileWriter::Pointer transWriter;
    transWriter = itk::TransformFileWriter::New();
    transWriter->SetInput(affineCalculator->GetAffineTransform());
    transWriter->SetFileName(transferFile);
    transWriter->Update();

    if (invTransferFile != "")
    {
      transWriter->SetInput(affineCalculator->GetAffineTransform()->GetInverseTransform());
      transWriter->SetFileName(invTransferFile);
      transWriter->Update();
    }
  }else{
    itk::TransformFileWriter::Pointer transWriter;
    transWriter = itk::TransformFileWriter::New();
    transWriter->SetInput(itk::IdentityTransform<double, SpaceDimension>::New());
    transWriter->SetFileName(transferFile);
    transWriter->Update();
    transWriter->SetFileName(invTransferFile);
    transWriter->Update();
  }
  return EXIT_SUCCESS;
}
