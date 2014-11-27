import sys
import os
import subprocess
import time

import cascade

def bash_source(src, pre=''):
    command = ['bash', '-c', 'source {} && env'.format(src)]
    proc = subprocess.Popen(command, stdout=subprocess.PIPE)
    for line in proc.stdout:
        line = line.strip()
        (key, _, value) = line.partition("=")
        if key.startswith(pre):
            cascade.logger.debug('Add ENV %s', line)
            os.environ[key] = value
    
    proc.communicate()

def check_output(cmd, args, output_files=None, silent=False, just_print=False):
    args = list(map (str, args))
    command_txt = ' '.join([cmd] + args)
    
    if just_print:
        with cascade.logger_mutex:
            cascade.logger.debug(command_txt)
        return ''
    
    try:        
        tc = time.time()
        output = subprocess.check_output([cmd] + args, stderr=sys.stdout, env=os.environ).strip()
        if not silent:
            with cascade.logger_mutex:
                if output:
                    cascade.logger.debug(command_txt + '\nStdout: "%s"\nTimeElapsed:%.1f s', output[:100], time.time() - tc)
                else:
                    cascade.logger.debug(command_txt + '\nTimeElapsed:%.1f s' , time.time() - tc)
        return output
    except subprocess.CalledProcessError as e:
        if not silent:
            with cascade.logger_mutex:
                cascade.logger.error('Error running %s', ' '.join([cmd] + args))
        cascade.util.ensureAbsence(output_files)
    except OSError as e:
        with cascade.logger_mutex:
            cascade.logger.error('%s: [OSError %d] %s', command_txt, e.errno, e.strerror)
    return None

def run(cmd, args, output_files=None, silent=False, just_print=False):
    output = check_output(cmd, args, output_files=output_files, silent=silent, just_print=just_print)
    return None != output

FSLPRE = ''
FSLFOUND = False
FSLBIN = ''
for pre in ['', 'fsl5.0-']:
    fslinfo_full = cascade.util.which(pre + 'fslinfo') 
    if fslinfo_full != None:
        FSLPRE = pre
        FSLFOUND = True
        FSLBIN = os.path.dirname(fslinfo_full)
        
if not FSLFOUND:
    raise Exception('Can not locate FSL installation. Please check your installation')

with cascade.logger_mutex:
    cascade.logger.info('FSL Binary are at %s (FSLPRE: %s)', FSLBIN, FSLPRE)

def fsl_check_output(fslcmd, fslargs, output_files=None):
    fslcmd = os.path.join(FSLBIN, FSLPRE + fslcmd)
    return check_output(fslcmd, fslargs, output_files)

def fsl_run(fslcmd, fslargs, output_files=None):
    return None != fsl_check_output(fslcmd, fslargs, output_files)
    
cascadeCommands = ['linRegister',
                   'resample',
                   'resampleVector',
                   'ComposeToVector',
                   'ImportImage',
                   
                   'inhomogeneity',
                   'brainExtraction',

                   'relabel',

                   'refineBTS',
                   'CorrectGrayMatterFalsePositive',
                   'TissueTypeSegmentation',
                   
                   'EvidentNormal',
                   'OneSampleKolmogorovSmirnovTest',
                   'TwoSampleKolmogorovSmirnovTest',

                   'atlas',
                   ]

for cascadecmd in cascadeCommands:
    cascadeBinary = os.path.join(cascade.config.ExecDir, cascadecmd)
    if not cascade.util.which(cascadeBinary):
        raise Exception("Cascade command {} not found".format(cascadecmd))

with cascade.logger_mutex:
    cascade.logger.info('Cascade location: %s', cascade.config.ExecDir)



def cascade_check_output(cascadecmd, cascadeargs, output_files=None):
    if cascadecmd not in cascadeCommands:
        raise Exception("Unknown Cascade command {}".format(cascadecmd))
    cascadeBinary = os.path.join(cascade.config.ExecDir, cascadecmd)
    return check_output(cascadeBinary, cascadeargs, output_files)

def cascade_run(cascadecmd, cascadeargs, output_files=None):
    return None != cascade_check_output(cascadecmd, cascadeargs, output_files)
