#!/usr/bin/env python
import sys
import os
import shutil
import argparse
from itertools import product
import ruffus
import time

import util.checkpython
import util.terminalsize
from warnings import catch_warnings

runtimeString = time.strftime("%Y%m%d%H%M%S")

# Fix the envirnment variable for width. It is not corectly set on all platforms
os.environ['COLUMNS'] = str(util.terminalsize.get_terminal_size()[0])
defaultStr = ' (default: %(default)s)'

parser = ruffus.cmdline.get_argparse(description='Cascade academics: Segmentation of white matter hyperintensities.',
                                     version = 'Cascade academics v. 1.1',
                                     ignored_args = ["key_legend_in_graph", "draw_graph_horizontally", 
                                                     "flowchart_format", "checksum_file_name", "recreate_database",
                                                     "use_threads"])

# Output
outputOptions = parser.add_argument_group('Output options')
outputOptions.add_argument('-r', '--root', required=True,
                    help='Image root directory')

# Input files
inputOptions = parser.add_argument_group('Input files')
inputOptions.add_argument('-t', '--t1' , required=True, metavar='T1.nii.gz',
                    help='T1 image. (required)')

inputOptions.add_argument('-f', '--flair', metavar='FLAIR.nii.gz',
                    help='FLAIR image')
inputOptions.add_argument('-p', '--pd',metavar='PD.nii.gz',
                    help='PD image')
inputOptions.add_argument('-s', '--t2',metavar='T2.nii.gz',
                    help='T2 image')

generalOptions = parser.add_argument_group('General Options')
generalOptions.add_argument('--radius', default=1, type=float,
                    help='Radius of local histogram in millimeter.'+defaultStr)
generalOptions.add_argument('-c', '--calculation-space',
                    choices=['T1', 'T2', 'FLAIR', 'PD'], default='T1',
                    help='Calculation space'+defaultStr)
generalOptions.add_argument('--already-registered' , action='store_true',
                    help='Skip registration assume all sequences are preregistered.')
generalOptions.add_argument('--evident' , action='store_true',
                    help='Trim evident normal tissues first.')

# importing Options
importOptions = parser.add_argument_group('Import processes', 
                                          'Import from maximum one source is allowed.')
importOptions.add_argument('--brain-mask',metavar=('brainMask.nii.gz', '{T1,T2,FLAIR,PD}'),
                           help='Brain mask and the space it is located. '
                           'All area with value greater than zero will be considered as brain.',
                           nargs=2)

importOptions.add_argument('--freesurfer', metavar='FS_SUBJECT',
                           help='Import freesurfer from the directory. You should '
                           'point to the subject directory i.e. $SUBJECTS_DIR/SubjectID.'
                           'mri directory is expected to be present in this directory.')


modeOptions = parser.add_argument_group('Pipeline procedure control',
                                        'Only one of these options are allowed. '
                                        'For training do not use any.')

modeOptions.add_argument('--simple' , action='store_true',
                    help='Run the pipeline in simple mode i.e. without trained model.')

modeOptions.add_argument('-d', '--model-dir',
                    help='Directory where the trained model located. This option'
                    'implies runing in train/test model and presume you have a '
                    'trained model stored in a directory.')
modeOptions.add_argument('--all-tissues' , action='store_true', help='Create model for CSF,GM and WM. The model for WM will be created.')

reportOptions = parser.add_argument_group('Reporting controls')
reportOptions.add_argument('--report-threshold', default=0.5, type=float,
                    help='Probability threshold used in the report (0-1).'+defaultStr)
reportOptions.add_argument('--report-radius', default=0, type=float,
                    help='Minimum lesion radius to report (in mm).'+defaultStr)
reportOptions.add_argument('--report-size', default=0, type=float,
                    help='Minimum lesion size to report (in mm^3).'+defaultStr)

options = parser.parse_args()
logger, logger_mutex = ruffus.cmdline.setup_logging (__name__, options.log_file, options.verbose)
os.environ['CASCADE_VERBOSE'] =  str(options.verbose)
import cascade
cascade.logger, cascade.logger_mutex = ruffus.cmdline.setup_logging ('cascade', options.log_file, options.verbose)

