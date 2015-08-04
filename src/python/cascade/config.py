import os

ScriptDir=os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
CascadeDir=os.path.dirname(os.path.dirname(ScriptDir))

InSource= os.path.basename(os.path.dirname(ScriptDir)) == 'src'
if InSource:
    ExecDir=os.path.join(CascadeDir, 'build')
    DataDir=os.path.join(CascadeDir, 'data')
else:
    ExecDir=os.path.join(CascadeDir, 'bin')
    DataDir=os.path.join(CascadeDir, 'share', 'data')

StandardDir=os.path.join(DataDir, 'std')

StandardImg =os.path.join(StandardDir, 'MNI152_T1_2mm.nii.gz')
StandardBrain =os.path.join(StandardDir, 'MNI152_T1_2mm_brain.nii.gz')
StandardBrainMask =os.path.join(StandardDir, 'MNI152_T1_2mm_brain_mask.nii.gz')
StandardWM  =os.path.join(StandardDir, 'avg152T1_white.nii.gz')
StandardGM  =os.path.join(StandardDir, 'avg152T1_gray.nii.gz')
StandardCSF =os.path.join(StandardDir, 'avg152T1_csf.nii.gz')

if not os.path.exists(StandardImg):
    raise Exception('Can not find standard image.')

FreeSurfer_To_BrainTissueSegmentation=os.path.join(DataDir, 'map', 'FS_label.map.txt')
FreeSurfer_Label_Names=os.path.join(DataDir, 'map', 'FS_label_name.map.txt')
Unity_Transform=os.path.join(DataDir, 'transform', 'unity.tfm')

epsilon = 0.00001

Evident = {
          'T1': -0.90,
          'FLAIR': 0.5,
          'T2': 0.3,
          'PD': 0.1,
          }