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
#ifndef __itkMembershipImageFilter_h
#define __itkMembershipImageFilter_h

#include "itkUnaryFunctorImageFilter.h"

#include "itkVectorImage.h"
#include "itkVectorContainer.h"
namespace itk
{
namespace Functor
{
template< typename TInput, typename TOutput, typename TMembershipFunction >
class MembershipFunctor
{
public:
  typedef typename TMembershipFunction::Pointer MembershipFunctionPointer;
  typedef typename TMembershipFunction::MeasurementVectorType MeasurementVectorType;

  typedef VectorContainer< unsigned int, MembershipFunctionPointer > MembershipFunctionContainerType;
  typedef typename MembershipFunctionContainerType::Pointer MembershipFunctionContainerPointer;

  MembershipFunctor()
  {
    m_MembershipFunctions = MembershipFunctionContainerType::New();
    m_MembershipFunctions->Initialize(); // Clear elements
    m_NumberOfClasses = 0;
  }
  virtual ~MembershipFunctor()
  {
  }
  bool operator!=(const MembershipFunctor &) const
  {
    return false;
  }

  bool operator==(const MembershipFunctor & other) const
  {
    return !(*this != other);
  }

  inline TOutput operator()(const TInput & A) const
  {
    TOutput membershipPixel;
    NumericTraits< TOutput >::SetLength(membershipPixel, m_NumberOfClasses);

    MeasurementVectorType mv;
    NumericTraits< MeasurementVectorType >::SetLength(mv, NumericTraits< TInput >::GetLength(A));
    NumericTraits< TInput >::AssignToArray(A, mv);

    for (int i = 0; i < m_NumberOfClasses; i++)
    {
      membershipPixel[i] = (m_MembershipFunctions->GetElement(i))->Evaluate(mv);
    }
    return membershipPixel;
  }

  void AddMembershipFunction(TMembershipFunction * _arg)
  {
    m_MembershipFunctions->InsertElement(m_NumberOfClasses, _arg);
    m_NumberOfClasses = m_MembershipFunctions->Size();
  }
  void ClearMembershipFunctions()
  {
    m_MembershipFunctions->Initialize(); // Clear elements
    m_NumberOfClasses = 0;
  }

  itkGetConstMacro(NumberOfClasses, unsigned int);

private:
  MembershipFunctionContainerPointer m_MembershipFunctions;
  unsigned int m_NumberOfClasses;
};
}

/** \class MembershipImageFilter
 */
template< typename TInputImage, typename TMembershipFunction,
    typename TProbabilityPrecision = float,
    typename TOutputImage = VectorImage< TProbabilityPrecision,
        TInputImage::ImageDimension > >
class MembershipImageFilter: public UnaryFunctorImageFilter< TInputImage,
    TOutputImage,
    Functor::MembershipFunctor< typename TInputImage::PixelType,
        typename TOutputImage::PixelType, TMembershipFunction > >
{
public:
  /** Standard class typedefs. */
  typedef MembershipImageFilter Self;
  typedef UnaryFunctorImageFilter< TInputImage, TOutputImage,
      Functor::MembershipFunctor< typename TInputImage::PixelType,
          typename TOutputImage::PixelType, TMembershipFunction > > Superclass;

  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self)
  ;

  /** Runtime information support. */
  itkTypeMacro(MembershipImageFilter,
      UnaryFunctorImageFilter)
  ;

  typedef typename TInputImage::PixelType InputPixelType;
  typedef typename TOutputImage::PixelType OutputPixelType;

  void AddMembershipFunction(TMembershipFunction * _arg)
  {
    itkDebugMacro("adding " << _arg <<  " as membership function");
    this->GetFunctor().AddMembershipFunction(_arg);
    this->Modified();
  }
  void ClearMembershipFunctions()
  {
    this->GetFunctor().ClearMembershipFunctions();
    this->Modified();
  }

protected:
  virtual void GenerateOutputInformation()
  {
    this->Superclass::GenerateOutputInformation();
    TOutputImage *output = this->GetOutput();
    output->SetNumberOfComponentsPerPixel( this->GetFunctor().GetNumberOfClasses() );
  }

  MembershipImageFilter()
  {
  }
  virtual ~MembershipImageFilter()
  {
  }

private:
  MembershipImageFilter(const Self &); //purposely not implemented
  void operator=(const Self &); //purposely not implemented
};
} // end namespace itk

#endif