if not any([options.t2,options.flair,options.pd]):
    parser.error('At least one of FLAIR, T2 or PD should be specified.')

if len(filter(None, [options.freesurfer,options.brain_mask])) > 1:
    parser.error('You can import only from one source.')

if len(filter(None, [options.simple,options.model_dir])) > 1:
    parser.error('You should either use simple mode or test/train mode.')


if options.simple:
    trainMode = False
    testMode = False
else:
    trainMode = options.model_dir is None
    testMode = not trainMode

#logger, logger_mutex = ruffus.cmdline.setup_logging (__name__, options.log_file, options.verbose)

imageTypes = {1:'T1', 2:'FLAIR', 3:'T2', 4:'PD'}

inputImages = {'T1':options.t1,
               'T2':options.t2,
               'PD':options.pd,
               'FLAIR':options.flair
               }
testDir = {'FLAIR': 'pos', 'T1':'neg', 'T2':'pos', 'PD':'pos'}

inputImages = dict(filter(lambda x: x[1] is not None, inputImages.iteritems()))

for imgName, imgFile in inputImages.iteritems():
    if not os.path.exists(imgFile):
        raise Exception('{} file does not exist: {}'.format(imgName, imgFile))
    else:
        print '{} file exists: {}'.format(imgName, imgFile)

if options.calculation_space in inputImages:    
    calculationSpace = options.calculation_space
else:
    calculationSpace = inputImages.keys()[0]

calculationBase = inputImages[calculationSpace]

cascadeManager = cascade.CascadeFileManager(options.root)
cascadeManager.calcSpace = calculationSpace


do_BrainExtract = True
do_WMEstimate = True
do_BTS = True
has_atlas = False

if options.all_tissues:
    neededTissues = cascadeManager.brainTissueNames.values()
else:
    neededTissues = ['WM']
    
if testMode:
    for i in product(inputImages.keys(), neededTissues):
        modelName = os.path.join(options.model_dir, '.'.join(i) + '.model.nii.gz')
        print modelName
        if not os.path.exists(modelName):
            raise Exception('No model found at {}'.format(modelName))

print cascade.Copyright()

###############################################################################
# Bring each sequence into the pipeline
###############################################################################
def originalImagesParam():
    for imageName, image in inputImages.iteritems():
        params = [image, cascadeManager.imageInSpace(imageName + '.nii.gz', imageName)]
        yield params
               
#@ruffus.graphviz(label='Original Images')
@ruffus.files(originalImagesParam)
def originalImages(input, output):
    cascade.binary_proxy.fsl_run('fslchfiletype', ['NIFTI_GZ', input, output])
    cascade.binary_proxy.fsl_run('fslreorient2std', [output, output])

###############################################################################
# Bring the base image into the pipeline
###############################################################################
@ruffus.originate(cascadeManager.imageInSpace(cascadeManager.calcSpace + '.nii.gz', cascadeManager.calcSpace))
def baseImage(output_file):
    pass

###############################################################################
# Intra subject registration of all sequences
###############################################################################    
@ruffus.combinatorics.product(originalImages,
                              ruffus.formatter(),
                              baseImage,
                              ruffus.formatter(),
                              (
                               cascadeManager.imageInSpace('{basename[0][0]}{ext[0][0]}', cascadeManager.calcSpace),
                               cascadeManager.transITKName('{basename[0][0]}', '{basename[1][0]}'),
                               cascadeManager.transITKName('{basename[1][0]}', '{basename[0][0]}'),
                               ),
                              cascadeManager)
def interaRegistration(input, output, manager):
    movingImage = input[0]
    fixedImage = input[1]
    movedImage = output[0]
    transferFile = output[1]
    invTransferFile = output[2]
    if options.already_registered:
        shutil.copy(cascade.config.Unity_Transform, transferFile)
        shutil.copy(cascade.config.Unity_Transform, invTransferFile)
    else:
        cascade.binary_proxy.cascade_run('linRegister', [fixedImage, movingImage, transferFile, invTransferFile, 'intra'])
        
    if movedImage != movingImage:
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile])

