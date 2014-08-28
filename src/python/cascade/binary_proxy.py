import sys
import os
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

def check_output(cmd, args, output_files=None, silent=False):
    args = list(map (str, args))
    command_txt = ' '.join([cmd] + args)
    return ''
    try:        
        tc = time.time()
        output=subprocess.check_output([cmd] + args, stderr=sys.stdout, env=os.environ).strip()
        if not silent:
            if output:
                cascade.logger.debug(command_txt + '\nStdout: "%s"\nTimeElapsed:%.1f s', output[:100],time.time() - tc)
            else:
                cascade.logger.debug(command_txt + '\nTimeElapsed:%.1f s' ,time.time() - tc)
        return output
    except subprocess.CalledProcessError as e:
        if not silent: cascade.logger.error('Error running %s', ' '.join([cmd] + args) )
        if type(output_files) is type(''):
            ensureAbsence(output_files)
        else:
            for f in output_files:
                ensureAbsence(output_files)
    return None

def run(cmd, args, output_files = None, silent=False):
    output = check_output(cmd, args, output_files=output_files, silent=silent)
    return None != output

FSLPRE=''
FSLFOUND=False
FSLBIN=''
for pre in ['', 'fsl5.0-']:
    fslinfo_full = check_output('command', ['-v', pre+'fslinfo'], silent=True) 
    if fslinfo_full != None:
        FSLPRE=pre
        FSLFOUND=True
        FSLBIN=os.path.dirname(fslinfo_full)
        
if not FSLFOUND:
    raise Exception('Can not locate FSL installation. Please check your installation')

cascade.logger.debug('FSL Binary are at %s (FSLPRE: %s)', FSLBIN, FSLPRE)

def fsl_check_output(fslcmd, fslargs, output_files = None):
    fslcmd=os.path.join(FSLBIN, FSLPRE+fslcmd)
    return check_output(fslcmd, fslargs, output_files)

def fsl_run(fslcmd, fslargs, output_files = None):
    fslcmd=os.path.join(FSLBIN, FSLPRE+fslcmd)
    output = check_output(fslcmd, fslargs, output_files)
    return None != output
    
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
                   'ks',
                   'combine',
                   'relabel',
                   ]

for cascadecmd in cascadeCommands:
    cascadeBinary=os.path.join(cascade.config.ExecDir, cascadecmd)
    if not run('command', ['-v', cascadeBinary], silent=True):
        raise Exception("Cascade command {} not found".format(cascadecmd))

cascade.logger.info('Cascade location: %s', cascade.config.ExecDir)


def cascade_run(cascadecmd, cascadeargs, output_files = None):
    if cascadecmd not in cascadeCommands:
        raise Exception("Unknown Cascade command {}".format(cascadecmd))
    cascadeBinary=os.path.join(cascade.config.ExecDir, cascadecmd)
    output = check_output(cascadeBinary, cascadeargs, output_files)
    return None != output

