#!/usr/bin/env python

levels = 5
import sys
if sys.version_info<(2,7,0):
    sys.stderr.write("You need python 2.7 or later to run the Cascade.\n")
    sys.stderr.write("You are using python"+sys.version+".\n")
    exit(1)


import os
import shutil
from itertools import product

import cascade
import ruffus
import time
runtimeString = time.strftime("%Y%m%d%H%M%S")

# TODO: Purge unused scripts

print cascade.Copyright()
parser = ruffus.cmdline.get_argparse(description='Cascade academics: Segmentation of white matter hyperintensities.',
                                     version = 'Cascade academics v. 1.1',
                                     ignored_args = ["log_file", "key_legend_in_graph", "draw_graph_horizontally", 
                                                     "flowchart_format", "checksum_file_name", "recreate_database",
                                                     "touch_files_only", "use_threads"])

# Output
outputOptions = parser.add_argument_group('Output options')
outputOptions.add_argument('-r', '--root', required=True,
                    help='Image root directory')

# Input files
inputOptions = parser.add_argument_group('Input files')
inputOptions.add_argument('-t', '--t1' , required=True, metavar='T1.nii.gz',
                    help='T1 image')
inputOptions.add_argument('-f', '--flair', metavar='FLAIR.nii.gz',
                    help='FLAIR image')
inputOptions.add_argument('-p', '--pd',metavar='PD.nii.gz',
                    help='PD image')
inputOptions.add_argument('-s', '--t2',metavar='T2.nii.gz',
                    help='T2 image')

generalOptions = parser.add_argument_group('General Options')
generalOptions.add_argument('--levels', default=5, type=int,
                    help='Number of levels to evaluate histogram. Default = 5')
generalOptions.add_argument('--radius', default=1, type=float,
                    help='Radius of local histogram in millimeter. Default = 1.0mm')
generalOptions.add_argument('-c', '--calculation-space',
                    choices=['T1', 'T2', 'FLAIR', 'PD'], default='T1',
                    help='Calculation space')

# importing Options
importOptions = parser.add_argument_group('Import processes')
importOptions.add_argument('-b', '--brain-mask',metavar='brainMask.nii.gz',
                    help='Brain mask')
importOptions.add_argument('-m', '--brain-mask-space',
                    choices=['T1', 'T2', 'FLAIR', 'PD'],
                    help='Brain mask space')

importOptions.add_argument('--freesurfer', help='Import freesurfer')


modeOptions = parser.add_argument_group('Pipeline procedure control')
modeOptions.add_argument('--simple' , action='store_true',
                    help='Mode to run the pipeline')
modeOptions.add_argument('-d', '--model-dir',
                    help='Directory where the trained model located.')

simpleOptions = parser.add_argument_group('Simple mode options')
simpleOptions.add_argument('--spread', default=2, type=float,
                    help='Relative brightness/darkness of WML. It controls how'
                         ' aggressive the pipeline will be. Higher spread, '
                         'smaller lesion size. Default = 2.0')

reportOptions = parser.add_argument_group('Reporting controls')
reportOptions.add_argument('--threshold', default=0, type=float,
                    help='threshold to create report. Default = 1.0')

options = parser.parse_args()
if not any([options.t2,options.flair,options.pd]):
    parser.error('At least one of FLAIR, T2 or PD should be specified.')


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

if testMode:
    for i in product(inputImages.keys(), cascadeManager.brainTissueNames.values()):
        modelName = os.path.join(options.model_dir, '.'.join(i) + '.model.nii.gz')
        print modelName
        if not os.path.exists(modelName):
            raise Exception('No model found at {}'.format(modelName))

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
    cascade.binary_proxy.cascade_run('linRegister', [fixedImage, movingImage, transferFile, invTransferFile])
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



