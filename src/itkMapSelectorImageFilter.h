/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkMapSelectorImageFilter_h
#define __itkMapSelectorImageFilter_h

#include "itkImageToImageFilter.h"

namespace itk
{
/** \class MapSelectorImageFilter
 * \brief Combines two images in a MapSelector pattern.
 *
 * MapSelectorImageFilter takes two input images that must have the same
 * dimension, size, origin and spacing and produces an output image of the same
 * size by combinining the pixels from the two input images in a MapSelector
 * pattern. This filter is commonly used for visually comparing two images, in
 * particular for evaluating the results of an image registration process.
 *
 * This filter is implemented as a multithreaded filter.  It provides a
 * ThreadedGenerateData() method for its implementation.
 *
 * \ingroup IntensityImageFilters  MultiThreaded
 * \ingroup ITKImageCompare
 *
 * \wiki
 * \wikiexample{Inspection/MapSelectorImageFilter,Combine two images by alternating blocks of a MapSelector pattern}
 * \endwiki
 */
template< typename TMap, typename TImage >
class MapSelectorImageFilter:
  public ImageToImageFilter< TImage, TImage >
{
public:
  /** Standard class typedefs. */
  typedef MapSelectorImageFilter              Self;
  typedef ImageToImageFilter< TImage, TImage > Superclass;
  typedef SmartPointer< Self >                 Pointer;
  typedef SmartPointer< const Self >           ConstPointer;

  typedef TMap                                  MapImageType;
  typedef TImage                                InputImageType;
  typedef TImage                                OutputImageType;
  typedef typename InputImageType::ConstPointer InputImageConstPointer;
  typedef typename OutputImageType::Pointer     OutputImagePointer;
  typedef typename OutputImageType::RegionType  ImageRegionType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(MapSelectorImageFilter, ImageToImageFilter);

  /** Number of dimensions. */
  itkStaticConstMacro(ImageDimension, unsigned int,
                      TImage::ImageDimension);

  /** Connect one of the operands for checker board */
  void SetMap(const MapImageType *image1);
  /** Connect one of the operands for checker board */

protected:
  MapSelectorImageFilter();
  ~MapSelectorImageFilter() {}
  void PrintSelf(std::ostream & os, Indent indent) const;

  const MapImageType * GetMap();

  void ThreadedGenerateData(const ImageRegionType & outputRegionForThread,
                            ThreadIdType threadId);

private:
  MapSelectorImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &);          //purposely not implemented

};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkMapSelectorImageFilter.hxx"
#endif

#endif
