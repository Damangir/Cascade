/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkNeighborhoodMaskedBinaryThresholdImageFunction_hxx
#define __itkNeighborhoodMaskedBinaryThresholdImageFunction_hxx

#include "itkNeighborhoodMaskedBinaryThresholdImageFunction.h"
#include "itkNumericTraits.h"
#include "itkConstNeighborhoodIterator.h"

namespace itk
{
/**
 * Constructor
 */
template< typename TInputImage, typename TMaskImage, typename TCoordRep >
NeighborhoodMaskedBinaryThresholdImageFunction< TInputImage, TMaskImage,
    TCoordRep >::NeighborhoodMaskedBinaryThresholdImageFunction()
{
  m_Radius.Fill(1);
  m_HighHill = true;
  m_AcceptanceRatio = 1;
}

/**
 *
 */
template< typename TInputImage, typename TMaskImage, typename TCoordRep >
void NeighborhoodMaskedBinaryThresholdImageFunction< TInputImage, TMaskImage,
    TCoordRep >::PrintSelf(std::ostream & os, Indent indent) const
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Radius: " << m_Radius << std::endl;
}

/**
 *
 */
template< typename TInputImage, typename TMaskImage, typename TCoordRep >
bool NeighborhoodMaskedBinaryThresholdImageFunction< TInputImage, TMaskImage,
    TCoordRep >::EvaluateAtIndex(const IndexType & index) const
{
  if (!this->GetInputImage() || !this->GetMaskImage())
  {
    return (false);
  }

  if (!this->IsInsideBuffer(index))
  {
    return (false);
  }

  // Create an N-d neighborhood kernel, using a zeroflux boundary condition
  ConstNeighborhoodIterator< InputImageType > imageIt(
      m_Radius, this->GetInputImage(),
      this->GetInputImage()->GetBufferedRegion());

  ConstNeighborhoodIterator< MaskImageType > maskIt(
      m_Radius, this->GetMaskImage(),
      this->GetMaskImage()->GetBufferedRegion());

  // Set the iterator at the desired location
  imageIt.SetLocation(index);
  maskIt.SetLocation(index);

  // Walk the neighborhood
  unsigned int numberInBand = 0;
  unsigned int numberInMask = 0;
  PixelType value;
  MaskPixelType maskValue;
  const unsigned int size = imageIt.Size();
  for (unsigned int i = 0; i < size; ++i)
  {
    maskValue = maskIt.GetPixel(i);
    if (maskValue > NumericTraits< MaskPixelType >::ZeroValue(maskValue))
    {
      if (this->isInCorrectDirection(imageIt.GetCenterPixel(),imageIt.GetPixel(i)))
      {
        numberInBand++;
      }
      numberInMask++;
    }
  }
  if (numberInMask > 0)
  {
    if(size == numberInMask)
    {
      return true;
    }
    else if (numberInBand >= m_AcceptanceRatio * numberInMask )
    {
      std::cout << imageIt.GetCenterPixel() << std::endl;
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

template< typename TInputImage, typename TMaskImage, typename TCoordRep >
void NeighborhoodMaskedBinaryThresholdImageFunction< TInputImage, TMaskImage,
    TCoordRep >::SetMaskImage(const MaskImageType *ptr)
{
  // set the input image
  m_MaskImage = ptr;

  if (ptr && this->GetInputImage())
  {
    typename InputImageType::SizeType size = ptr->GetBufferedRegion().GetSize();
    itkAssertOrThrowMacro(
        Superclass::m_StartIndex == ptr->GetBufferedRegion().GetIndex(),
        "Start Index does not match");

    for (unsigned int j = 0; j < ImageDimension; j++)
    {
      itkAssertOrThrowMacro(
          Superclass::m_EndIndex[j] == Superclass::m_StartIndex[j]
              + static_cast< IndexValueType >(size[j])
                                       - 1,
          "End index does not match.");
      itkAssertOrThrowMacro(
          Superclass::m_StartContinuousIndex[j] == static_cast< TCoordRep >(Superclass::m_StartIndex[j]
              - 0.5),
          "Start continuous index does not match.");
      itkAssertOrThrowMacro(
          Superclass::m_EndContinuousIndex[j] == static_cast< TCoordRep >(Superclass::m_EndIndex[j]
              + 0.5),
          "End continuous index does not match.");
    }
  }
}

template< typename TInputImage, typename TMaskImage, typename TCoordRep >
void NeighborhoodMaskedBinaryThresholdImageFunction< TInputImage, TMaskImage,
    TCoordRep >::SetInputImage(const InputImageType *ptr)
{
  // set the input image
  Superclass::SetInputImage(ptr);

  if (ptr && this->GetMaskImage())
  {
    // Enforce checking consistency between mask and input image.
    this->SetMaskImage(this->GetMaskImage());
  }
}

} // end namespace itk

#endif
