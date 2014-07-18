import os
import shutil
from itertools import product

import cascade
import ruffus

# TODO: Update the chart at each run

print cascade.Copyright()
parser = ruffus.cmdline.get_argparse(description='Cascade academics')

parser.add_argument('-r', '--root' , required=True , help='Image root directory')

parser.add_argument('-t', '--t1' , required=True , help='T1 image')
parser.add_argument('-f', '--flair' , help='FLAIR image')
parser.add_argument('-p', '--pd' , help='PD image')
parser.add_argument('-s', '--t2' , help='T2 image')

parser.add_argument('-b', '--brain-mask' , help='Brain mask')
parser.add_argument('-m', '--brain-mask-space' , choices=['T1', 'T2', 'FLAIR', 'PD'] , help='Brain mask space')
parser.add_argument('-c', '--calculation-space' , choices=['T1', 'T2', 'FLAIR', 'PD'], default='T1' , help='Calculation space')

parser.add_argument('-d', '--model-dir' , help='Directory where the model located')

parser.add_argument('--radius', default=1, help='Radius of local histogram')
parser.add_argument('--min-bin', default=0, help='Value correspond to histogram minimum bin')
parser.add_argument('--max-bin', default=1, help='Value correspond to histogram maximum bin')
parser.add_argument('--num-bin', default=10, help='Number of bins in local histogram')

parser.add_argument('--simple' , action='store_true' , help='Mode to run the pipeline')

options = parser.parse_args()

radius = options.radius
min_bin = options.min_bin
max_bin = options.max_bin
num_bin = options.num_bin

if options.simple:
    trainMode = False
    testMode = False
else:
    trainMode = options.model_dir is None
    testMode = not trainMode

logger, logger_mutex = ruffus.cmdline.setup_logging (__name__, options.log_file, options.verbose)

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
        
@ruffus.files(originalImagesParam)
def originalImages(input, output):
    cascade.binary_proxy.fsl_run('fslchfiletype', ['NIFTI_GZ', input, output])
    cascade.binary_proxy.fsl_run('fslreorient2std', [output, output])

###############################################################################
# Bring the brain mask into the pipeline
###############################################################################
@ruffus.transform(options.brain_mask, ruffus.regex('.*'), cascadeManager.imageInSpace('brainMask.nii.gz', options.brain_mask_space))
def brainMask(input, output):
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
# Linear registration to MNI space
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
# Resample MNI space to native
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
def resampleToStandard(input, output, manager):
    movingImage = input[0]
    fixedImage = input[1]
    transferFile = input[2]
    movedImage = output[0]
    if 'mask' in os.path.basename(movingImage).lower():
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)
    else:
        cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile], movedImage)
    
###############################################################################
# Register brain mask to images
###############################################################################
@ruffus.follows(interaRegistration)
@ruffus.transform(brainMask, ruffus.formatter(),
                  cascadeManager.imageInSpace('brainMask.nii.gz', cascadeManager.calcSpace),
                  cascadeManager)
def brainMaskRegistration(input, output, manager):
    movingImage = input
    movedImage = output
    fixedImage = manager.imageInSpace(manager.calcSpace + '.nii.gz', manager.calcSpace)
    transferFile = manager.transITKName(manager.getImageSpace(input), manager.getImageSpace(fixedImage))
    cascade.binary_proxy.cascade_run('resample', [fixedImage, movingImage, movedImage, transferFile, 'nn'], movedImage)

###############################################################################
# Brain extraction
###############################################################################
@ruffus.combinatorics.product(interaRegistration,
                              ruffus.formatter(),
                              brainMaskRegistration,
                              ruffus.formatter(),
                              cascadeManager.imageInSpace('{basename[0][0]}.brain{ext[0][0]}', cascadeManager.calcSpace),
                              cascadeManager)
def brainExtraction(input, output, manager):
    cascade.binary_proxy.fsl_run('fslmaths', [input[0][0], '-mas', input[1], output])

###############################################################################
# Normalize image
###############################################################################
@ruffus.transform(brainExtraction, ruffus.formatter(),
                  cascadeManager.imageInSpace('{basename[0]}.norm{ext[0]}', cascadeManager.calcSpace),
                  cascadeManager)
