/* Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved */

#ifndef __itkIterativeBayesianImageFilter_hxx
#define __itkIterativeBayesianImageFilter_hxx

#include "itkIterativeBayesianImageFilter.h"

namespace itk
{

template< class TImage, class TClassificationImage, class TProbabilityPrecision >
IterativeBayesianImageFilter< TImage, TClassificationImage,
    TProbabilityPrecision >::IterativeBayesianImageFilter()
{
  m_PriorBias = 0.0;
  m_NumberOfIterations = 1;
  m_MembershipFunctions = MembershipFunctionContainerType::New();
  m_MembershipFunctions->Initialize(); // Clear elements
  m_NumberOfClasses = 0;
}

template< class TImage, class TClassificationImage, class TProbabilityPrecision >
void IterativeBayesianImageFilter< TImage, TClassificationImage,
    TProbabilityPrecision >::GenerateData()
{
  itkAssertOrThrowMacro(
      m_NumberOfClasses == this->GetPriorVectorImage()->GetNumberOfComponentsPerPixel(),
      "Number of membership functions does not match.");

  const unsigned int nComp = this->GetInput()->GetNumberOfComponentsPerPixel();
  const unsigned int hSize = 100;
  typename WeightedHistogramType::HistogramSizeType size(nComp);
  size.Fill(hSize);

  typename MAPMarkovFilterType::Pointer MAPMarkovFilter =
      MAPMarkovFilterType::New();
  typename MembershipFilterType::Pointer membershipFilter =
      MembershipFilterType::New();

  membershipFilter->SetInput(this->GetInput());
  MAPMarkovFilter->SetPriorVectorImage(this->GetPriorVectorImage());
  MAPMarkovFilter->SetPriorBias(this->GetPriorBias());
  MAPMarkovFilter->SetNumberOfIterations(1);

  typename MaskPriorFilter::Pointer maskWeights = MaskPriorFilter::New();
  typename ClassSelectorFilter::Pointer classSelector = ClassSelectorFilter::New();
  typename PriorSelectorImageFilterType::Pointer classPriorSelector =
      PriorSelectorImageFilterType::New();
  classPriorSelector->SetInput(this->GetPriorVectorImage());

  membershipFilter->ClearMembershipFunctions();
  for (unsigned int i = 0; i < m_NumberOfClasses; ++i)
  {
    membershipFilter->AddMembershipFunction(
        m_MembershipFunctions->GetElement(i));
  }
  for (unsigned int iteration = 0; iteration < m_NumberOfIterations; iteration++)
  {
    MAPMarkovFilter->SetMembershipVectorImage(membershipFilter->GetOutput());
    MAPMarkovFilter->Update();
    typename ClassifierOutputImageType::Pointer lastSeg = MAPMarkovFilter->GetOutput();
    lastSeg->DisconnectPipeline();
    membershipFilter->ClearMembershipFunctions();
    classSelector->SetInput1(lastSeg);

    for (ClassificationPixelType i = 0; i < m_NumberOfClasses; ++i)
    {
      typename WeightedHistogramType::Pointer hist =
          WeightedHistogramType::New();
      typename EmpiricalDistributionMembershipType::Pointer membership =
          EmpiricalDistributionMembershipType::New();

      classPriorSelector->SetIndex(i);
      classSelector->SetConstant2(i);
      maskWeights->SetInput(classSelector->GetOutput());
      maskWeights->SetMaskImage(classPriorSelector->GetOutput());
      maskWeights->Update();
      hist->SetWeightImage(maskWeights->GetOutput());
      hist->SetInput(this->GetInput());
      hist->SetAutoMinimumMaximum(true);
      hist->SetHistogramSize(size);
      hist->Update();
      membership->SetDistribution(hist->GetOutput());
      membershipFilter->AddMembershipFunction(membership);
    }
    /*
     * Calculate the difference between previous and current distribution
     */
  }
  MAPMarkovFilter->SetMembershipVectorImage(membershipFilter->GetOutput());
  MAPMarkovFilter->Update();
  this->GraftOutput(MAPMarkovFilter->GetOutput());
}

template< class TImage, class TClassificationImage, class TProbabilityPrecision >
void IterativeBayesianImageFilter< TImage, TClassificationImage,
    TProbabilityPrecision >::PrintSelf(std::ostream& os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Iterations: " << m_NumberOfIterations << std::endl;
  os << indent << "Prior bias: " << m_PriorBias << std::endl;
  os << indent << "Number of classes: " << m_NumberOfClasses << std::endl;
}

} // end namespace itk

#endif