###############################################################################
# Linear registration to STD space
###############################################################################
def linStdParam():
    imgForStdReg = filter(lambda x:x in inputImages, ['T1', 'T2', 'PD'])[0]
    params = [
              (
               cascade.config.StandardImg,
               cascadeManager.imageInSpace(imgForStdReg + '.nii.gz', cascadeManager.calcSpace),
              ),
              (
               cascadeManager.transITKName('std', cascadeManager.calcSpace),
               cascadeManager.transITKName(cascadeManager.calcSpace, 'std'),
              ),
              cascadeManager
              ]
    yield params

@ruffus.follows(interaRegistration)        
@ruffus.files(linStdParam)
def linearRegistrationToStandard(input, output, manager):
    movingImage = input[0]
    fixedImage = input[1]
    transferFile = output[0]
    invTransferFile = output[1]
    cascade.binary_proxy.cascade_run('linRegister', [fixedImage, movingImage, transferFile, invTransferFile])

###############################################################################
# Resample STD space to native
###############################################################################
def resampleStdParam():
    imgForStdReg = filter(lambda x:x in inputImages, ['T1', 'T2', 'PD'])[0]
    for stdImage in [cascade.config.StandardBrainMask, cascade.config.StandardWM, cascade.config.StandardGM, cascade.config.StandardCSF]:
        params = [
                  (
                   stdImage,
                   cascadeManager.imageInSpace(imgForStdReg + '.nii.gz', cascadeManager.calcSpace),
                   cascadeManager.transITKName('std', cascadeManager.calcSpace),
                  ),
                  (
                   cascadeManager.imageInSpace(os.path.basename(stdImage), cascadeManager.calcSpace),
                  ),
                  cascadeManager
                  ]
        yield params

@ruffus.follows(linearRegistrationToStandard)        
@ruffus.files(resampleStdParam)
def resampleStdToNative(input, output, manager):
    movingImage = input[0]
    fixedImage = input[1]
    transferFile = input[2]
    movedImage = output[0]
    if 'mask' in os.path.basename(movingImage).lower():
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)
    else:
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile], movedImage)
       
###############################################################################        
# Import from Freesurfer
###############################################################################        
if options.freesurfer:
    do_BTS = False
    do_BrainExtract = False
    do_WMEstimate = False
    has_atlas = True
    
    def fsImport():
        inImages = [
                    os.path.join(options.freesurfer, 'mri', 'rawavg.mgz'),
                    os.path.join(options.freesurfer, 'mri', 'aseg.mgz'),
                    os.path.join(options.freesurfer, 'mri', 'wmparc.mgz'),
                   ]
        param = [cascadeManager,
                 'nearest'
                  ]
        outImages = [
                    cascadeManager.imageInSpace('aseg.nii.gz', 'T1'),
                    cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', 'T1'),
                    cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('wmparc.nii.gz', 'T1'),
                    cascadeManager.imageInSpace('atlas.nii.gz', cascadeManager.calcSpace),
                    ]
       
        params = [
                  inImages,
                  outImages,
                  param
                  ]
        yield params
    @ruffus.files(fsImport)      
    @ruffus.follows(interaRegistration)
    def ImportFS(input, output, param):
        #Import aseg as brainTissueSegmentation
        cascade.binary_proxy.run('mri_convert', ['-rt', param[1], '-rl', input[0], input[1], output[0]], output[0])
        cascade.binary_proxy.cascade_run('relabel', [output[0], cascade.config.FreeSurfer_To_BrainTissueSegmentation, output[1]], output[1])
        manager = param[0]
        movingImage = output[1]
        movedImage = output[2]
        fixedImage = manager.imageInSpace(manager.calcSpace + '.nii.gz', manager.calcSpace)
        transferFile = manager.transITKName(manager.getImageSpace(movingImage), manager.getImageSpace(fixedImage))
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)
        #Import wmparc as atlas
        cascade.binary_proxy.run('mri_convert', [input[2], output[3]], output[3])
        movingImage = output[3]
        movedImage = output[4]
        fixedImage = manager.imageInSpace(manager.calcSpace + '.nii.gz', manager.calcSpace)
        transferFile = manager.transITKName(manager.getImageSpace(movingImage), manager.getImageSpace(fixedImage))
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)

    @ruffus.transform(ImportFS, ruffus.formatter(),
                      cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                      cascadeManager)
    def brainExtraction(input, output, manager):
        cascade.binary_proxy.fsl_run('fslmaths', [input[2], '-mas', output])

    @ruffus.transform(ImportFS, ruffus.formatter(),
                      cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                      cascadeManager)
    def brainTissueSegmentation(input, output, param):
        pass

    @ruffus.transform(ImportFS, ruffus.formatter(),
                      cascadeManager.imageInSpace('norm.mask.nii.gz', cascadeManager.calcSpace),
                      cascadeManager)
    def normalizationMask(input, output, param):
        cascade.binary_proxy.fsl_run('fslmaths', [input[2], '-thr', 3, output])


