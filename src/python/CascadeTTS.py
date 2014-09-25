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
WMGM_PRIOR= 'White_Gray_prior.nii.gz'

# Fix the envirnment variable for width. It is not corectly set on all platforms
os.environ['COLUMNS'] = str(util.terminalsize.get_terminal_size()[0])
defaultStr = ' (default: %(default)s)'

parser = ruffus.cmdline.get_argparse(description='Cascade academics: Tissue type segmentation.',
                                     version = 'Cascade academics v. 1.1',
                                     ignored_args = ["key_legend_in_graph", "draw_graph_horizontally", 
                                                     "flowchart_format", "checksum_file_name", "recreate_database",
                                                     "touch_files_only", "use_threads"])

# Output
outputOptions = parser.add_argument_group('Output options')
outputOptions.add_argument('--out', required=True,
                    help='Tissue type segmentation image')

outputOptions.add_argument('--intermediate',metavar='TMP',
                    help='Directory for intermediate files')

# Input files
inputOptions = parser.add_argument_group('Input files')
inputOptions.add_argument('--t1' , metavar='T1.nii.gz',
                    help='T1 image.')
inputOptions.add_argument('--flair', metavar='FLAIR.nii.gz',
                    help='FLAIR image')
inputOptions.add_argument('--pd',metavar='PD.nii.gz',
                    help='PD image')
inputOptions.add_argument('--t2',metavar='T2.nii.gz',
                    help='T2 image')

inputOptions.add_argument('--brain',metavar='brain.nii.gz',
                    help='brain mask image')

priorOptions = parser.add_argument_group('Prior files')
priorOptions.add_argument('--csf' , metavar='CSF.nii.gz',
                    help='CSF Prior probability image.')
priorOptions.add_argument('--gm' , metavar='WM.nii.gz',
                    help='GM Prior probability image.')
priorOptions.add_argument('--wm' , metavar='GM.nii.gz',
                    help='WM Prior probability image.')

options = parser.parse_args()
logger, logger_mutex = ruffus.cmdline.setup_logging (__name__, options.log_file, options.verbose)
os.environ['CASCADE_VERBOSE'] =  str(options.verbose)
import cascade
cascade.logger, cascade.logger_mutex = ruffus.cmdline.setup_logging ('cascade', options.log_file, options.verbose)

if not any([options.t1,options.t2,options.flair,options.pd]):
    parser.error('At least one of input sequences must be set.')

if not any([options.csf,options.gm,options.wm]):
    with logger_mutex:
        logger.log(ruffus.cmdline.MESSAGE, 'No prior probability is set. Set prior probability to flat distribution.')
    options.csf = options.brain
    options.gm = options.brain
    options.wm = options.brain
    
if not all([options.csf,options.gm,options.wm]):
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
 # CSF segmentation
 ###############################################################################
def csfParam():
    imgForCSFSeg = filter(lambda x:x in inputImages, ['FLAIR', 'T1', 'T2', 'PD'])[0]
    
    inImages = [inputImages[imgForCSFSeg],
                options.brain,
                options.csf,   # CSF Prior
                options.gm,    # GM Prior    
                options.wm,    # WM Prior    
                ]
    
    outImages = os.path.join(options.intermediate, CSF_SEGMENTATION)
                
    params = [
              inImages,
              outImages,
              ]
    yield params

@ruffus.files(csfParam)
def csfSegmentation(input, output):
    wg_prior = os.path.join(safe_tmp_dir, WMGM_PRIOR)
    cascade.binary_proxy.fsl_run('fslmaths', [input[3], '-add', input[4], wg_prior], wg_prior)
    cascade.binary_proxy.cascade_run('tts', [input[0],input[1], output, input[2], wg_prior], output)


###############################################################################
# Initial brain tissue segmentation
###############################################################################
def wgSepParam():
    imgForWGSeg = filter(lambda x:x in inputImages, ['T1', 'T2', 'PD', 'FLAIR'])[0]
    
    inImages = [inputImages[imgForWGSeg],
                os.path.join(options.intermediate, CSF_SEGMENTATION),
                options.gm, # GM Prior
                options.wm, # WM Prior    
                ]
    
    outImages = os.path.join(options.intermediate, INITIAL_SEGMENTATION)
                
    params = [
              inImages,
              outImages,
              ]
    yield params

@ruffus.follows(csfSegmentation)        
@ruffus.files(wgSepParam)
def WhiteGrayMatterSeparation(input, output):
    csf_seg = input[1]
    wm_gm_mask = os.path.join(options.intermediate, WMGM_MASK)

    map_file = os.path.join(safe_tmp_dir, 'csfSeg2WGmask.map')
    with open(map_file, 'w') as f:
        f.write("0    0\n1    0\n2    1");
        
    cascade.binary_proxy.cascade_run('relabel', [csf_seg, map_file, wm_gm_mask], wm_gm_mask)
    cascade.binary_proxy.cascade_run('tts', [input[0], wm_gm_mask, output, input[2], input[3]], output)
    cascade.binary_proxy.fsl_run('fslmaths', [csf_seg,'-bin', '-add', output, output], output)


###############################################################################
# BTS segmentation
###############################################################################
def btsParam():
    imgForBTSSeg = filter(lambda x:x in inputImages, ['FLAIR', 'T2', 'PD'])
    if imgForBTSSeg:
        imgForBTSSeg=imgForBTSSeg[0]
    else:
        imgForBTSSeg=None
    inImages = [inputImages.get(imgForBTSSeg),
                os.path.join(options.intermediate, INITIAL_SEGMENTATION),
                0.5,  # Alpha (spread)
                0.2,  # Beta (birth threshold)
                ]
    outImages = options.out
    params = [
              inImages,
              outImages,
              ]
    yield params

@ruffus.follows(WhiteGrayMatterSeparation)        
@ruffus.files(btsParam)
def brainTissueSegmentation(input, output):
    if not input[0]:
        with logger_mutex:
            logger.warning('Can not refine tissue segmentation. Initial segmentation will be copied as refined')

        shutil.copy(input[1] , output)
    else:
        cascade.binary_proxy.cascade_run('refineBTS', [input[0], input[1] , output, input[2], input[3]], output)

if __name__ == '__main__':
    ruffus.cmdline.run (options, pipeline_name = 'Cascade tissue type segmentation')
