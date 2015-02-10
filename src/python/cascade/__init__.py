__author__  = "Soheil Damangir"
__status__  = "production"
__version__ = "1.0"
__date__    = "24 September 2014"

def Copyright():
    return """
The Cascade Academics pipeline. http://ki.se/en/nvs/cascade
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

import sys
import os
import shutil
import logging

#logging.basicConfig()
class DummyMutex():    
    def __enter__(self):
      pass
    def __exit__(self, *rest):
      pass

logger = logging.getLogger(__name__)  
logger_mutex = DummyMutex()


def get_logging_level_from_verbose(verbose):
    if verbose == 0:
        return logging.CRITICAL
    elif verbose == 1:
        return logging.ERROR
    elif verbose == 2:
        return logging.WARNING
    elif verbose == 3:
        return logging.INFO
    elif verbose == 4:
        return logging.DEBUG
    elif verbose > 4:
        return 0
    else:
        return logging.CRITICAL
    
def setup_logging_factory (logger_name, args):
    log_file_name, verbose = args
    """
    This function is a copy of setup_logging_factory of ruffus pipeline
    """
    #
    #   Log file name with logger level
    #
    new_logger = logging.getLogger(logger_name)

    class NullHandler(logging.Handler):
        """
        for when there is no logging
        """
        def emit(self, record):
            pass

    # We are interesting in all messages
    new_logger.setLevel(logging.DEBUG)
    has_handler = False

    # log to file if that is specified
    if log_file_name:
        handler = logging.FileHandler(log_file_name, delay=False)
        class stipped_down_formatter(logging.Formatter):
            def format(self, record):
                prefix = ""
                if not hasattr(self, "first_used"):
                    self.first_used = True
                    prefix = "-"*80 + Copyright() 
                    prefix +="%(name)s run at " % record.__dict__
                    prefix += self.formatTime(record, "%Y-%m-%d %H:%M:%S")+"\n" 
                    prefix += "-"*80
                    prefix += "\n"

                self._fmt = " %(asctime)s %(levelname)-7s: %(message)s"
                return prefix + logging.Formatter.format(self, record)
        handler.setFormatter(stipped_down_formatter("%(asctime)s - %(name)s - %(levelname)6s - %(message)s", "%H:%M:%S"))
        handler.setLevel(0)
        new_logger.addHandler(handler)
        has_handler = True

    # log to stderr if verbose
    if verbose:
        stderrhandler = logging.StreamHandler(sys.stderr)
        stderrhandler.setFormatter(logging.Formatter("%(message)s"))
        stderrhandler.setLevel(logging.INFO)
        new_logger.addHandler(stderrhandler)
        has_handler = True

    # no logging
    if not has_handler:
        new_logger.addHandler(NullHandler())

    #
    #   This log object will be wrapped in proxy
    #
    return new_logger



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
