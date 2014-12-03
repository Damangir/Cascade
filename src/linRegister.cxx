/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkAffineTransformCalculator.h"
#include "itkRigid3DTransformCalculator.h"

#include "itkImageUtil.h"

#include "itkTransformFileWriter.h"
#include "itkTransformFileReader.h"
#include "itkTransformFactoryBase.h"

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr
        << " fixedImage movingImage transferFile [invTransFile] [mode=intra]";
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

  typedef itk::ImageUtil< ImageType > ImageUtil;

  if (fixedImage == movingImage)
  {
    itk::TransformFileWriter::Pointer transWriter;
    transWriter = itk::TransformFileWriter::New();
    transWriter->SetInput(
        itk::IdentityTransform< double, SpaceDimension >::New());
    transWriter->SetFileName(transferFile);
    transWriter->Update();
    transWriter->SetFileName(invTransferFile);
    transWriter->Update();
    return EXIT_SUCCESS;
  }
  if (mode == "intra")
  {
    typedef itk::Rigid3DTransformCalculator< ImageType > Rigid3DCalculatorType;
    Rigid3DCalculatorType::Pointer rigidCalculator =
        Rigid3DCalculatorType::New();

    rigidCalculator->SetFixedImage(ImageUtil::ReadImage(fixedImage));
    rigidCalculator->SetMovingImage(ImageUtil::ReadImage(movingImage));
    rigidCalculator->Update();

    itk::TransformFileWriter::Pointer transWriter;
    transWriter = itk::TransformFileWriter::New();
    transWriter->SetInput(rigidCalculator->GetRigid3DTransform());
    transWriter->SetFileName(transferFile);
    transWriter->Update();

    if (invTransferFile != "")
    {
      transWriter->SetInput(
          rigidCalculator->GetRigid3DTransform()->GetInverseTransform());
      transWriter->SetFileName(invTransferFile);
      transWriter->Update();
    }
  }
  else
  {
    typedef itk::AffineTransformCalculator< ImageType > AffineCalculatorType;
    AffineCalculatorType::Pointer affineCalculator =
        AffineCalculatorType::New();

    affineCalculator->SetFixedImage(ImageUtil::ReadImage(fixedImage));
    affineCalculator->SetMovingImage(ImageUtil::ReadImage(movingImage));
    affineCalculator->Update();

    itk::TransformFileWriter::Pointer transWriter;
    transWriter = itk::TransformFileWriter::New();
    transWriter->SetInput(affineCalculator->GetAffineTransform());
    transWriter->SetFileName(transferFile);
    transWriter->Update();

    if (invTransferFile != "")
    {
      transWriter->SetInput(
          affineCalculator->GetAffineTransform()->GetInverseTransform());
      transWriter->SetFileName(invTransferFile);
      transWriter->Update();
    }
  }
  return EXIT_SUCCESS;
}
