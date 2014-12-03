/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"

#include "itkImageUtil.h"

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " ModelVector OutputModelScalar";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string modelVec(argv[1]);
  std::string ModelScl(argv[2]);

  const unsigned int ImageDimension = 3;
  const unsigned int SpaceDimension = ImageDimension;
  typedef float PixelType;

  typedef itk::Image< PixelType, ImageDimension > ImageType;
  typedef itk::VectorImage< PixelType, ImageDimension > VectorImageType;
  typedef VectorImageType::PixelType VectorPixelType;
  typedef itk::ImageRegionConstIterator< VectorImageType > VectorImageIterator;
  typedef itk::ImageRegionIterator< ImageType > ImageIterator;

  typedef itk::ImageUtil<VectorImageType> VectorImageUtil;
  typedef itk::ImageUtil<ImageType> ImageUtil;

  VectorImageType::Pointer modelVectorImage = VectorImageUtil::ReadImage(
      modelVec);

  ImageType::Pointer modelScalarImage = ImageType::New();
  modelScalarImage->CopyInformation(modelVectorImage);
  modelScalarImage->SetRequestedRegion(
      modelVectorImage->GetLargestPossibleRegion());
  modelScalarImage->SetBufferedRegion(modelVectorImage->GetBufferedRegion());
  modelScalarImage->Allocate();

  VectorImageIterator vmIt(modelVectorImage,
                           modelVectorImage->GetLargestPossibleRegion());
  ImageIterator smIt(modelScalarImage,
                     modelVectorImage->GetLargestPossibleRegion());

  size_t vecLen = modelVectorImage->GetNumberOfComponentsPerPixel();

  while (!vmIt.IsAtEnd())
  {
    size_t nz_count = 0;
    double total = 0;
    VectorPixelType px = vmIt.Get();
    for (size_t i = 0; i < vecLen; i++)
    {
      if(px[i] > 0)
      {
        total += px[i];
        nz_count ++;
      }
    }
    if(nz_count >0)
    {
      smIt.Set(total / nz_count);
    }else
    {
      smIt.Set(0);
    }
    ++vmIt;
    ++smIt;
  }

  ImageUtil::WriteImage(ModelScl, modelScalarImage);
  return EXIT_SUCCESS;
}