###############################################################################
# Import the brain mask
###############################################################################
if do_BrainExtract and options.brain_mask:
    do_BrainExtract = False
    ###############################################################################
    # Bring the brain mask into the pipeline
    ###############################################################################
    @ruffus.transform(options.brain_mask[0], ruffus.regex('.*'), cascadeManager.imageInSpace('brain_mask.nii.gz', options.brain_mask[1]))
    def brainMask(input, output):
        cascade.binary_proxy.fsl_run('fslchfiletype', ['NIFTI_GZ', input, output])
        cascade.binary_proxy.fsl_run('fslreorient2std', [output, output])

    ###############################################################################
    # Register brain mask to images
    ###############################################################################
    @ruffus.follows(interaRegistration)
    @ruffus.transform(brainMask, ruffus.formatter(),
                      cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                      cascadeManager)
    def brainMaskRegistration(input, output, manager):
        movingImage = input
        movedImage = output
        fixedImage = manager.imageInSpace(manager.calcSpace + '.nii.gz', manager.calcSpace)
        transferFile = manager.transITKName(manager.getImageSpace(input), manager.getImageSpace(fixedImage))
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)

    @ruffus.transform(brainMaskRegistration, ruffus.formatter(),
                      cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                      cascadeManager)
    def brainExtraction(input, output, manager):
        pass

if do_BrainExtract:
    def beParam():
        imgForBex = filter(lambda x:x in inputImages, ['PD', 'T1', 'FLAIR', 'T2'])[0]
        inImages = [cascadeManager.imageInSpace(imgForBex + '.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace(os.path.basename(cascade.config.StandardBrainMask), cascadeManager.calcSpace),
                   ]
        beParams = [
                     ]
        outImages = [
                    cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                    ]
       
        params = [
                  inImages,
                  outImages,
                  beParams
                  ]
        yield params
    @ruffus.follows(resampleStdToNative)        
    @ruffus.follows(interaRegistration)
    @ruffus.files(beParam)      
    def brainExtraction(input, output, param):
        cascade.binary_proxy.cascade_run('brainExtraction', input + output + param, output)


if do_WMEstimate:
    def nmParam():
        imgForNM = filter(lambda x:x in inputImages, ['T1', 'T2', 'PD', 'FLAIR'])[0]
        inImages = [cascadeManager.imageInSpace(imgForNM + '.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace), #CSF
                    cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace), #GM
                    cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace), #WM
                   ]
        outImages = cascadeManager.imageInSpace('norm.mask.nii.gz', cascadeManager.calcSpace)
                    
       
        params = [
                  inImages,
                  outImages,
                  cascadeManager
                  ]
        yield params
    @ruffus.follows(interaRegistration)        
    @ruffus.follows(brainExtraction)
    @ruffus.files(nmParam)      
    def normalizationMask(input, output, manager):
        cascade.binary_proxy.cascade_run('TissueTypeSegmentation', [input[0], input[1], output, input[2], input[3], input[4]], output)
        map_file = manager.getTempFilename('tts2norm_mask')
        with open(map_file, 'w') as f:
            f.write("0    0\n1    0\n2    0\n3    1");
        
        cascade.binary_proxy.cascade_run('relabel', [output, map_file, output], output)
    

