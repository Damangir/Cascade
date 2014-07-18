"""
Logic tier of the cascade
"""
import os
import shutil
import csv
import itertools
import math

import cascade


def nonlinearRegistration(movImg, initAffine, movedImg, transM2F, transF2M, manager):
    def createMask(imgName):
        fnirtMask = manager.imageInSpace('fnirtmask.nii.gz', 'debug')
        img = manager.imageInSpace(imgName + '.brain.nii.gz', manager.calcSpace)        
        percentage = float(cascade.binary_proxy.fsl_check_output('fslstats', [img, '-P', '95']))
        cascade.binary_proxy.fsl_run('fslmaths', [img, '-uthr', str(percentage), '-mas', img, '-bin', fnirtMask])
        return fnirtMask
        
    fnirtOpt = {}
    
    fnirtMask = None
    
    if manager.checkImageInSpace('FLAIR.brain.nii.gz', manager.calcSpace):
        fnirtMask = createMask('FLAIR')
    elif manager.checkImageInSpace('T2.brain.nii.gz', manager.calcSpace):
        fnirtMask = createMask('T2')
        
    if fnirtMask: fnirtOpt['inmask'] = fnirtMask
    
    fnirtOpt['iout'] = movedImg
    fnirtOpt['aff'] = initAffine
    fnirtOpt['cout'] = transM2F
    fnirtOpt['config'] = cascade.config.FNIRT2mm
    
    cascade.binary_proxy.fnirt(movImg, **fnirtOpt)
    cascade.binary_proxy.fsl_run('invwarp', ['--ref={}'.format(movImg),
                                             '--warp={}'.format(transM2F),
                                             '--out={}'.format(transF2M) ])

def modelFreeSegmentation(inputs, bts, possible, output, manager):
    wmImg = manager.getTempFilename('wm.nii.gz')
    modelFree = manager.imageInSpace('model.free.nii.gz', 'debug')
    # Only voxels that have probable WM can contain WML
    cascade.binary_proxy.fsl_run('fslmaths', [bts, '-thr', 3, '-uthr', 3, '-bin', wmImg])
    cascade.binary_proxy.fsl_run('fslmaths', [wmImg, '-mul', 0, modelFree])

    def addWML(img, wm_deviation=0, light=True):
        thr = '-thr' if light else '-uthr'
        thisModelFree = manager.imageInSpace(manager.getImageType(img)+'.model.free.nii.gz', 'debug')
        if wm_deviation > 0:
            wmHist, bins = cascade.util.imageHistogram(img, wmImg, 100)        
            thresh = cascade.util.histExtent(wmHist, bins, wm_deviation, 0.5)
            mm = sum(thresh) / float(len(thresh))  
            if light:
                cascade.binary_proxy.fsl_run('fslmaths', [img, '-thr', thresh[1], '-sub', mm,'-max',0, '-div', thresh[1] - mm ,
                                                      '-mul', wmImg, '-save', thisModelFree , '-add', modelFree, modelFree])
            else:
                tmpfile = manager.getTempFilename(cascade.util.randomFilename())
                cascade.binary_proxy.fsl_run('fslmaths', [img, '-uthr', thresh[0], tmpfile])
                cascade.binary_proxy.fsl_run('fslmaths', [tmpfile, '-sub', mm, '-div', thresh[0] - mm , '-mas', tmpfile,
                                                      '-mul', wmImg, '-save', thisModelFree ,'-add', modelFree, modelFree])
    
    for img in inputs:
        imgType = manager.getImageType(img)
        if imgType not in cascade.modelFreeRules:
            continue
        addWML(img, **cascade.modelFreeRules[imgType])
    
    cascade.binary_proxy.fsl_run('fslmaths', [modelFree, '-add', possible, '-bin', output])
    
    
    
def warpVector(input, reference, warp, output, manager):
    slicedPre = manager.getTempFilename(cascade.util.randomFilename())
    fourDImage = slicedPre + '.nii.gz'
    fourDWarped = slicedPre + '.warp.nii.gz'
    warpedSlicedPre = slicedPre + '.warp'
    
    cascade.binary_proxy.cascade_run('vector-util', [input, '-slice', slicedPre])
    cascade.binary_proxy.fsl_run('fslmerge', ['-a', fourDImage] + [i for i in cascade.util.fileSeries(slicedPre)])
    cascade.binary_proxy.fsl_run('applywarp', ['--in={}'.format(fourDImage),
                                               '--ref={}'.format(reference),
                                               '--warp={}'.format(warp),
                                               '--out={}'.format(fourDWarped) ])
    cascade.binary_proxy.fsl_run('fslsplit', [fourDWarped, warpedSlicedPre])
    cascade.binary_proxy.cascade_run('vector-util', ['-compose', warpedSlicedPre, output])
    

def mergeIndividualModel(input, pve, output, manager):    
    prefix = manager.getTempFilename(cascade.util.randomFilename())
    vector_commands = []
    for img in input:
        tissueName = filter(lambda x:x in img, manager.brainTissueNames.values())[0]
        tissueID = manager.brainTissueIDs[tissueName]
        mask = prefix + '_mask.nii.gz'
        tissueTypeModel = prefix + '_{}.nii.gz'.format(tissueID)
        cascade.binary_proxy.fsl_run('fslmaths', [pve, '-thr', tissueID, '-uthr', tissueID, mask])
        cascade.binary_proxy.cascade_run('vector-util', [img, '-mask', mask, tissueTypeModel])
        vector_commands.extend([tissueTypeModel, '-add'])
    
    vector_commands.pop()
    vector_commands.append(output)
    cascade.binary_proxy.cascade_run('vector-util', vector_commands)
