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

#include "itkN4ImageFilter.h"
#include "imageHelpers.h"

namespace CU = cascade::util;


int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input mask output";
    std::cerr << std::endl;
    return EXIT_FAILURE;
  }

  std::string inputImage(argv[1]);
  std::string maskImage(argv[2]);
  std::string outputImage(argv[3]);

  const unsigned int ImageDimension = 3;
  typedef float PixelType;
  typedef double CoordinateRepType;
  const unsigned int SpaceDimension = ImageDimension;

  typedef itk::Image< PixelType, ImageDimension > ImageType;

  typedef itk::N4ImageFilter<ImageType> N4CorrectorType;
  N4CorrectorType::Pointer n4Corrector = N4CorrectorType::New();
  n4Corrector->SetInput(0,CU::LoadImage<ImageType>(inputImage));
  n4Corrector->SetInput(1,CU::LoadImage<ImageType>(maskImage));
  CU::WriteImage<ImageType>(outputImage, n4Corrector->GetOutput());

  return EXIT_SUCCESS;
}
