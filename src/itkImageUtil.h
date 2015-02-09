/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkImageUtil_h
#define __itkImageUtil_h

#include "itkObject.h"
#include "itkObjectFactory.h"
#include "itkImageSource.h"

#include <string>
#include <vector>

namespace itk
{

template< typename TImage >
class ImageUtil: public Object
{
public:

  typedef ImageUtil< TImage > Self;
  typedef Object Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  typedef TImage ImageType;
  typedef typename ImageType::Pointer ImagePointer;
  typedef typename ImageType::PixelType PixelType;
  typedef typename ImageType::IndexType IndexType;
  typedef typename ImageType::IndexValueType IndexValueType;

  /** Offset typedef support. An offset is used to access pixel values. */
  typedef typename ImageType::OffsetType OffsetType;

  /** Size typedef support. A size is used to define region bounds. */
  typedef typename ImageType::SizeType SizeType;
  typedef typename ImageType::SizeValueType SizeValueType;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
  /** Run-time type information (and related methods). */
  itkTypeMacro(ImageUtil, Object)

  static ImagePointer
  ReadImage(std::string filename);

  static ImagePointer
  Read3DImage(std::string filename);

  static ImagePointer
  ReadMGHImage(std::string filename);

  static std::vector< ImagePointer >
  ReadDICOMImage(std::string filename);

  static void
  WriteImage(std::string filename, const ImageType* image);

  static SizeType
  GetRadiusFromPhysicalSize(const ImageType* img, float r);

  static float
  GetPhysicalPixelSize(const ImageType* img);

  static
  unsigned int
  GetNumberOfPixels(const SizeType& sz);

  static ImagePointer
  GraftOutput(ImageSource<ImageType>* imgSource, size_t index = 0);

  static ImagePointer
  Erode(const ImageType* img, float r, float foreground = 1);

  static ImagePointer
  Dilate(const ImageType* img, float r, float foreground = 1);

  static ImagePointer
  Opening(const ImageType* img, float r, float foreground = 1);

  static ImagePointer
  Closing(const ImageType* img, float r, float foreground = 1);

  static void ImageExtent(const ImageType* img, PixelType& minValue,
                   PixelType& maxValue, double quantile = 0.01);

protected:
  ImageUtil()
  {
  }

  virtual
  ~ImageUtil()
  {
  }

private:
  ImageUtil(const Self &);          //purposely not implemented
  void
  operator=(const Self &);          //purposely not implemented
public:
    itkStaticConstMacro(ImageDimension, unsigned int, ImageType::ImageDimension);};

      }
// end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkImageUtil.hxx"
#endif

#endif
