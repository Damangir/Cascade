import sys
import os
import subprocess
import time
import threading
import Queue

import cascade

def communicate_and_log(proc, logger_obj, logger_mutex=cascade.DummyMutex()):    
    LogFunc = {
               'STDOUT':logger_obj.info,
               'STDERR':logger_obj.debug,
               } 
    io_q = Queue.Queue()
    data={}
    def stream_watcher(identifier, stream):
        data[identifier] = data.get(identifier,[])
        for line in stream:
            io_q.put((identifier, line))
            data[identifier].append(line)
        if not stream.closed:
            stream.close()
    
    if proc.stdout:
        out_thr = threading.Thread(target=stream_watcher, args=('STDOUT', proc.stdout))
        out_thr.setDaemon(True)
        out_thr.start()
    if proc.stderr:
        err_thr = threading.Thread(target=stream_watcher, args=('STDERR', proc.stderr))
        err_thr.setDaemon(True)
        err_thr.start()
        
    def printer():
        while True:
            try:
                # Block for 1 second.
                item = io_q.get(True, 0.1)
            except Queue.Empty:
                # No output in either streams for a second. Are we done?
                if proc.poll() is not None:
                    break
            else:
                identifier, line = item
                with logger_mutex:
                    LogFunc.get(identifier, logger_obj.debug)(line)
    
    pr_thr = threading.Thread(target=printer)
    pr_thr.setDaemon(True)
    pr_thr.start()
    pr_thr.join()
    
    if proc.stderr:
        err_thr.join()
    if proc.stdout:
        out_thr.join()

    return '\n'.join(data.get('STDOUT',[])), '\n'.join(data.get('STDERR',[]))
 
def bash_source(src, pre=''):
    command = ['bash', '-c', 'source {} && env'.format(src)]
    proc = subprocess.Popen(command, stdout=subprocess.PIPE)
    for line in proc.stdout:
        line = line.strip()
        (key, _, value) = line.partition("=")
        if key.startswith(pre):
            with cascade.logger_mutex:
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
        with cascade.logger_mutex:
            cascade.logger.debug(command_txt)
            
        process = subprocess.Popen([cmd] + args,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   env=dict(os.environ))
        out, err = communicate_and_log(process, cascade.logger, cascade.logger_mutex)
        out = out.strip()
        err = err.strip()
        retcode = process.poll()
        if retcode:
            with cascade.logger_mutex:
                cascade.logger.error('Error running %s', ' '.join([cmd] + args))
            cascade.util.ensureAbsence(output_files)
            raise subprocess.CalledProcessError(retcode, cmd, out)
        else:
            with cascade.logger_mutex:
                cascade.logger.debug(command_txt + '\nTimeElapsed:%.1f s' , time.time() - tc)
            return out
    except OSError as e:
        with cascade.logger_mutex:
            cascade.logger.error('%s: [OSError %d] %s', command_txt, e.errno, e.strerror)
        cascade.util.ensureAbsence(output_files)
        raise e
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