###############################################################################
# Normalize image
###############################################################################
@ruffus.combinatorics.product(interaRegistration,
                              ruffus.formatter(),
                              normalizationMask,
                              ruffus.formatter(),
                              cascadeManager.imageInSpace('{basename[0][0]}.norm{ext[0][0]}', cascadeManager.calcSpace),
                              cascadeManager)
def normalize(input, output, manager):
    cascade.binary_proxy.cascade_run('inhomogeneity', [input[0][0], input[1], output], output)


if do_BTS:
    ###############################################################################
    # BTS segmentation
    ###############################################################################
    def btsParam():
        imgForNM = filter(lambda x:x in inputImages, ['T1', 'T2', 'PD', 'FLAIR'])[0]
        input = [cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace)]

        param = {
                 'out' : cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                 'intermediate' : cascadeManager.imageInSpace('', cascadeManager.calcSpace),
                 'brain': cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                 }
        for s in inputImages:
            param[s.lower()] = cascadeManager.imageInSpace( s + '.norm.nii.gz', cascadeManager.calcSpace)
            input.append(cascadeManager.imageInSpace( s + '.norm.nii.gz', cascadeManager.calcSpace))

        yield [input, param['out'], param]
           
    @ruffus.follows(brainExtraction)        
    @ruffus.follows(normalize)        
    @ruffus.files(btsParam)
    def brainTissueSegmentation(input, output, param):
        args = [os.path.join(os.path.dirname(__file__), 'CascadeTTS.py')]
        for arg, value in param.iteritems():
            if value: args.extend(['--' + arg, str(value)])
        cascade.binary_proxy.run(sys.executable, args)


if options.evident:
    def evidentParam():
        def getEvidentParam(mod):
            return  [
                     [
                         cascadeManager.imageInSpace(mod.upper() + '.norm.nii.gz'),
                         [cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz'),
                         cascade.config.Evident,
                         3]
                     ],
                     cascadeManager.imageInSpace(mod.upper() + '.evident.nii.gz'),
                     cascadeManager
                    ]

        if 'FLAIR' in inputImages:                      
            yield getEvidentParam('FLAIR')
        elif 'T2' in inputImages:
            yield getEvidentParam('T2')
        else:
            yield getEvidentParam('PD')
        
        yield getEvidentParam('T1')

        
    @ruffus.follows(normalize)        
    @ruffus.follows(brainTissueSegmentation)
    @ruffus.files(evidentParam)
    def trimEvident(input, output, manager):
        cascade.binary_proxy.cascade_run('EvidentNormal', [input[0],     # Image
                                                     input[1][0],  # Brain tissue segmentation
                                                     output,       # Output
                                                     # Percentile for the input image
                                                     input[1][1][manager.getImageType(input[0])],
                                                     input[1][2]], output)
    
    @ruffus.merge([trimEvident, brainTissueSegmentation],
                  cascadeManager.imageInSpace('possible.wml.nii.gz', cascadeManager.calcSpace)
                      )
    def possibleArea(input, output):
        btsOutput = input[-1]
        input = input[:-1]
        cascade.binary_proxy.fsl_run('fslmaths', [input[0] ,'-bin', output])
        for i in input[1:]:
            cascade.binary_proxy.fsl_run('fslmaths', [i, '-add', output ,'-bin', output])
    
        cascade.binary_proxy.fsl_run('fslmaths', [output, '-mul' , -1, '-add', 1, output])
        cascade.binary_proxy.fsl_run('fslmaths', [btsOutput, '-add', 1,'-thr', 4, '-bin', '-mul', output, output])
