/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */
#ifndef __itkVotingRelabelImageFilter_h
#define __itkVotingRelabelImageFilter_h

#include "itkVotingBinaryImageFilter.h"

namespace itk
{
/** \class VotingRelabelImageFilter
 * \brief Relabel image by a voting operation in a neighborhood of each pixel.
 *
 * At each voxel number of voxels equal to foreground, background and others are
 * counted.
 *  # Voxel is relabeled to BirthValue if number of foreground voxels is
 *  greater or equal to BirthThreshold percent of the rest of voxels
 *  # Voxel is relabeled to UnsurvivedValue if number of other voxels is
 *  greater or equal to SurvivalThreshold percent of total background or
 *  foreground voxels
 *
 * Please note: All thresholds are in percentage.
 */
template< typename TInputImage, typename TOutputImage >
class VotingRelabelImageFilter: public VotingBinaryImageFilter< TInputImage,
    TOutputImage >
{
public:
  /** Extract dimension from input and output image. */
    itkStaticConstMacro(InputImageDimension, unsigned int,
        TInputImage::ImageDimension);
      itkStaticConstMacro(OutputImageDimension, unsigned int,
          TOutputImage::ImageDimension);

      /** Convenient typedefs for simplifying declarations. */
      typedef TInputImage InputImageType;
      typedef TOutputImage OutputImageType;

      /** Standard class typedefs. */
      typedef VotingRelabelImageFilter Self;
      typedef VotingBinaryImageFilter< InputImageType, OutputImageType > Superclass;
      typedef SmartPointer< Self > Pointer;
      typedef SmartPointer< const Self > ConstPointer;

      /** Method for creation through the object factory. */
      itkNewMacro(Self);

      /** Run-time type information (and related methods). */
      itkTypeMacro(VotingRelabelImageFilter, VotingBinaryImageFilter);

      /** Image typedef support. */
      typedef typename InputImageType::PixelType InputPixelType;
      typedef typename OutputImageType::PixelType OutputPixelType;

      typedef typename InputImageType::RegionType InputImageRegionType;
      typedef typename OutputImageType::RegionType OutputImageRegionType;

      typedef typename InputImageType::SizeType InputSizeType;
      typedef typename InputImageType::SizeValueType SizeValueType;

      /** Returns the number of pixels that changed when the filter was executed. */
      itkGetConstReferenceMacro(NumberOfPixelsChanged, SizeValueType);

      itkGetConstReferenceMacro(BirthValue, InputPixelType);
      itkSetMacro(BirthValue, InputPixelType);

      itkGetConstReferenceMacro(UnsurvivedValue, InputPixelType);
      itkSetMacro(UnsurvivedValue, InputPixelType);

#ifdef ITK_USE_CONCEPT_CHECKING
    // Begin concept checking
    itkConceptMacro( IntConvertibleToInputCheck,
        ( Concept::Convertible< int, InputPixelType > ) );
    itkConceptMacro( UnsignedIntConvertibleToInputCheck,
        ( Concept::Convertible< unsigned int, InputPixelType > ) );
    // End concept checking
#endif

  protected:
    VotingRelabelImageFilter();
    virtual ~VotingRelabelImageFilter()
    {}
    void PrintSelf(std::ostream & os, Indent indent) const;

    void ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread,
        ThreadIdType threadId);

    void BeforeThreadedGenerateData();

    void AfterThreadedGenerateData();

  private:
    VotingRelabelImageFilter(const Self &); //purposely not implemented
    void operator=(const Self &);//purposely not implemented

    SizeValueType m_NumberOfPixelsChanged;

    // Auxiliary array for multi-threading
    Array< SizeValueType > m_Count;

    InputPixelType m_BirthValue;
    InputPixelType m_UnsurvivedValue;

  };
}
// end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkVotingRelabelImageFilter.hxx"
#endif

#endif