if do_BrainExtract and options.brain_mask:
    do_BrainExtract = False
    ###############################################################################
    # Bring the brain mask into the pipeline
    ###############################################################################
    @ruffus.transform(options.brain_mask, ruffus.regex('.*'), cascadeManager.imageInSpace('brain_mask.nii.gz', options.brain_mask_space))
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
# TODO: Estimate WM mask for N4 normalization
    @ruffus.transform(brainExtraction, ruffus.formatter(),
                      cascadeManager.imageInSpace('norm.mask.nii.gz', cascadeManager.calcSpace),
                      cascadeManager)
    def normalizationMask(input, output, param):
        cascade.binary_proxy.fsl_run('fslmaths', [input, '-bin', output])
    

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
    # CSF segmentation
    ###############################################################################
    def csfParam():
        imgForCSFSeg = filter(lambda x:x in inputImages, ['FLAIR', 'T1', 'T2', 'PD'])[0]
        inImages = [cascadeManager.imageInSpace(imgForCSFSeg + '.norm.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace(os.path.basename(cascade.config.StandardCSF), cascadeManager.calcSpace),
                  ]
        btsParams = [
                     0.5,  # Bias
                     3,  # nIteration
                     ]
        outImages = [
                    cascadeManager.imageInSpace('csf_mask.nii.gz', cascadeManager.calcSpace),
                    ]
        params = [
                  inImages,
                  outImages,
                  btsParams
                  ]
        yield params
   
    @ruffus.follows(resampleStdToNative)        
    @ruffus.follows(normalize)        
    @ruffus.files(csfParam)
    def csfSegmentation(input, output, param):
        cascade.binary_proxy.cascade_run('extractCSF', input + output + param, output)
   
    ###############################################################################
    # Initial brain tissue segmentation
    ###############################################################################
    def wgSepParam():
        # We can not perform this step on FLAIR as the algorithm suspect WM similar to
        # CSF as WML, which is not the case in FLAIR
        imgForBTSSeg = filter(lambda x:x in inputImages, ['T1', 'T2', 'PD'])[0]
        inImages = [cascadeManager.imageInSpace(imgForBTSSeg + '.norm.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('brain_mask.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('csf_mask.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace(os.path.basename(cascade.config.StandardGM), cascadeManager.calcSpace),
                    cascadeManager.imageInSpace(os.path.basename(cascade.config.StandardWM), cascadeManager.calcSpace), ]
        btsParams = [
                     0.3,  # bias
                     2,  # nIteration
                    ]
        outImages = [
                    cascadeManager.imageInSpace('WG.separation.nii.gz', cascadeManager.calcSpace),
                    ]
        params = [
                  inImages,
                  outImages,
                  btsParams
                  ]
        yield params
   
    @ruffus.follows(csfSegmentation)        
    @ruffus.files(wgSepParam)
    def WhiteGrayMatterSeparation(input, output, param):
        cascade.binary_proxy.cascade_run('separateWG', input + output + param, output)

    ###############################################################################
    # BTS segmentation
    ###############################################################################
    def btsParam():
        imgForBTSSeg = filter(lambda x:x in inputImages, ['FLAIR', 'T2', 'PD'])[0]
        inImages = [cascadeManager.imageInSpace(imgForBTSSeg + '.norm.nii.gz', cascadeManager.calcSpace),
                    cascadeManager.imageInSpace('WG.separation.nii.gz', cascadeManager.calcSpace),
                   ]
        btsParams = [
                     0.5,  # Alpha (spread)
                     0.2,  # Beta (birth threshold)
                     ]
        outImages = [
                    cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                    ]
        params = [
                  inImages,
                  outImages,
                  btsParams
                  ]
        yield params
   
    @ruffus.follows(WhiteGrayMatterSeparation)        
    @ruffus.files(btsParam)
    def brainTissueSegmentation(input, output, param):
        cascade.binary_proxy.cascade_run('refineBTS', input + output + param, output)

###############################################################################
# mark evidently normal brain
###############################################################################
def modelFreeParam():
    imgForModelFree = filter(lambda x:x in inputImages, ['FLAIR', 'T2', 'PD'])[0]
    inImages = [cascadeManager.imageInSpace(imgForModelFree + '.norm.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                ]
    btsParams = [
                 options.radius, # variance (for smoothing in pyramid creation)
                 options.spread, # alpha (spread, its sign controls whether MWL is dark or bright.)
                 options.levels, # number of levels to evaluate
                 ]
    outImages = [
                cascadeManager.imageInSpace('model.free.wml.nii.gz', cascadeManager.calcSpace),
                ]
   
    params = [
              inImages,
              outImages,
              btsParams
              ]
    yield params

@ruffus.follows(brainTissueSegmentation)
@ruffus.follows(normalize)
@ruffus.files(modelFreeParam)
def modelFreeSegmentation(input, output, param):
    cascade.binary_proxy.cascade_run('modelFree', input + output + param, output)

###############################################################################
# This is model free segmentation. Fine results for volume estimation.
###############################################################################
if options.simple:
    if not options.target_tasks:
        options.target_tasks = ['modelFreeSegmentation']
 
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
        movingImage = input[0][0]
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

###############################################################################
# Creat local feature of each normalized image
###############################################################################
    @ruffus.follows(resampleBTSToStandard)
    @ruffus.transform(resampleToStandard, ruffus.regex(r'.*/(.*)/(.*).norm.nii.gz$'),
                      cascadeManager.imageInSpace(r'\2.feature.nii.gz', r'\1'),
                      cascadeManager)
    def stdLocalFeature(input, output, manager):
        cascade.binary_proxy.cascade_run('localFeature', [input,
                                                          manager.imageInSpace('brainTissueSegmentation.nii.gz', 'STD'),
                                                          output,
                                                          options.radius,
                                                          options.levels], output)

if testMode:
###############################################################################
# Creat local feature of each normalized image
###############################################################################
    @ruffus.transform(normalize, ruffus.regex(r'.*/(.*)/(.*).norm.nii.gz$'),
                      cascadeManager.imageInSpace(r'\2.feature.nii.gz', r'\1'),
                      cascadeManager)
    def localFeature(input, output, manager):
        cascade.binary_proxy.cascade_run('localFeature', [input,
                                                          manager.imageInSpace('brain_mask.nii.gz', manager.calcSpace),
                                                          output,
                                                          options.radius,
                                                          options.levels], output)

###############################################################################
# Register normal model to the calculation space
###############################################################################
    def registerModelParam():
        modelPrefix = ''
        for i in product(inputImages.keys(), cascadeManager.brainTissueNames.values()):
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
        cascade.binary_proxy.cascade_run('resampleVector', [fixedImage, movingImage, movedImage, transferFile], movedImage)

    @ruffus.collate(registerModel,
                    ruffus.regex(r'.*/(.*)\.(.*).model.nii.gz'),
                    cascadeManager.imageInSpace(r'\1.model.nii.gz', os.path.join(cascadeManager.calcSpace, 'model')),
                    cascadeManager)    
    def createIndividualModel(input, output, manager):
        map = manager.imageInSpace('brainTissueSegmentation.nii.gz', manager.calcSpace)
        input = [
                 filter(lambda x:'CSF' in os.path.basename(x), input)[0],
                 filter(lambda x:'GM' in os.path.basename(x), input)[0],
                 filter(lambda x:'WM' in os.path.basename(x), input)[0],
                 ]
        cascade.binary_proxy.cascade_run('combine', [map, output] + inputs, output)
###############################################################################
# Kolmogorov Smirnov to find p-value of each image with respect to model
###############################################################################
    @ruffus.collate([localFeature, createIndividualModel],
                    ruffus.regex(r'.*/(.*)\..*.nii.gz'),
                    cascadeManager.imageInSpace(r'\1.pvalue.nii.gz', cascadeManager.calcSpace),
                    cascadeManager)
    def KolmogorovSmirnov(input, output, manager):
        cascade.binary_proxy.cascade_run('ks', [input[0], input[1], output], output)

if has_atlas and not testMode:
    options.target_tasks = ['report']
    def reportParam():
        if options.simple:
            finalOutput = cascadeManager.imageInSpace('model.free.wml.nii.gz', cascadeManager.calcSpace)
        else:
            finalOutput = cascadeManager.imageInSpace('pvalue.wml.nii.gz', cascadeManager.calcSpace)

        inImages = [finalOutput,
                    cascadeManager.imageInSpace('atlas.nii.gz', cascadeManager.calcSpace),
                    float(options.threshold),
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
    @ruffus.follows(modelFreeSegmentation)
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
    if not options.flowchart:
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

    exit(0)
    ruffus.cmdline.run (options, pipeline_name = 'Cascade pipeline')
