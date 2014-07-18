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
    std::cerr << " fixedImage movingImage transferFile [invTransFile]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string fixedImage(argv[1]);
  std::string movingImage(argv[2]);
  std::string transferFile(argv[3]);

  std::string invTransferFile;
  if (argc == 5) invTransferFile = argv[4];

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