def normalize(input, output, manager):
    cascade.binary_proxy.cascade_run('inhomogeneity', [input, output], output)
    pass

###############################################################################
# CSF segmentation
###############################################################################
def csfParam():
    imgForCSFSeg = filter(lambda x:x in inputImages, ['FLAIR', 'T1','T2', 'PD'])[0]
    inImages = [cascadeManager.imageInSpace(imgForCSFSeg+'.brain.norm.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace('brainMask.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace(os.path.basename(cascade.config.StandardCSF),cascadeManager.calcSpace),
               ]
    btsParams = [
                 0.5, # Bias
                 3,   # nIteration
                 ]
    outImages = [
                cascadeManager.imageInSpace('csfMask.nii.gz', cascadeManager.calcSpace),
                ]
    
    params = [
              inImages,
              outImages,
              btsParams
              ]
    yield params
    
@ruffus.follows(resampleToStandard)        
@ruffus.follows(normalize)        
@ruffus.files(csfParam)
def csfSegmentation(input, output, param):
    cascade.binary_proxy.cascade_run('extractCSF', input + output + param, output)
    
###############################################################################
# Initial brain tissue segmentation
###############################################################################
def wgSepParam():
    imgForBTSSeg = filter(lambda x:x in inputImages, ['T1','T2', 'FLAIR', 'PD'])[0]
    inImages = [cascadeManager.imageInSpace(imgForBTSSeg+'.brain.norm.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace('brainMask.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace('csfMask.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace(os.path.basename(cascade.config.StandardGM),cascadeManager.calcSpace),
                cascadeManager.imageInSpace(os.path.basename(cascade.config.StandardWM),cascadeManager.calcSpace), ]
    btsParams = [
                 0.3, # bias
                 2,   # nIteration
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
# CSF segmentation
###############################################################################
def btsParam():
    imgForBTSSeg = filter(lambda x:x in inputImages, ['FLAIR', 'T2', 'PD'])[0]
    inImages = [cascadeManager.imageInSpace(imgForBTSSeg+'.brain.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace('WG.separation.nii.gz', cascadeManager.calcSpace),
               ]
    btsParams = [
                 0.5, # Alpha
                 0.2, # Beta
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
    imgForModelFree = filter(lambda x:x in inputImages, ['FLAIR', 'T2','PD'])[0]
    inImages = [cascadeManager.imageInSpace(imgForModelFree + '.brain.nii.gz', cascadeManager.calcSpace),
                cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                ]
    btsParams = [
                 2,   # variance
                 1,   # alpha
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
@ruffus.files(modelFreeParam)
def modelFreeSegmentation(input, output, param):
    cascade.binary_proxy.cascade_run('modelFree', input + output + param, output)

###############################################################################
# This is model free segmentation. Fine results for volume estimation.
###############################################################################
if options.simple:
    pass
###############################################################################
# This part onward is for normal run which contain modeling of normal brain
# and segmentation using predefined model
###############################################################################

if not options.simple:    
    ###############################################################################
    # Nonlinear registration to MNI space
    ###############################################################################
    @ruffus.transform(linearStandardRegistration, ruffus.regex(r'.*/(.*)/.*.brain.nii.gz$'),
                  [cascadeManager.transNLName(r'\1', 'MNI'),
                  cascadeManager.transNLName('MNI', r'\1')]
                  , cascadeManager)
    def nlStandardRegistration(input, output, manager):
        movedImg = manager.imageInSpace(manager.getImageType(input[0]) + '.nl.mni.nii.gz', 'debug')
        cascade.logic.nonlinearRegistration(input[0], input[1], movedImg, output[0], output[1], manager)
        imgName = cascadeManager.getQCNname('mni.nonlinear.gif')
    
if trainMode:
###############################################################################
# Warp all normalized images to MNI space
###############################################################################
    @ruffus.combinatorics.product(brainTissueSegmentation,
                                  ruffus.formatter(),
                                  nlStandardRegistration,
                                  ruffus.formatter(),
                                  '{subpath[0][0][1]}/MNI/{basename[0][0]}{ext[0][0]}',
                                  cascadeManager)
    def warpBTSToStandard(input, output, manager):
        cascade.util.ensureDirPresence(output)
        cascade.binary_proxy.fsl_run('applywarp', ['--in={}'.format(input[0][0]),
                                                   '--ref={}'.format(cascade.config.StandardImg),
                                                   '--warp={}'.format(input[1][0]),
                                                   '--out={}'.format(output),
                                                   '--interp=nn'])

    @ruffus.combinatorics.product(normalize,
                                  ruffus.formatter(),
                                  nlStandardRegistration,
                                  ruffus.formatter(),
                                  '{subpath[0][0][1]}/MNI/{basename[0][0]}{ext[0][0]}',
                                  cascadeManager)
    def warpToStandard(input, output, manager):
        cascade.util.ensureDirPresence(output)
        cascade.binary_proxy.fsl_run('applywarp', ['--in={}'.format(input[0]),
                                                   '--ref={}'.format(cascade.config.StandardImg),
                                                   '--warp={}'.format(input[1][0]),
                                                   '--out={}'.format(output) ])
###############################################################################
# Creat local histogram of each normalized image
###############################################################################
    @ruffus.transform(warpToStandard, ruffus.regex(r'.*/(.*)/(.*).brain.norm.nii.gz$'),
                      cascadeManager.imageInSpace(r'\2.hist.nii.gz', r'\1'),
                      cascadeManager)
    def stdLocalHistogram(input, output, manager):
        radius = 1
        min_bin = 0
        max_bin = 1.5
        num_bin = 10
        cascade.binary_proxy.cascade_run('local-hist', ['--input', input,
                                      '--output', output,
                                      '--radius', radius,
                                      '--min', min_bin,
                                      '--max', max_bin,
                                      '--nbin', num_bin])
if testMode:
###############################################################################
# Creat local histogram of each normalized image
###############################################################################
    @ruffus.transform(normalize, ruffus.regex(r'.*/(.*)/(.*).brain.norm.nii.gz$'),
                      cascadeManager.imageInSpace(r'\2.hist.nii.gz', r'\1'),
                      cascadeManager)
    def localHistogram(input, output, manager):
        cascade.binary_proxy.cascade_run('local-hist', ['--input', input,
                                      '--output', output,
                                      '--radius', radius,
                                      '--min', min_bin,
                                      '--max', max_bin,
                                      '--nbin', num_bin])

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
                       cascadeManager.transNLName('MNI', cascadeManager.calcSpace),
                       ),
                      cascadeManager.imageInSpace(modelName, os.path.join(cascadeManager.calcSpace, 'model')),
                      cascadeManager
                      ]
            yield params

    @ruffus.follows(nlStandardRegistration)
    @ruffus.files(registerModelParam)
    def registerModel(input, output, manager):
        cascade.logic.warpVector(input[0], input[1], input[2], output, manager)

###############################################################################
# Creat individual model for each sequence
###############################################################################
    @ruffus.follows(brainTissueSegmentation)
    @ruffus.collate(registerModel,
                    ruffus.regex(r'.*/(.*)\.(.*).model.nii.gz$'),
                    cascadeManager.imageInSpace(r'\1.model.nii.gz', cascadeManager.calcSpace),
                    {
                     'manager':cascadeManager,
                     'bts':cascadeManager.imageInSpace('brainTissueSegmentation.nii.gz', cascadeManager.calcSpace),
                     })
    def createIndividualModel(input, output, extra):
        bts = extra['bts']
        manager = extra['manager']
        cascade.logic.mergeIndividualModel(input, bts, output, manager)
        
###############################################################################
# Kolmogorov Smirnov to find p-value of each image with respect to model 
###############################################################################
    @ruffus.collate([localHistogram, createIndividualModel],
                    ruffus.regex(r'.*/(.*)\..*.nii.gz'),
                    cascadeManager.imageInSpace(r'\1.pvalue.nii.gz', cascadeManager.calcSpace),
                    cascadeManager)
    def KolmogorovSmirnov(input, output, manager):
        cascade.binary_proxy.cascade_run('ks', ['--input', input[0],
                                                '--reference', input[1],
                                                '--output', output,
                                                '--min1', min_bin,
                                                '--max1', max_bin,
                                                '--min2', min_bin,
                                                '--max2', max_bin])

if __name__ == '__main__':
    ruffus.cmdline.run (options)
