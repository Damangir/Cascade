#ifndef __itkStatisticalTestBase_h
#define __itkStatisticalTestBase_h

#include "itkObject.h"
#include "itkObjectFactory.h"

#include "itkMeasurementVectorTraits.h"
#include "itkSample.h"
#include "itkSubsample.h"

namespace itk
{
namespace Statistics
{
template< typename SampleT1, typename SampleT2=SampleT1, typename TRealValueType = double >
class StatisticalTestBase: public Object
{
public:
  /** Standard typedefs */
  typedef StatisticalTestBase Self;
  typedef Object Superclass;
  typedef SmartPointer< Self > Pointer;
  typedef SmartPointer< const Self > ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro(StatisticalTestBase, Object)
  ;

  typedef TRealValueType OutputType;

  typedef SampleT1 SampleType1;
  typedef SampleT2 SampleType2;

  virtual TRealValueType Evaluate(const SampleType1 * x1, const SampleType2 * x2) const = 0;

  itkGetConstMacro(TwoTail, bool);
  itkBooleanMacro (TwoTail);

  itkGetConstMacro(RightTail, bool);
  itkBooleanMacro (RightTail);

  itkGetConstMacro(LeftTail, bool);
  itkBooleanMacro (LeftTail);

  virtual void SetTwoTail(const bool _arg)
  {
    itkDebugMacro("setting OneTail to " << _arg);
    if (this->m_TwoTail != _arg)
    {
      this->m_TwoTail = _arg;
      this->Modified();
    }
    if (this->GetTwoTail())
    {
      this->SetLeftTail(false);
      this->SetRightTail(false);
    }
  }
  virtual void SetRightTail(const bool _arg)
  {
    itkDebugMacro("setting RightTail to " << _arg);
    if (this->m_RightTail != _arg)
    {
      this->m_RightTail = _arg;
      this->Modified();
    }
    if (this->GetRightTail())
    {
      this->SetLeftTail(false);
      this->SetTwoTail(false);
    }
  }
  virtual void SetLeftTail(const bool _arg)
  {
    itkDebugMacro("setting OneTail to " << _arg);
    if (this->m_LeftTail != _arg)
    {
      this->m_LeftTail = _arg;
      this->Modified();
    }
    if (this->GetLeftTail())
    {
      this->SetRightTail(false);
      this->SetTwoTail(false);
    }
  }

protected:
  StatisticalTestBase(){
    m_TwoTail = true;
    m_RightTail = false;
    m_LeftTail = false;
  }
  virtual ~StatisticalTestBase()
  {
  }
  void PrintSelf(std::ostream & os, Indent indent) const
  {
    Superclass::PrintSelf(os, indent);
    os << indent << "Two tail test: " << (m_TwoTail ? "Yes" : "No")
       << std::endl;
    os << indent << "Right tail test: " << (m_RightTail ? "Yes" : "No")
       << std::endl;
    os << indent << "Left tail test: " << (m_LeftTail ? "Yes" : "No")
       << std::endl;
  }


private:
  bool m_TwoTail;
  bool m_RightTail;
  bool m_LeftTail;
};
// end of class
}// end of namespace Statistics
} // end of namespace itk

#endif
