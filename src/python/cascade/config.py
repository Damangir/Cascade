import os

ScriptDir=os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
InSource=os.path.basename(ScriptDir) == 'python'

if InSource:
    CascadeDir=os.path.dirname(os.path.dirname(ScriptDir))
    ExecDir=os.path.join(CascadeDir, 'build')
    DataDir=os.path.join(CascadeDir, 'data')
else:
    raise Exception ('Installed version not fully supported.')
    for p in os.environ["PATH"].split(os.pathsep) + [ScriptDir] :
        if os.path.exists(os.path.join(p, 'cascade-range')):
            ExecDir=p
    if not ExecDir:
        raise Exception("Can not locate Cascade binaries.")


HistDir=os.path.join(DataDir, 'histograms')
AtlasDir=os.path.join(DataDir, 'atlas')
StandardDir=os.path.join(DataDir, 'std')
MaskDir=os.path.join(DataDir, 'mask')
ReportDir=os.path.join(DataDir, 'report')
ConfigDir=os.path.join(DataDir, 'config')

StandardImg =os.path.join(StandardDir, 'MNI152_T1_2mm.nii.gz')
StandardBrain =os.path.join(StandardDir, 'MNI152_T1_2mm_brain.nii.gz')
StandardBrainMask =os.path.join(StandardDir, 'MNI152_T1_2mm_brain_mask.nii.gz')
StandardWM  =os.path.join(StandardDir, 'avg152T1_white.nii.gz')
StandardGM  =os.path.join(StandardDir, 'avg152T1_gray.nii.gz')
StandardCSF =os.path.join(StandardDir, 'avg152T1_csf.nii.gz')

if not os.path.exists(StandardImg):
    raise Exception('Can not find standard image.')

FNIRT1mm=os.path.join(ConfigDir, 'T1_2_MNI152_1mm.conf')
FNIRT2mm=os.path.join(ConfigDir, 'T1_2_MNI152_2mm.conf')