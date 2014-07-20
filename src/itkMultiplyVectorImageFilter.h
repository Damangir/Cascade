/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkMultiplyVectorImageFilter_h
#define __itkMultiplyVectorImageFilter_h

#include "itkBinaryFunctorImageFilter.h"

namespace itk
{
namespace Functor
{
template< typename TInput1, typename TInput2, typename TOutput >
class MultiplyVectorFunctor
{
public:

  MultiplyVectorFunctor()
  {
  }
  virtual ~MultiplyVectorFunctor()
  {
  }
  bool operator!=(const MultiplyVectorFunctor &) const
  {
    return false;
  }

  bool operator==(const MultiplyVectorFunctor & other) const
  {
    return !(*this != other);
  }

  inline TOutput operator()(const TInput1 & A, const TInput2 & B) const
  {
    const unsigned int vectorDimension = NumericTraits< TInput1 >::GetLength(A);

    TOutput result;
    NumericTraits< TOutput >::SetLength(result, vectorDimension);

    for (unsigned int i = 0; i < vectorDimension; i++)
    {
      result[i] = A[i] * B[i];
    }
    return result;
  }
};
}

/** \class MultiplyVectorImageFilter
 */
template< typename TInputImage1, typename TInputImage2 = TInputImage1,
    typename TOutputImage = TInputImage1 >
class MultiplyVectorImageFilter: public BinaryFunctorImageFilter< TInputImage1,
    TInputImage2, TOutputImage,
    Functor::MultiplyVectorFunctor< typename TInputImage1::PixelType,
        typename TInputImage2::PixelType, typename TOutputImage::PixelType > >
{
public:
  /** Standard class typedefs. */
  typedef MultiplyVectorImageFilter Self;
  typedef BinaryFunctorImageFilter< TInputImage1, TInputImage2, TOutputImage,
      Functor::MultiplyVectorFunctor< typename TInputImage1::PixelType,
          typename TInputImage2::PixelType, typename TOutputImage::PixelType > > Superclass;

  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
  ;

  /** Runtime information support. */
  itkTypeMacro(MultiplyVectorImageFilter,
      UnaryFunctorImageFilter)
  ;

  typedef typename TInputImage1::PixelType Input1PixelType;
  typedef typename TInputImage2::PixelType Input2PixelType;
  typedef typename TOutputImage::PixelType OutputPixelType;

protected:
  virtual void GenerateOutputInformation()
  {
    this->Superclass::GenerateOutputInformation();
    TOutputImage *output = this->GetOutput();
    output->SetNumberOfComponentsPerPixel(
        this->GetInput()->GetNumberOfComponentsPerPixel());
  }

  MultiplyVectorImageFilter()
  {
  }
  virtual ~MultiplyVectorImageFilter()
  {
  }

private:
  MultiplyVectorImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &); //purposely not implemented
};
} // end namespace itk

#endif
