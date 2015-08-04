#!/usr/bin/env python

import os
import shutil
import argparse

import ruffus

import util.checkpython
import util.terminalsize

CSF_SEGMENTATION = 'CSF_segmentation.nii.gz'
INITIAL_SEGMENTATION = 'Initial_segmentation.nii.gz'
WMGM_MASK = 'White_Gray_mask.nii.gz'
WMGM_PRIOR = 'White_Gray_prior.nii.gz'

# Fix the envirnment variable for width. It is not corectly set on all platforms
os.environ['COLUMNS'] = str(util.terminalsize.get_terminal_size()[0])
defaultStr = ' (default: %(default)s)'

parser = ruffus.cmdline.get_argparse(description='Cascade academics: Tissue type segmentation.',
                                     version='Cascade academics v. 1.1',
                                     ignored_args=["key_legend_in_graph", "draw_graph_horizontally",
                                                     "flowchart_format", "checksum_file_name", "recreate_database",
                                                     "touch_files_only", "use_threads"])

# Output
outputOptions = parser.add_argument_group('Output options')
outputOptions.add_argument('--out', required=True,
                    help='Tissue type segmentation image')

outputOptions.add_argument('--intermediate', metavar='TMP',
                    help='Directory for intermediate files')

# Input files
inputOptions = parser.add_argument_group('Input files')
inputOptions.add_argument('--t1' , metavar='T1.nii.gz',
                    help='T1 image.')
inputOptions.add_argument('--flair', metavar='FLAIR.nii.gz',
                    help='FLAIR image')
inputOptions.add_argument('--pd', metavar='PD.nii.gz',
                    help='PD image')
inputOptions.add_argument('--t2', metavar='T2.nii.gz',
                    help='T2 image')

inputOptions.add_argument('--brain', metavar='brain.nii.gz',
                    help='brain mask image')

priorOptions = parser.add_argument_group('Prior files')
priorOptions.add_argument('--csf' , metavar='CSF.nii.gz',
                    help='CSF Prior probability image.')
priorOptions.add_argument('--gm' , metavar='WM.nii.gz',
                    help='GM Prior probability image.')
priorOptions.add_argument('--wm' , metavar='GM.nii.gz',
                    help='WM Prior probability image.')

options = parser.parse_args()
import cascade
print cascade.Copyright()
cascade.logger, cascade.logger_mutex = ruffus.proxy_logger.make_shared_logger_and_proxy (cascade.setup_logging_factory,
                                                                                         'Cascade',
                                                                                         [options.log_file, options.verbose])

if not any([options.t1, options.t2, options.flair, options.pd]):
    parser.error('At least one of input sequences must be set.')

if not any([options.csf, options.gm, options.wm]):
    with logger_mutex:
        logger.log(ruffus.cmdline.MESSAGE, 'No prior probability is set. Set prior probability to flat distribution.')
    options.csf = options.brain
    options.gm = options.brain
    options.wm = options.brain
    
if not all([options.csf, options.gm, options.wm]):
    parser.error('You should specify all or no prior')

import atexit, tempfile
safe_tmp_dir = tempfile.mkdtemp()
@atexit.register
def cleanAfter():
    shutil.rmtree(safe_tmp_dir)

if not options.intermediate:
    options.intermediate = safe_tmp_dir

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

print cascade.Copyright()
    


###############################################################################
# Initial brain tissue segmentation
###############################################################################
def initialSegParam():
    imgForSeg = filter(lambda x:x in inputImages, ['T1', 'T2', 'PD', 'FLAIR'])[0]
    
    inImages = [inputImages[imgForSeg],
            options.brain,
            options.csf,  # CSF Prior
            options.gm,  # GM Prior    
            options.wm,  # WM Prior    
            ]

    outImages = os.path.join(options.intermediate, INITIAL_SEGMENTATION)
                
    params = [
              inImages,
              outImages,
              imgForSeg,
              ]
    yield params

@ruffus.files(initialSegParam)
def initialSegmentation(input, output, imgForSeg):
    tts = os.path.join(safe_tmp_dir, 'tts.nii.gz')
    map_file = os.path.join(safe_tmp_dir, 'tts-relabel.map')
    
    with open(map_file, 'w') as f:
        if imgForSeg == 'T2':
            cascade.binary_proxy.cascade_run('TissueTypeSegmentation', [input[0], input[1], tts, input[4], input[3], input[2] ], tts)
            f.write("0    0\n1    3\n2    2\n3    1");
        elif imgForSeg == 'FLAIR':
            cascade.binary_proxy.cascade_run('TissueTypeSegmentation', [input[0], input[1], tts, input[2], input[4], input[3] ], tts)
            f.write("0    0\n1    1\n2    3\n3    2");
        else:
            cascade.binary_proxy.cascade_run('TissueTypeSegmentation', [input[0], input[1], tts, input[2], input[3], input[4] ], tts)
            f.write("0    0\n1    1\n2    2\n3    3");
        
    cascade.binary_proxy.cascade_run('relabel', [tts, map_file, output], output)


###############################################################################
# BTS segmentation
###############################################################################
def btsParam():
    imgForBTSSeg = filter(lambda x:x in inputImages, ['FLAIR', 'T2', 'PD'])
    if imgForBTSSeg:
        imgForBTSSeg = imgForBTSSeg[0]
    else:
        imgForBTSSeg = None
    inImages = [inputImages.get(imgForBTSSeg),
                os.path.join(options.intermediate, INITIAL_SEGMENTATION),
                0.85,  # alpha: Percentile of GM to be doublechecked
                0.2,  # beta:  Percentage of WM to GM voxels in surrounding in order for a voxel to be change to WM
                ]
    outImages = options.out
    params = [
              inImages,
              outImages,
              imgForBTSSeg
              ]
    yield params

@ruffus.follows(initialSegmentation)        
@ruffus.files(btsParam)
def brainTissueSegmentation(input, output, inputType):
    if not inputType:
        with logger_mutex:
            logger.warning('Can not refine tissue segmentation. Initial segmentation will be copied as refined')

        shutil.copy(input[1] , output)
        return 0

    cascade.binary_proxy.cascade_run('refineBTS', [input[0], input[1] , output, input[2], input[3]], output)
    cascade.binary_proxy.cascade_run('CorrectGrayMatterFalsePositive', [input[0], input[1] , output , 0.9, 0.4], output)


 
if __name__ == '__main__':
    ruffus.cmdline.run (options, pipeline_name='Cascade tissue type segmentation')
