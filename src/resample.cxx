/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#include "itkCompositeTransform.h"

#include "itkIdentityTransform.h"

#include "itkResampleImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkTransformFileReader.h"
#include "itkTransformFactoryBase.h"

#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"

int main(int argc, char *argv[])
{
  if (argc < 5)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " fixedImage movingImage output transferFile [interpolator]";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string fixedImage(argv[1]);
  std::string movingImage(argv[2]);
  std::string output(argv[3]);

  std::string transferFile;
  std::string interpolator;

  if (argc > 4) transferFile = argv[4];
  if (argc > 5) interpolator = argv[5];

  const unsigned int ImageDimension = 3;
  typedef float PixelType;
  typedef double CoordinateRepType;
  const unsigned int SpaceDimension = ImageDimension;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::ImageFileReader< ImageType > ImageReaderType;
  typedef itk::CompositeTransform< CoordinateRepType, ImageDimension > CompositeTransformType;

  ImageReaderType::Pointer fixedImageReader = ImageReaderType::New();
  ImageReaderType::Pointer movingImageReader = ImageReaderType::New();
  fixedImageReader->SetFileName(fixedImage);
  movingImageReader->SetFileName(movingImage);

  fixedImageReader->Update();
  movingImageReader->Update();

  CompositeTransformType::Pointer transform = CompositeTransformType::New();

  if (transferFile != "")
  {
    itk::TransformFactoryBase::RegisterDefaultTransforms();

    itk::TransformFileReader::Pointer transReader =
        itk::TransformFileReader::New();
    transReader->SetFileName(transferFile);
    transReader->Update();
    typedef itk::TransformFileReader::TransformListType TransformListType;
    TransformListType* transformations = transReader->GetTransformList();
    for (TransformListType::iterator transIt = transformations->begin();
        transIt != transformations->end(); ++transIt)
    {
      CompositeTransformType::TransformType *trans =
          dynamic_cast< CompositeTransformType::TransformType * >(transIt->GetPointer());
      if (trans)
      {
        transform->AddTransform(trans);
      }
      else
      {
        itkGenericExceptionMacro(<< "Can not add transformation");
      }
    }
  }
  else
  {
    typedef itk::IdentityTransform<CoordinateRepType, ImageDimension> EyeType;
    EyeType::Pointer identity = EyeType::New();
    transform->AddTransform(identity);
  }

  typedef itk::NearestNeighborInterpolateImageFunction< ImageType, double > NNInterpolatorType;
  typedef itk::LinearInterpolateImageFunction< ImageType, double > LinInterpolatorType;

  typedef itk::ResampleImageFilter< ImageType, ImageType > ResampleFilterType;
  ResampleFilterType::Pointer resample = ResampleFilterType::New();

  if (interpolator == "nn")
  {
    NNInterpolatorType::Pointer nnInterp = NNInterpolatorType::New();
    resample->SetInterpolator(nnInterp);
  }
  else
  {
    LinInterpolatorType::Pointer linInterp = LinInterpolatorType::New();
    resample->SetInterpolator(linInterp);
  }

  resample->SetTransform(transform);
  resample->SetInput(movingImageReader->GetOutput());
  resample->SetSize(
      fixedImageReader->GetOutput()->GetLargestPossibleRegion().GetSize());
  resample->SetOutputOrigin(fixedImageReader->GetOutput()->GetOrigin());
  resample->SetOutputSpacing(fixedImageReader->GetOutput()->GetSpacing());
  resample->SetOutputDirection(fixedImageReader->GetOutput()->GetDirection());
  resample->SetDefaultPixelValue(0);

  resample->Update();

  typedef itk::ImageFileWriter< ImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetInput(resample->GetOutput());
  writer->SetFileName(output);
  writer->Update();

  return EXIT_SUCCESS;
}
