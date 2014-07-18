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
#ifndef __itkNeighborhoodMaskedBinaryThresholdImageFunction_h
#define __itkNeighborhoodMaskedBinaryThresholdImageFunction_h

#include "itkImageFunction.h"

namespace itk
{
/**
 * \class NeighborhoodMaskedBinaryThresholdImageFunction
 * \brief Determine whether masked portion of the pixels in the specified
 * neighborhood is smaller/larger than the the center pixel
 *
 * If called with a ContinuousIndex or Point, the calculation is performed
 * at the nearest neighbor.
 */
template< typename TInputImage, typename TMaskImage, typename TCoordRep = float >
class NeighborhoodMaskedBinaryThresholdImageFunction:
  public ImageFunction< TInputImage, bool, TCoordRep >
{
public:
  /** Standard class typedefs. */
  typedef NeighborhoodMaskedBinaryThresholdImageFunction               Self;
  typedef ImageFunction< TInputImage, bool, TCoordRep > Superclass;
  typedef SmartPointer< Self >                                   Pointer;
  typedef SmartPointer< const Self >                             ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(NeighborhoodMaskedBinaryThresholdImageFunction, ImageFunction);

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** InputImageType typedef support. */
  typedef TInputImage InputImageType;

  /** InputPixel typedef support */
  typedef typename InputImageType::PixelType PixelType;

  /** MaskImageType typedef support. */
  typedef TMaskImage MaskImageType;

  /** MaskPixel typedef support */
  typedef typename MaskImageType::PixelType MaskPixelType;

  /** MaskImagePointer typedef support */
  typedef typename MaskImageType::ConstPointer MaskImageConstPointer;


  /** OutputType typdef support. */
  typedef typename Superclass::OutputType OutputType;

  /** Index typedef support. */
  typedef typename Superclass::IndexType IndexType;

  /** ContinuousIndex typedef support. */
  typedef typename Superclass::ContinuousIndexType ContinuousIndexType;

  /** Point typedef support. */
  typedef typename Superclass::PointType PointType;

  /** Dimension of the underlying image. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      InputImageType::ImageDimension);

  /** SizeType of the input image */
  typedef typename InputImageType::SizeType InputSizeType;

  /** Set the radius of the neighborhood used in computation. */
  itkSetMacro(Radius, InputSizeType);

  /** Get the radius of the neighborhood used in computation */
  itkGetConstReferenceMacro(Radius, InputSizeType);

  /** Evalulate the function at specified index */
  virtual bool EvaluateAtIndex(const IndexType & index) const;

  /** Evaluate the function at non-integer positions */
  virtual bool Evaluate(const PointType & point) const
  {
    IndexType index;

    this->ConvertPointToNearestIndex(point, index);
    return this->EvaluateAtIndex(index);
  }

  virtual bool EvaluateAtContinuousIndex(
    const ContinuousIndexType & cindex) const
  {
    IndexType index;

    this->ConvertContinuousIndexToNearestIndex(cindex, index);
    return this->EvaluateAtIndex(index);
  }

  virtual void SetInputImage(const InputImageType *ptr);
  virtual void SetMaskImage(const MaskImageType *ptr);

  /** Get the mask image. */
  const MaskImageType * GetMaskImage() const
  { return m_MaskImage.GetPointer(); }

  itkSetMacro(HighHill, bool);
  itkGetMacro(HighHill, bool);
  itkBooleanMacro(HighHill);

  itkSetMacro(AcceptanceRatio, float);
  itkGetMacro(AcceptanceRatio, float);

protected:
  NeighborhoodMaskedBinaryThresholdImageFunction();
  ~NeighborhoodMaskedBinaryThresholdImageFunction(){}
  void PrintSelf(std::ostream & os, Indent indent) const;

private:
  NeighborhoodMaskedBinaryThresholdImageFunction(const Self &); //purposely not
                                                          // implemented
  void operator=(const Self &);                           //purposely not

  // implemented

  bool isInCorrectDirection(PixelType a ,PixelType b) const
  {
    if(m_HighHill)
    {
      return (a >= b);
    }else
    {
      return (a <= b);
    }
  }

  InputSizeType m_Radius;

  MaskImageConstPointer m_MaskImage;

  bool m_HighHill;
  float m_AcceptanceRatio;
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkNeighborhoodMaskedBinaryThresholdImageFunction.hxx"
#endif

#endif
