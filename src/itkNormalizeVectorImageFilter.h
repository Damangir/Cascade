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
#ifndef __itkNormalizeVectorImageFilter_h
#define __itkNormalizeVectorImageFilter_h

#include "itkUnaryFunctorImageFilter.h"

#include "itkVectorImage.h"
#include "itkVectorContainer.h"
namespace itk
{
namespace Functor
{
template< typename TInput, typename TOutput >
class NormalizeVectorFunctor
{
public:
  NormalizeVectorFunctor()
  {
    m_Bias = 0;
  }
  virtual ~NormalizeVectorFunctor()
  {
  }
  bool operator!=(const NormalizeVectorFunctor & other) const
  {
    return this->m_Bias != other.m_Bias;
  }

  bool operator==(const NormalizeVectorFunctor & other) const
  {
    return !(*this != other);
  }

  inline TOutput operator()(const TInput & A) const
  {
    const unsigned int vectorDimension = NumericTraits< TInput >::GetLength(A);

    TOutput result;
    NumericTraits< TOutput >::SetLength(result, vectorDimension);
    double sum = 0;

    for (unsigned int i = 0; i < vectorDimension; i++)
    {
      sum += A[i];
    }

    for (unsigned int i = 0; i < vectorDimension; i++)
    {
      result[i] = (A[i] / sum + m_Bias) / (1 + m_Bias * vectorDimension);
    }
    return result;
  }
  void SetBias(const double _arg)
  {
    this->m_Bias = _arg;
  }
  double GetBias()
  {
    return this->m_Bias;
  }
private:
  double m_Bias;
};

}

/** \class NormalizeVectorImageFilter
 */
template< typename TInputImage, typename TOutputImage = TInputImage >
class NormalizeVectorImageFilter: public UnaryFunctorImageFilter< TInputImage,
    TOutputImage,
    Functor::NormalizeVectorFunctor< typename TInputImage::PixelType,
        typename TOutputImage::PixelType > >
{
public:
  /** Standard class typedefs. */
  typedef NormalizeVectorImageFilter Self;
  typedef UnaryFunctorImageFilter< TInputImage, TOutputImage,
      Functor::NormalizeVectorFunctor< typename TInputImage::PixelType,
          typename TOutputImage::PixelType > > Superclass;

  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
  ;

  /** Runtime information support. */
  itkTypeMacro(NormalizeVectorImageFilter,
      UnaryFunctorImageFilter)
  ;

  virtual void SetBias(const double _arg)
  {
    itkDebugMacro("setting Bias to " << _arg);
    if (this->GetBias() != _arg)
    {
      this->GetFunctor().SetBias(_arg);
      this->Modified();
    }
  }

  virtual double GetBias()
  {
    return this->GetFunctor().GetBias();
  }

  typedef typename TInputImage::PixelType InputPixelType;
  typedef typename TOutputImage::PixelType OutputPixelType;

protected:
  virtual void GenerateOutputInformation()
  {
    this->Superclass::GenerateOutputInformation();
    TOutputImage *output = this->GetOutput();
    output->SetNumberOfComponentsPerPixel(
        this->GetInput()->GetNumberOfComponentsPerPixel());
  }

  NormalizeVectorImageFilter()
  {
  }
  virtual ~NormalizeVectorImageFilter()
  {
  }

private:
  NormalizeVectorImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &); //purposely not implemented
};
} // end namespace itk

#endif
