#!/usr/bin/env python

import os
import sys
import shutil
import string
from itertools import product

import ruffus

parser = ruffus.cmdline.get_argparse(description='Cascade academics - Dataset processing tool')

parser.add_argument('--project' , required=True , help='Project root directory')
parser.add_argument('--subject' , required=True , help='Pattern for subjects')

parser.add_argument('--t1' , action='store_true' , help='Create model for T1')
parser.add_argument('--flair' , action='store_true', help='Create model for FLAIR images')
parser.add_argument('--pd' , action='store_true', help='Create model for PD images')
parser.add_argument('--t2' , action='store_true', help='Create model for T2 images')

parser.add_argument('--all-tissues' , action='store_true', help='Create model for CSF,GM and WM. The model for WM will be created.')

parser.add_argument('--model-dir', default='.', help='Directory for the output model')

options = parser.parse_args()

logger, logger_mutex = ruffus.cmdline.setup_logging (__name__, options.log_file, options.verbose)

import cascade

print cascade.Copyright()

inputImages=[]
for img in ['FLAIR', 'T1', 'T2', 'PD']:
    if getattr(options, img.lower()):
        inputImages.append(img)     

if options.all_tissues:
    neededTissues = cascade.CascadeFileManager.brainTissueNames.values()
else:
    neededTissues = ['WM']
###############################################################################
# Bring each sequence into the pipeline
###############################################################################
def allways_run(*args, **kwargs):
    return True, "Must run"

def ExtractTissueImagesParam():
    extent = {'WM' : [3, 3], 'GM' : [2, 2], 'CSF' : [1,1]}
    for subjectDir in cascade.util.findDir(options.project, options.subject):
        subjectID = os.path.basename(subjectDir)
        cascadeManager = cascade.CascadeFileManager(subjectDir)
        bts=cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', 'STD')
        for tissue in neededTissues:
            for img in inputImages:
                inImg =cascadeManager.imageInSpace(img + '.norm.nii.gz', 'STD')
                outImg = cascadeManager.imageInSpace(img + '.' + tissue+'.nii.gz', 'STD')
                yield [[bts, inImg], outImg, extent[tissue]]
        
@ruffus.files(ExtractTissueImagesParam)
def ExtractTissueImages(input, output, extent):
    cascade.binary_proxy.fsl_run('fslmaths', [input[0], '-thr', extent[0],'-uthr', extent[1], '-bin', '-mul', input[1], output], output)

def ModelTissueParam():
    for tissue in neededTissues:
        for img in inputImages:
            inputs = []
            modelName = os.path.join(options.model_dir, '.'.join([img, tissue]) + '.model.nii.gz')

            for subjectDir in cascade.util.findDir(options.project, options.subject):
                subjectID = os.path.basename(subjectDir)
                cascadeManager = cascade.CascadeFileManager(subjectDir)
                tissueImg = cascadeManager.imageInSpace(img + '.' + tissue+'.nii.gz', 'STD')
                inputs.append(tissueImg)
            yield [inputs, modelName]

@ruffus.follows(ExtractTissueImages)
@ruffus.files(ModelTissueParam)
def ModelTissue(input, output):
    cascade.binary_proxy.cascade_run('ComposeToVector', [output]+input, output)

if __name__ == '__main__':
    ruffus.cmdline.run (options)
