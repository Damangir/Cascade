import os
import sys
import subprocess
import time


import cascade

def bash_source(src, pre=''):
    command = ['bash', '-c', 'source {} && env'.format(src)]
    proc = subprocess.Popen(command, stdout = subprocess.PIPE)
    for line in proc.stdout:
        line = line.strip()
        (key, _, value) = line.partition("=")
        if key.startswith(pre):
            cascade.logger.debug('Add ENV %s', line)
            os.environ[key] = value
    
    proc.communicate()

def check_output(cmd, args, output_files=None):
    args = list(map (str, args))
    command_txt = ' '.join([cmd] + args)
    try:        
        tc = time.time()
        output=subprocess.check_output([cmd] + args, stderr=sys.stdout).strip()
        if output:
            cascade.logger.debug(command_txt + '\nStdout: "%s"\nTimeElapsed:%.1f s', output[:100],time.time() - tc)
        else:
            cascade.logger.debug(command_txt + '\nTimeElapsed:%.1f s' ,time.time() - tc)
        return output
    except subprocess.CalledProcessError as e:
        cascade.logger.error('Error running %s', ' '.join([cmd] + args) )
    return None

def run(cmd, args, output_files = None):
    output = check_output(cmd, args, output_files)
    return None != output

FSLDIR=os.environ.get('FSLDIR', '')
cascade.logger.info('FSLDIR is %s', FSLDIR)
if not FSLDIR:
    raise Exception('Can not locate FSL installation. Please set FSLDIR')

FSLPRE=''
for p in os.environ["PATH"].split(os.pathsep) :
    for pre in ['', 'fsl5.0-']:
        if os.path.exists(os.path.join(p, pre+'fslinfo')):
            FSLPRE=pre
            FSLBIN=p
            
if not FSLPRE:
    fsl_config_file = os.path.join(FSLDIR, 'etc/fslconf/fsl.sh')
    if os.path.exists(fsl_config_file):
        cascade.logger.info('Sourcing %s', fsl_config_file)
        bash_source(fsl_config_file, pre="FSL")
    else:
        cascade.logger.error('Can not find FSL config file at %s', fsl_config_file)
        raise Exception('Can not find FSL config file')
        
cascade.logger.info('FSLDIR: %s, FSLPRE: %s', FSLDIR, FSLPRE)

def fsl_check_output(fslcmd, fslargs, output_files = None):
    fslcmd=os.path.join(FSLBIN, FSLPRE+fslcmd)
    return check_output(fslcmd, fslargs, output_files)

def fsl_run(fslcmd, fslargs, output_files = None):
    fslcmd=os.path.join(FSLBIN, FSLPRE+fslcmd)
    output = check_output(fslcmd, fslargs, output_files)
    return None != output

def flirt(inimg, ref, init=None, omat=None, out=None, applyxfm=None, output_files = None, **kwargs):
    fslargs = ['-in', inimg, '-ref', ref]
    if out:
        fslargs = fslargs + ['-out', out]
    if omat:
        fslargs = fslargs + ['-omat', omat]
    if init:
        fslargs = fslargs + ['-init', init]

    if applyxfm:
        if not init:
            raise Exception('Applyxfm requires init')
        else:
            fslargs = fslargs + ['-applyxfm']

    if kwargs is not None:
        for key, value in kwargs.iteritems():
            fslargs = fslargs + [ '-'+key, value]

    fsl_run('flirt', fslargs)

def fnirt(inimg, output_files=None, **kwargs):
    fslargs = ['--in='+inimg]

    if kwargs is not None:
        for key, value in kwargs.iteritems():
            fslargs = fslargs + [ '--'+key+'='+value]

    fsl_run('fnirt', fslargs)
    
cascade.logger.info('Cascade location: %s', cascade.config.ExecDir)

cascadeCommands = ['linRegister',
                   'resample',
                   'resampleVector',
                   'inhomogeneity',
                   'brainExtraction',
                   'extractCSF',
                   'separateWG',
                   'refineBTS',
                   'modelFree',
                   'localFeature',
                   ]

def cascade_run(cascadecmd, cascadeargs, output_files = None):
    if cascadecmd not in cascadeCommands:
        raise Exception("Unknown Cascade command {}".format(cascadecmd))
    cascadeBinary=os.path.join(cascade.config.ExecDir, cascadecmd)
    output = check_output(cascadeBinary, cascadeargs, output_files)
    return None != output

