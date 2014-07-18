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


#ifndef __itkStripTsImageFilter_h
#define __itkStripTsImageFilter_h


#include "itkImageToImageFilter.h"

#include "itkImageDuplicator.h"

#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkResampleImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"

#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryErodeImageFilter.h"

#include "itkBinaryThresholdImageFilter.h"
#include "itkGradientAnisotropicDiffusionImageFilter.h"
#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkSigmoidImageFilter.h"
#include "itkGeodesicActiveContourLevelSetImageFilter.h"
#include "itkCastImageFilter.h"

namespace itk
{

/** \class StripTsImageFilter
 * \brief composite ITK filter for automatic skull-stripping
 *
 * This ITK filter performs automatic skull stripping for 3D medical images
 *
 * It requires 2 inputs:
 * SetInput()
 * SetAtlasBrainMask()
 * and it outputs the brain mask for the patient image
 *
 * \warning images have to be 3D
 *
 * Inspired by:
 * Stefan Bauer
 * Institute for Surgical Technology and Biomechanics, University of Bern
 * stefan.bauer at istb.unibe.ch
 *
 *  This code is from the Insight Journal paper:
 *    "A Skull-Stripping Filter for ITK"
 *    Bauer S., Fejes T., Reyes M.
 *    http://hdl.handle.net/10380/3353
 *  Based on the paper:
 *    "Skull-stripping for Tumor-bearing Brain Images".
 *    Stefan Bauer, Lutz-Peter Nolte, and Mauricio Reyes.
 *    In Annual Meeting of the Swiss Society for Biomedical
 *    Engineering, page 2, Bern, April 2011..
 */

template <class TImageType, class TAtlasLabelType>
class ITK_EXPORT
StripTsImageFilter : public ImageToImageFilter<TImageType, TAtlasLabelType>
{
public:
  itkStaticConstMacro(SpaceDimension, unsigned int,
                      TImageType::ImageDimension);

  // standard class typedefs
  typedef StripTsImageFilter                              Self;
  typedef ImageToImageFilter<TImageType,TAtlasLabelType>  Superclass;
  typedef SmartPointer<Self>                              Pointer;
  typedef SmartPointer<const Self>                        ConstPointer;

  // method for creation through the object factory
  itkNewMacro(Self);

  // run-time type information (and related methods)
  itkTypeMacro(StripTsImageFilter, ImageToImageFilter);

  // image and label templates
  typedef TImageType                        ImageType;
  typedef typename ImageType::Pointer       ImagePointer;
  typedef typename ImageType::ConstPointer  ImageConstPointer;

  typedef TAtlasLabelType                       AtlasLabelType;
  typedef typename AtlasLabelType::Pointer      AtlasLabelPointer;
  typedef typename AtlasLabelType::ConstPointer AtlasLabelConstPointer;


  void SetSubjectImage( const TImageType *image)
    {
    this->ProcessObject::SetNthInput( 0, const_cast< TImageType * >( image ) );
    }
  void SetAtlasBrainMask(const TAtlasLabelType *image)
    {
    this->ProcessObject::SetNthInput( 1, const_cast< TAtlasLabelType * >( image ) );
    }

protected:

  StripTsImageFilter();
  ~StripTsImageFilter(){}

  // does the real work
  virtual void GenerateData();
  // display
  void PrintSelf( std::ostream& os, Indent indent ) const;

private:

  const TImageType * GetSubjectImage()
    {
    return static_cast<const TImageType*>(this->ProcessObject::GetInput(0));
    }

  const TAtlasLabelType * GetAtlasBrainMask()
    {
    return static_cast<const TAtlasLabelType*>(this->ProcessObject::GetInput(1));
    }

  StripTsImageFilter(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  ImagePointer      m_WorkingImage;
  AtlasLabelPointer m_AtlasMask;

  void RescaleImages();
  void DownsampleImage(int isoSpacing);
  void BinaryErosion();
  void MultiResLevelSet();
  void InversePyramidFilter();
  void LevelSetRefinement(int isoSpacing);
  void UpsampleLabels();

}; // end of class

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkStripTsImageFilter.hxx"
#endif

#endif
