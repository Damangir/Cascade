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
#ifndef __itkMaximumIndexVectorImageFilter_h
#define __itkMaximumIndexVectorImageFilter_h

#include "itkUnaryFunctorImageFilter.h"

#include "itkVectorImage.h"
#include "itkVectorContainer.h"
namespace itk
{
namespace Functor
{
template< typename TInput, typename TOutput>
class MaximumIndexFunxtor
{
public:

  typedef typename NumericTraits< TInput >::ScalarRealType RealValueType;

  MaximumIndexFunxtor()
  {
  }
  ~MaximumIndexFunxtor()
  {
  }
  bool operator!=(const MaximumIndexFunxtor &) const
  {
    return false;
  }

  bool operator==(const MaximumIndexFunxtor & other) const
  {
    return !(*this != other);
  }

  inline TOutput operator()(const TInput & A) const
  {
    unsigned int maximumIndex = 0;
    RealValueType maxVal = NumericTraits< RealValueType >::min();
    for (unsigned int i = 0; i < NumericTraits< TInput >::GetLength(A); i++)
    {
      if (maxVal < A[i])
      {
        maximumIndex = i;
        maxVal = A[i];
      }
    }
    return static_cast<TOutput>(maximumIndex);
  }
};
}

/** \class MaximumIndexVectorImageFilter
 */
template< typename TInputImage,
    typename TOutputImage = Image< unsigned char,
        TInputImage::ImageDimension > >
class MaximumIndexVectorImageFilter: public UnaryFunctorImageFilter< TInputImage,
    TOutputImage,
    Functor::MaximumIndexFunxtor< typename TInputImage::PixelType,
        typename TOutputImage::PixelType > >
{
public:
  /** Standard class typedefs. */
  typedef MaximumIndexVectorImageFilter Self;
  typedef UnaryFunctorImageFilter< TInputImage, TOutputImage,
      Functor::MaximumIndexFunxtor< typename TInputImage::PixelType,
          typename TOutputImage::PixelType > > Superclass;

  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
  ;

  /** Runtime information support. */
  itkTypeMacro(MaximumIndexVectorImageFilter,
      UnaryFunctorImageFilter)
  ;

  typedef typename TInputImage::PixelType InputPixelType;
  typedef typename TOutputImage::PixelType OutputPixelType;

protected:
  MaximumIndexVectorImageFilter()
  {
  }
  virtual ~MaximumIndexVectorImageFilter()
  {
  }

private:
  MaximumIndexVectorImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &); //purposely not implemented
};
} // end namespace itk

#endif