else:
    @ruffus.merge(brainTissueSegmentation,
                  cascadeManager.imageInSpace('possible.wml.nii.gz', cascadeManager.calcSpace)
                      )
    def possibleArea(input, output):
        btsOutput = input[0]
        cascade.binary_proxy.fsl_run('fslmaths', [btsOutput, '-add', 1,'-thr', 4, '-bin', output])
        
###############################################################################
# mark evidently normal brain
###############################################################################
def modelFreeParam():
    testDir = {'FLAIR': 'pos', 'T1':'neg', 'T2':'pos', 'PD':'pos'}
    for img in inputImages.keys():
        inImages = [cascadeManager.imageInSpace(img + '.norm.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                    options.radius, # Radius for histogram creation in mm
                    cascadeManager.imageInSpace('possible.wml.nii.gz', cascadeManager.calcSpace)
                    ]
    
        outImages = cascadeManager.imageInSpace(img+'.model.free.nii.gz', cascadeManager.calcSpace)

       
        params = [
                  inImages,
                  outImages,
                  [cascadeManager,testDir[img]]
                  ]
        yield params

@ruffus.follows(possibleArea)
@ruffus.follows(brainTissueSegmentation)
@ruffus.follows(normalize)
@ruffus.files(modelFreeParam)
def modelFreeSegmentation(input, output, extra):
    wm = extra[0].getTempFilename('WM.nii.gz')
    cascade.binary_proxy.fsl_run('fslmaths', [input[1], '-thr',3, '-bin', wm])
    cascade.binary_proxy.cascade_run('OneSampleKolmogorovSmirnovTest', [wm,wm, input[0], output, input[2], extra[1]] , output)

###############################################################################
# This is model free segmentation. Fine results for volume estimation.
###############################################################################
if options.simple:
    @ruffus.merge(modelFreeSegmentation, cascadeManager.imageInSpace('model.free.wml.nii.gz', cascadeManager.calcSpace))
    def summarize(infiles, summary_file):
        print infiles
        print summary_file

    if not options.target_tasks:
        options.target_tasks = ['summarize']
    
 
###############################################################################
# This part onward is for normal run which contain modeling of normal brain
# and segmentation using predefined model
###############################################################################
   
if trainMode:
###############################################################################
# Warp all normalized images to STD space
###############################################################################
    @ruffus.combinatorics.product(brainTissueSegmentation,
                                  ruffus.formatter(),
                                  linearRegistrationToStandard,
                                  ruffus.formatter(),
                                  '{subpath[0][0][1]}/STD/{basename[0][0]}{ext[0][0]}',
                                  cascadeManager)
    def resampleBTSToStandard(input, output, manager):
        fixedImage = cascade.config.StandardBrainMask
        transferFile = input[1][1]
        movingImage = input[0]
        movedImage = output
        cascade.util.ensureDirPresence(output)
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)

    @ruffus.combinatorics.product(normalize,
                                  ruffus.formatter(),
                                  linearRegistrationToStandard,
                                  ruffus.formatter(),
                                  '{subpath[0][0][1]}/STD/{basename[0][0]}{ext[0][0]}',
                                  cascadeManager)
    def resampleToStandard(input, output, manager):
        fixedImage = cascade.config.StandardBrainMask
        transferFile = input[1][1]
        movingImage = input[0]
        movedImage = output
        cascade.util.ensureDirPresence(output)
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile], movedImage)

if testMode:
###############################################################################
# Register normal model to the calculation space
###############################################################################
    def registerModelParam():
        modelPrefix = ''
        for i in product(inputImages.keys(), neededTissues):
            modelName = '.'.join(i) + '.model.nii.gz'
            params = [
                      (
                       os.path.join(options.model_dir, modelName),
                       cascadeManager.imageInSpace(cascadeManager.calcSpace + '.nii.gz', cascadeManager.calcSpace),
                       cascadeManager.transITKName('std', cascadeManager.calcSpace),
                       ),
                      cascadeManager.imageInSpace(modelName, os.path.join(cascadeManager.calcSpace, 'model'))
                      ]
            yield params

    @ruffus.follows(linearRegistrationToStandard)
    @ruffus.files(registerModelParam)
    def registerModel(input, output):
        fixedImage = input[1]
        transferFile = input[2]
        movingImage = input[0]
        movedImage = output
        cascade.util.ensureDirPresence(output)
        cascade.binary_proxy.cascade_run('resampleVector', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)

