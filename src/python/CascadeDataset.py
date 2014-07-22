import os
import sys
import shutil
import string
from itertools import product

import ruffus

parser = ruffus.cmdline.get_argparse(description='Cascade academics - Dataset processing tool')

parser.add_argument('--source' , required=True , help='Subject subject root directory')
parser.add_argument('--target' , required=True , help='Target subject root directory')
parser.add_argument('--subject' , required=True , help='Pattern for subjects')

parser.add_argument('--t1' , required=True , help='T1 image pattern')
parser.add_argument('--flair' , help='FLAIR image pattern')
parser.add_argument('--pd' , help='PD image pattern')
parser.add_argument('--t2' , help='T2 image pattern')
parser.add_argument('--brain-mask', help='Brain mask pattern')

parser.add_argument('--brain-mask-space', choices=['T1', 'T2', 'FLAIR', 'PD'] , help='Brain mask space')
parser.add_argument('--calculation-space',required=True, choices=['T1', 'T2', 'FLAIR', 'PD'], help='Calculation space')

parser.add_argument('--model-dir', default='.', help='Directory where the model located')

parser.add_argument('--mode' , choices=['train', 'test', 'simple'] , help='Mode to run the pipeline')

parser.add_argument('--radius', default=1, help='Radius of local histogram')

parser.add_argument('--extra', default="", help='Extrapipeline control parameter to pass on')

options = parser.parse_args()

if options.brain_mask and not options.brain_mask_space:
    raise Exception('Option --brain-mask-space should be set.')

logger, logger_mutex = ruffus.cmdline.setup_logging (__name__, options.log_file, options.verbose)

import cascade

print cascade.Copyright()

###############################################################################
# Bring each sequence into the pipeline
###############################################################################
def allways_run(*args, **kwargs):
    return True, "Must run"

def originalImagesParam():
    extraInputs = options.extra.split()
    if options.mode == 'simple':
        extraInputs.append('--simple')
    seqs = filter(lambda x: getattr(options,x), ['t1', 't2', 'pd', 'flair'])
    for subjectDir in cascade.util.findDir(options.source, options.subject):
        subjectID = os.path.basename(subjectDir)
        inputArgs = {
                     'root' : os.path.join(options.target, subjectID),
                     'calculation_space':options.calculation_space,
                     'radius' : options.radius,
                     }
        if options.brain_mask and options.brain_mask_space:
            inputArgs['brain_mask_space'] = options.brain_mask_space,
            inputArgs['brain-mask']= next(cascade.util.findFile(subjectDir, options.brain_mask), None),

        expectedOutputs = []
        if options.mode == 'simple':
            expectedOutputs.append(os.path.join(inputArgs['root'], 'image', inputArgs['calculation_space'].upper(), 'model.free.wml.nii.gz'))
        elif options.mode == 'test':
            expectedOutputs.append(os.path.join(inputArgs['root'], 'image', inputArgs['calculation_space'].upper(), 'wml.pvalue.nii.gz'))
        for s in seqs:
            inputArgs[s] = next(cascade.util.findFile(subjectDir, getattr(options,s)), None)
            if options.mode == 'test':
                expectedOutputs.append(os.path.join(inputArgs['root'], 'image', inputArgs['calculation_space'].upper(), s.upper()+'.pvalue.nii.gz'))
            elif options.mode == 'train':
                expectedOutputs.append(os.path.join(inputArgs['root'], 'image', 'STD', s.upper()+'.feature.nii.gz'))
        
        if options.mode == 'test':
            inputArgs['model_dir'] = options.model_dir
        elif options.mode == 'train':
            expectedOutputs.append(os.path.join(inputArgs['root'], 'image', 'STD', 'brainTissueSegmentation.nii.gz'))

        yield [inputArgs, expectedOutputs, extraInputs]
        
@ruffus.files(originalImagesParam)
@ruffus.check_if_uptodate(allways_run)
def originalImages(input, output, extra=[]):
    cascade.util.ensureDirPresence(input['root'])
    tbl = string.maketrans('_', '-')
    args = [os.path.join(os.path.dirname(__file__), 'Cascade.py')]
    for arg, value in input.iteritems():
        if value: args.extend(['--' + arg.translate(tbl), str(value)])
    args.extend(extra)
    cascade.binary_proxy.run(sys.executable, args)

###############################################################################
# Bring each sequence into the pipeline
###############################################################################
def trainParam():
    if options.mode == 'test':
        return
    seqs = filter(lambda x: getattr(options,x), ['t1', 't2', 'pd', 'flair'])
    virtParam = {}
    outputs = []
    for subjectDir in cascade.util.findDir(options.source, options.subject):
        subjectID = os.path.basename(subjectDir)
        subjectRoot = os.path.join(options.target, subjectID, 'image', 'MNI')
        virtParam[subjectID] = {'root':subjectRoot}
        virtParam[subjectID]['bts'] = os.path.join(subjectRoot, 'brainTissueSegmentation.nii.gz')
        virtParam[subjectID]['hists'] = {}
        for s in seqs:
            virtParam[subjectID]['hists'][s] = os.path.join(subjectRoot, s.upper()+'.hist.nii.gz')

    for tissueName in cascade.CascadeFileManager.brainTissueNames.itervalues():
        for imgName in seqs:
            thisTraining = os.path.join(options.model_dir, '{}.{}.model.nii.gz'.format(imgName.upper(), tissueName.upper()))
            outputs.append(thisTraining)
            
    yield [virtParam, outputs]
        
        
#@ruffus.files(trainParam)
#@ruffus.follows(originalImages)
def train(input, output):
    filter(cascade.util.ensureAbsence, input)
    for subjID, params in input.iteritems():
        for tissueId, tissueName in cascade.CascadeFileManager.brainTissueNames.iteritems():
            thisTissueFile =  os.path.join(params['root'], tissueName+'.nii.gz')
            cascade.binary_proxy.fsl_run('fslmaths', [params['bts'], '-thr', tissueId, '-uthr', tissueId, thisTissueFile])
            for imgName, imgFile in params['hists'].iteritems():
                thisTraining = os.path.join(options.model_dir, '{}.{}.model.nii.gz'.format(imgName.upper(), tissueName.upper()))
                if os.path.exists(thisTraining):
                    addOrNot = ['-add', thisTraining]
                else:
                    addOrNot = []
                cascade.binary_proxy.cascade_run('vector-util', [imgFile, '-mask', thisTissueFile] + addOrNot + [thisTraining])

if __name__ == '__main__':
    ruffus.cmdline.run (options)
