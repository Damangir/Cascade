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

#include "itkStripTsImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

int main( int argc, char *argv[] )
{
  if( argc < 4 )
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image  stdBrainMask outputBrainMask";
    std::cerr << std::endl;
    return EXIT_FAILURE;
    }

  std::string subjectImage(argv[1]);
  std::string standardBrainMask(argv[2]);
  std::string outputBrainMask(argv[3]);

  const    unsigned int    ImageDimension = 3;
  typedef  signed short    PixelType;
  typedef itk::Image< PixelType, ImageDimension >  ImageType;
  typedef itk::Image< PixelType, ImageDimension >  MaskImageType;
  const unsigned int SpaceDimension = ImageDimension;
  const unsigned int SplineOrder = 3;

  typedef itk::ImageFileReader< ImageType  > ImageReaderType;
  typedef itk::ImageFileReader< MaskImageType > MaskImageReaderType;
  ImageReaderType::Pointer  imageReader  = ImageReaderType::New();
  MaskImageReaderType::Pointer atlasBrainReader = MaskImageReaderType::New();
  imageReader->SetFileName(  subjectImage );
  atlasBrainReader->SetFileName( standardBrainMask );

  imageReader->Update();
  atlasBrainReader->Update();

  typedef itk::StripTsImageFilter<ImageType, MaskImageType> BrainExtractorType;
  BrainExtractorType::Pointer brainExtractor = BrainExtractorType::New();

  brainExtractor->SetSubjectImage(imageReader->GetOutput());
  brainExtractor->SetAtlasBrainMask(atlasBrainReader->GetOutput());

  brainExtractor->Update();

  typedef itk::ImageFileWriter< ImageType >  WriterType;
  WriterType::Pointer writer =  WriterType::New();
  writer->SetInput(brainExtractor->GetOutput());
  writer->SetFileName( outputBrainMask );
  writer->Update();

  return EXIT_SUCCESS;
}
