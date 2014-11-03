__author__  = "Soheil Damangir"
__status__  = "production"
__version__ = "1.0"
__date__    = "24 September 2014"

def Copyright():
    return """
The Cascade pipeline. http://ki.se/en/nvs/cascade
Copyright (C) 2013-2014 Soheil Damangir - All Rights Reserved
"""

def License():
    return """
You may use and distribute, but not modify this code under the terms of the Creative Commons Attribution-NonCommercial-NoDerivs 3.0 Unported License under the following conditions:
Attribution - You must attribute the work in the manner specified by the author or licensor (but not in any way that suggests that they endorse you or your use of the work).
Noncommercial - You may not use this work for commercial purposes.
No Derivative Works - You may not alter, transform, or build upon this work

To view a copy of the license, visit:
http://creativecommons.org/licenses/by-nc-nd/3.0/
"""

import os
import shutil
import logging

logging.basicConfig()
class DummyMutex():    
    def __enter__(self):
      pass
    def __exit__(self, *rest):
      pass

logger = logging.getLogger(__name__)  
logger_mutex = DummyMutex()
verbose = os.environ.get('CASCADE_VERBOSE')
if verbose == '0':
    logger.setLevel(logging.CRITICAL)
elif verbose == '1':
    logger.setLevel(logging.ERROR)
elif verbose == '2':
    logger.setLevel(logging.WARNING)
elif verbose == '3':
    logger.setLevel(logging.INFO)
elif verbose == '4':
    logger.setLevel(logging.DEBUG)
else:
    logger.setLevel(logging.CRITICAL)

from cascade import config
from cascade import util
from cascade import binary_proxy

class CascadeFileManager(object):

    sequences = ('T1', 'FLAIR', 'T2', 'PD')
    brainTissueNames = {1:'CSF', 2:'GM', 3:'WM'}
    brainTissueIDs = dict((v,k) for k, v in brainTissueNames.iteritems())

    
    _trans_dir = 'trans'
    _image_dir = 'image'
    _report_dir = 'report'
    _qc_dir = 'QC'
    
    @classmethod
    def trans_name(cls,img1, img2):
        return '{0}_to_{1}'.format(img1, img2)
    @classmethod
    def itk_trans_name(cls,img1, img2):
        return '{0}.tfm'.format(cls.trans_name(img1, img2))
    @classmethod
    def itk_nl_trans_name(cls,img1, img2):
        return '{0}.nl.tfm'.format(cls.trans_name(img1, img2))
    @classmethod
    def fsl_trans_name(cls,img1, img2):
        return '{0}.mat'.format(cls.trans_name(img1, img2))
    @classmethod
    def nonlinear_trans_name(cls,img1, img2):
        return '{0}_NL.nii.gz'.format(cls.trans_name(img1, img2))
    
    def __init__(self, root):
        self.root = os.path.abspath(os.path.expanduser(root))
        if not os.path.isdir(os.path.dirname(self.root)):
            raise Exception('Invalid root directory: ' + self.root)

        if not os.path.isdir(self.root): os.mkdir(self.root)
                
        with logger_mutex:
            logger.debug('root: %s', self.root)
              
        import tempfile
        self.safe_tmp= tempfile.mkdtemp()
        logger.debug('temp: %s', self.safe_tmp)
    
    def __del__(self):
        try:
            with logger_mutex:
                logger.debug('Removing: %s', self.safe_tmp)
            shutil.rmtree(self.safe_tmp)
        except:
            with logger_mutex:
                logger.error('Unable to remove: %s', self.safe_tmp)

    @property
    def calcSpace(self):
        return getattr(self, '_calcSpace', None)
    @calcSpace.setter
    def calcSpace(self, value):
        if value not in self.sequences:
            raise Exception('Unsupported sequence {}.'.format(value))        
        self._calcSpace = value

    @property
    def transDir(self):
        return os.path.join(self.root, self._trans_dir)
    @property
    def imageDir(self):
        return os.path.join(self.root, self._image_dir)
    @property
    def reportDir(self):
        return os.path.join(self.root, self._report_dir)
    @property
    def qcDir(self):
        return os.path.join(self.root, self._qc_dir)

    def imageInSpace(self, image, space=None):
        if not space:
            space = self.calcSpace
        if space[0] != '\\':
            util.ensureDirectory(os.path.join(self.imageDir, space))
        return os.path.join(self.imageDir, space, image)

    def checkImageInSpace(self, imgName, space):
        return os.path.exists(self.imageInSpace(imgName, space))

    def getImageSpace(self, image):
        return os.path.basename(os.path.dirname(image))

    def getImageType(self, image):
        imgFileName = os.path.basename(image)
        for s in self.sequences:
            if s in imgFileName: return s
        return None
    
    def getTransFile(self, transname):
        util.ensureDirectory(self.transDir)
        return os.path.join(self.transDir, transname)

    def transITKName(self, img1, img2):
        return self.getTransFile( self.itk_trans_name(img1, img2))

    def transITKMLName(self, img1, img2):
        return self.getTransFile( self.itk_nl_trans_name(img1, img2))
    
    def transName(self, img1, img2):
        return self.getTransFile( self.fsl_trans_name(img1, img2))
    
    def transNLName(self, img1, img2):
        return self.getTransFile( self.nonlinear_trans_name(img1, img2))

    def reportName(self, name, subDir = ''):
        fname = os.path.join(self.reportDir, subDir, name)
        util.ensureDirPresence(fname)
        return fname 
        
    def getTempFilename(self, filename, extension=''):
        return os.path.join(self.safe_tmp, filename)+extension
    
    def getQCNname(self, filename, extension=''):
        util.ensureDirectory(self.qcDir)
        return os.path.join(self.qcDir, filename)+extension
