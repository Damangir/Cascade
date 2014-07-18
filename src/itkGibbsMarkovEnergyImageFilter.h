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
#ifndef __itkGibbsMarkovEnergyImageFilter_h
#define __itkGibbsMarkovEnergyImageFilter_h

#include "itkBoxImageFilter.h"
#include "itkImage.h"
#include "itkVectorImage.h"
#include "itkVectorContainer.h"

namespace itk
{
/** \class GibbsMarkovEnergyImageFilter
 * \brief Applies a GibbsMarkovEnergy filter to an image
 * \endwiki
 */
template< typename TInputImage, typename TProbabilityPrecision=float,
    typename TOutputImage=VectorImage< TProbabilityPrecision,
    TInputImage::ImageDimension > >
class GibbsMarkovEnergyImageFilter:
  public BoxImageFilter< TInputImage, TOutputImage >
{
public:
  /** Extract dimension from input and output image. */
  itkStaticConstMacro(InputImageDimension, unsigned int,
                      TInputImage::ImageDimension);
  itkStaticConstMacro(OutputImageDimension, unsigned int,
                      TOutputImage::ImageDimension);

  /** Convenient typedefs for simplifying declarations. */
  typedef TInputImage  InputImageType;
  typedef TOutputImage OutputImageType;

  /** Standard class typedefs. */
  typedef GibbsMarkovEnergyImageFilter                                     Self;
  typedef ImageToImageFilter< InputImageType, OutputImageType > Superclass;
  typedef SmartPointer< Self >                                  Pointer;
  typedef SmartPointer< const Self >                            ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(GibbsMarkovEnergyImageFilter, BoxImageFilter);

  /** Image typedef support. */
  typedef typename InputImageType::PixelType  InputPixelType;
  typedef typename OutputImageType::PixelType OutputPixelType;

  typedef typename InputImageType::RegionType  InputImageRegionType;
  typedef typename OutputImageType::RegionType OutputImageRegionType;

  typedef typename InputImageType::SizeType InputSizeType;

#ifdef ITK_USE_CONCEPT_CHECKING
  // Begin concept checking
  itkConceptMacro( SameDimensionCheck,
                   ( Concept::SameDimension< InputImageDimension, OutputImageDimension > ) );
  itkConceptMacro( DoubleConvertibleToOutputCheck,
                   ( Concept::Convertible< double, OutputPixelType > ) );
  // End concept checking
#endif

  unsigned int GetNumberOfClasses()const
  {
    return m_ClassIDs->Size();
  }

  void AddClass(const InputPixelType& p);
  void ClearClasses();
protected:
  GibbsMarkovEnergyImageFilter();
  virtual ~GibbsMarkovEnergyImageFilter() {}

  virtual void GenerateOutputInformation();

  void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
                            ThreadIdType threadId);

private:
  GibbsMarkovEnergyImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);    //purposely not implemented
  typedef VectorContainer<unsigned int, InputPixelType> InputClassContainerType;
  typedef typename InputClassContainerType::ConstIterator ClassIteratorType;

  typename InputClassContainerType::Pointer m_ClassIDs;
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkGibbsMarkovEnergyImageFilter.hxx"
#endif

#endif