###############################################################################
# Kolmogorov Smirnov to find p-value of each image with respect to model
# " Reference TestArea Input Output Radius [pos/neg]"
###############################################################################
    def KolmogorovSmirnovParam():
        modelPrefix = ''
        for i in product(inputImages.keys(), neededTissues):
            modelName = '.'.join(i) + '.model.nii.gz'
            inImages = [
                      cascadeManager.imageInSpace(modelName, os.path.join(cascadeManager.calcSpace, 'model')), # Reference
                      cascadeManager.imageInSpace(i[0] + '.norm.nii.gz', cascadeManager.calcSpace),
                      cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                      options.radius, # Radius for histogram creation in mm
                      ]
            outImages = cascadeManager.imageInSpace(i[0]+'.KS.nii.gz', cascadeManager.calcSpace)
            
            params = [
                  inImages,
                  outImages,
                  [cascadeManager,testDir[i[0]]]
                  ]
            yield params
            
    @ruffus.follows(registerModel)
    @ruffus.files(KolmogorovSmirnovParam)
    def KolmogorovSmirnov(input, output, extra):
        wm = extra[0].getTempFilename('WM.nii.gz')
        cascade.binary_proxy.fsl_run('fslmaths', [input[2], '-thr',3, '-bin', wm])
        cascade.binary_proxy.cascade_run('TwoSampleKolmogorovSmirnovTest', [input[0],wm, input[1], output[0], input[3], extra[1]] , output)
    
    @ruffus.merge(KolmogorovSmirnov, cascadeManager.imageInSpace('pvalue.wml.nii.gz', cascadeManager.calcSpace))
    def summarize(infiles, summary_file):
        print infiles
        print summary_file


if has_atlas and not trainMode:
    options.target_tasks = ['report']
    def reportParam():
        if options.simple:
            finalOutput = cascadeManager.imageInSpace('model.free.wml.nii.gz', cascadeManager.calcSpace)
        else:
            finalOutput = cascadeManager.imageInSpace('pvalue.wml.nii.gz', cascadeManager.calcSpace)

        inImages = [finalOutput,
                    cascadeManager.imageInSpace('atlas.nii.gz', cascadeManager.calcSpace),
                    float(options.report_threshold),
                    cascade.config.FreeSurfer_Label_Names
                    ]
        outImages = [
                    cascadeManager.reportName('atlas-report.csv')
                    ]
        params = [
                  inImages,
                  outImages,
                  ]
        yield params
    @ruffus.follows(summarize)
    @ruffus.files(reportParam)    
    def report(input, output):
        reportTxt = "#"*80
        reportTxt += "\n# ".join(cascade.Copyright().splitlines()) + "\n"
        reportTxt += "# Working Directory\n"
        reportTxt += "# " + os.getcwd() + "\n"
        reportTxt += "# Executed Command\n"
        reportTxt += "# " + " ".join(sys.argv) + "\n"
        reportTxt += "#"*80 + "\n"        
        reportTxt += cascade.binary_proxy.cascade_check_output('atlas', input)
        with open(output[0], "w") as text_file:
            text_file.write(reportTxt)    


if __name__ == '__main__':
    if not any([options.flowchart, options.just_print]):
        try:
            ruffus.pipeline_printout_graph (open(cascadeManager.reportName("chart.svg", runtimeString), "w"),
                                            "svg",
                                            options.target_tasks,
                                            options.forced_tasks,
                                            draw_vertically = True,
                                            no_key_legend   = True,
                                            minimal_key_legend = True,
                                            pipeline_name = 'Cascade pipeline')
            with open(cascadeManager.reportName("args.log", runtimeString), "w") as text_file:
                for p in vars(options).iteritems():
                    text_file.write("{} {}\n".format(*p))
        except:
            pass    

    ruffus.cmdline.run (options, pipeline_name = 'Cascade pipeline')
